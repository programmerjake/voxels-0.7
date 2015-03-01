/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "world/world.h"
#include "generate/random_world_generator.h"
#include "world/view_point.h"
#include <random>
#include "block/builtin/air.h"
#include "block/builtin/stone.h"
#include "block/builtin/grass.h"
#include "entity/builtin/items/stone.h"
#include <cmath>
#include <random>
#include <limits>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "util/logging.h"
#include "util/global_instance_maker.h"
#include "platform/thread_priority.h"
#include "util/wrapped_entity.h"

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace
{
class MyWorldGenerator final : public RandomWorldGenerator
{
    friend class global_instance_maker<MyWorldGenerator>;
private:
    MyWorldGenerator()
    {
    }
public:
    static const WorldGenerator *getInstance()
    {
        return global_instance_maker<MyWorldGenerator>::getInstance();
    }
protected:
    virtual void generate(PositionI chunkBasePosition, BlocksArray &blocks, World &world, WorldLockManager &lock_manager, RandomSource &randomSource) const override
    {
        BlockIterator bi = world.getBlockIterator(chunkBasePosition);
        for(int x = 0; x < BlockChunk::chunkSizeX; x++)
        {
            for(int z = 0; z < BlockChunk::chunkSizeZ; z++)
            {
                PositionI columnBasePosition = chunkBasePosition + VectorI(x, 0, z);
                bi.moveTo(columnBasePosition);
                const BiomeProperties &bp = bi.getBiomeProperties(lock_manager);
                float groundHeightF = 0;
                for(const BiomeWeights::value_type &v : bp.getWeights())
                {
                    BiomeDescriptorPointer biome = std::get<0>(v);
                    float weight = std::get<1>(v);
                    groundHeightF += weight * biome->getGroundHeight(columnBasePosition, randomSource);
                }
                int groundHeight = ifloor(0.5f + groundHeightF);
                bp.getDominantBiome()->makeGroundColumn(chunkBasePosition, columnBasePosition, blocks, randomSource, groundHeight);
            }
        }
    }
};
}

World::World(SeedType seed)
    : World(seed, MyWorldGenerator::getInstance())
{

}

namespace
{
World::SeedType makeRandomSeed()
{
    random_device g;
    return uniform_int_distribution<World::SeedType>()(g);
}
}

World::World(SeedType seed, const WorldGenerator *worldGenerator, internal_construct_flag)
    : randomGenerator(seed), wrng(*this), physicsWorld(std::make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed), destructing(false), lightingStable(false)
{
}

World::World()
    : World(makeRandomSeed())
{

}

BlockUpdate *World::removeAllBlockUpdatesInChunk(BlockUpdateKind kind, BlockIterator bi, WorldLockManager &lock_manager)
{
    BlockChunk *chunk = bi.chunk;
    lock_manager.clear();
    BlockChunkFullLock lockChunk(*chunk);
    auto lockIt = std::unique_lock<decltype(chunk->chunkVariables.blockUpdateListLock)>(chunk->chunkVariables.blockUpdateListLock);
    BlockUpdate *retval = nullptr;
    for(BlockUpdate *node = chunk->chunkVariables.blockUpdateListHead; node != nullptr;)
    {
        if(node->kind == kind)
        {
            if(node->chunk_prev != nullptr)
                node->chunk_prev->chunk_next = node->chunk_next;
            else
                chunk->chunkVariables.blockUpdateListHead = node->chunk_next;
            if(node->chunk_next != nullptr)
                node->chunk_next->chunk_prev = node->chunk_prev;
            else
                chunk->chunkVariables.blockUpdateListTail = node->chunk_prev;
            BlockUpdate *next_node = node->chunk_next;
            node->chunk_prev = nullptr;
            node->chunk_next = retval;
            if(retval != nullptr)
                retval->chunk_prev = node;
            retval = node;
            node = next_node;
            VectorI relativePos = BlockChunk::getChunkRelativePosition(retval->position);
            for(BlockUpdate **pnode = &chunk->blocks[relativePos.x][relativePos.y][relativePos.z].updateListHead; *pnode != nullptr; pnode = &(*pnode)->block_next)
            {
                if(*pnode == retval)
                {
                    *pnode = retval->block_next;
                    break;
                }
            }
            retval->block_next = nullptr;
        }
        else
            node = node->chunk_next;
    }
    return retval;
}

World::World(SeedType seed, const WorldGenerator *worldGenerator)
    : randomGenerator(seed), wrng(*this), physicsWorld(std::make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed), destructing(false), lightingStable(false)
{
    getDebugLog() << L"generating initial world..." << postnl;
    lightingThread = thread([this]()
    {
        lightingThreadFn();
    });
    WorldLockManager lock_manager;
    for(int dx = -1; dx < 1; dx++)
    {
        for(int dz = -1; dz < 1; dz++)
        {
            BlockChunk *initialChunk = getBlockIterator(PositionI(dx * BlockChunk::chunkSizeX, 0, dz * BlockChunk::chunkSizeZ, Dimension::Overworld)).chunk;
            initialChunk->chunkVariables.generateStarted = true;
            getDebugLog() << L"generating " << initialChunk->basePosition << postnl;
            generateChunk(initialChunk, lock_manager);
            initialChunk->chunkVariables.generated = true;
        }
    }
    lock_manager.clear();
    getDebugLog() << L"lighting initial world..." << postnl;
    for(int i = 0; i < 500 && !isLightingStable(); i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    getDebugLog() << L"generated initial world." << postnl;
#if 1 || defined(NDEBUG)
    const int threadCount = 5;
#else
    const int threadCount = 1;
#endif
    for(int i = 0; i < threadCount; i++)
    {
        chunkGeneratingThreads.emplace_back([this]()
        {
            setThreadPriority(ThreadPriority::Low);
            chunkGeneratingThreadFn();
        });
    }
}

World::~World()
{
    assert(viewPoints.empty());
    destructing = true;
    if(lightingThread.joinable())
        lightingThread.join();
    for(auto &t : chunkGeneratingThreads)
        if(t.joinable())
            t.join();
}

Lighting World::getBlockLighting(BlockIterator bi, WorldLockManager &lock_manager, bool isTopFace)
{
    Block b = bi.get(lock_manager);
    if(!b)
    {
        switch(bi.position().d)
        {
        case Dimension::Overworld:
            if(bi.position().y >= 64 && isTopFace)
                return Lighting::makeSkyLighting();
            return Lighting();
        }
        assert(false);
        return Lighting();
    }
    return b.lighting;
}

void World::lightingThreadFn()
{
    setThreadPriority(ThreadPriority::Low);
    WorldLockManager lock_manager;
    BlockChunkMap *chunks = &physicsWorld->chunks;
    while(!destructing)
    {
        bool didAnything = false;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            if(destructing)
                break;
            BlockChunk *chunk = &(*chunkIter);
            if(chunkIter.is_locked())
                chunkIter.unlock();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            for(BlockUpdate *node = removeAllBlockUpdatesInChunk(BlockUpdateKind::Lighting, cbi, lock_manager); node != nullptr; node = removeAllBlockUpdatesInChunk(BlockUpdateKind::Lighting, cbi, lock_manager))
            {
                didAnything = true;
                while(node != nullptr)
                {
                    BlockIterator bi = cbi;
                    bi.moveTo(node->position);
                    Block b = bi.get(lock_manager);
                    if(b.good())
                    {
                        BlockIterator binx = bi;
                        binx.moveTowardNX();
                        BlockIterator bipx = bi;
                        bipx.moveTowardPX();
                        BlockIterator biny = bi;
                        biny.moveTowardNY();
                        BlockIterator bipy = bi;
                        bipy.moveTowardPY();
                        BlockIterator binz = bi;
                        binz.moveTowardNZ();
                        BlockIterator bipz = bi;
                        bipz.moveTowardPZ();
                        Lighting newLighting = b.descriptor->lightProperties.eval(getBlockLighting(binx, lock_manager, false),
                                                                                  getBlockLighting(bipx, lock_manager, false),
                                                                                  getBlockLighting(biny, lock_manager, false),
                                                                                  getBlockLighting(bipy, lock_manager, true),
                                                                                  getBlockLighting(binz, lock_manager, false),
                                                                                  getBlockLighting(bipz, lock_manager, false));
                        if(newLighting != b.lighting)
                        {
                            b.lighting = newLighting;
                            setBlock(bi, lock_manager, b);
                        }
                    }

                    BlockUpdate *deleteMe = node;
                    node = node->chunk_next;
                    if(node != nullptr)
                        node->chunk_prev = nullptr;
                    delete deleteMe;
                }
                if(destructing)
                    break;
            }
        }
        if(!didAnything)
        {
            lightingStable = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void World::generateChunk(BlockChunk *chunk, WorldLockManager &lock_manager)
{
    WorldLockManager new_lock_manager;
    World newWorld(worldGeneratorSeed, nullptr, internal_construct_flag());
    worldGenerator->generateChunk(chunk->basePosition, newWorld, new_lock_manager);
#if 0
    new_lock_manager.clear();
    newWorld.lightingThread = thread([&]()
    {
        newWorld.lightingThreadFn();
    });
    for(int i = 0; i < 100 && !newWorld.isLightingStable() && !destructing; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    newWorld.destructing = true;
#endif

    typedef checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> BlockArrayType;
    typedef checked_array<checked_array<BiomeProperties, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> BiomeArrayType;
    std::unique_ptr<BlockArrayType> blocks(new BlockArrayType);
    std::unique_ptr<BiomeArrayType> biomes(new BiomeArrayType);
    BlockIterator gcbi = newWorld.getBlockIterator(chunk->basePosition);
    for(auto i = BlockChunkRelativePositionIterator::begin(); i != BlockChunkRelativePositionIterator::end(); i++)
    {
        BlockIterator bi = gcbi;
        bi.moveBy(*i);
        blocks->at(i->x)[i->y][i->z] = bi.get(new_lock_manager);
    }
    for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
    {
        for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
        {
            BlockIterator bi = gcbi;
            bi.moveBy(VectorI(dx, 0, dz));
            bi.updateBiomeLock(new_lock_manager);
            biomes->at(dx)[dz].swap(bi.getBiome().biomeProperties);
        }
    }
    BlockIterator cbi = getBlockIterator(chunk->basePosition);
    {
        std::unique_lock<std::recursive_mutex> lockSrcChunk(gcbi.chunk->chunkVariables.entityListLock, std::defer_lock);
        std::unique_lock<std::recursive_mutex> lockDestChunk(cbi.chunk->chunkVariables.entityListLock, std::defer_lock);
        std::lock(lockSrcChunk, lockDestChunk);
        WrappedEntity::ChunkListType &srcChunkList = gcbi.chunk->chunkVariables.entityList;
        WrappedEntity::ChunkListType &destChunkList = cbi.chunk->chunkVariables.entityList;
        for(WrappedEntity::ChunkListType::iterator srcChunkIter = srcChunkList.begin(); srcChunkIter != srcChunkList.end(); )
        {
            BlockChunkSubchunk *srcSubchunk = srcChunkIter->currentSubchunk;
            assert(srcSubchunk);
            new_lock_manager.block_lock.set(srcSubchunk->lock);
            WrappedEntity::SubchunkListType &srcSubchunkList = srcSubchunk->entityList;
            WrappedEntity::SubchunkListType::iterator srcSubchunkIter = srcSubchunkList.to_iterator(&*srcChunkIter);
            if(!srcChunkIter->entity.good())
            {
                srcSubchunkList.erase(srcSubchunkIter);
                srcChunkIter = gcbi.chunk->chunkVariables.entityList.erase(srcChunkIter);
                continue;
            }
            srcChunkIter->lastEntityRunCount = 0;
            PositionF position = srcChunkIter->entity.physicsObject->getPosition();
            VectorF velocity = srcChunkIter->entity.physicsObject->getVelocity();
            srcChunkIter->entity.physicsObject->destroy();
            srcChunkIter->entity.physicsObject = srcChunkIter->entity.descriptor->physicsObjectConstructor->make(position, velocity, physicsWorld);
            WrappedEntity::ChunkListType::iterator nextSrcIter = srcChunkIter;
            ++nextSrcIter;
            BlockIterator destBi = cbi;
            destBi.moveTo((PositionI)position);
            destBi.updateLock(lock_manager);
            WrappedEntity::SubchunkListType &destSubchunkList = destBi.getSubchunk().entityList;
            srcChunkIter->currentChunk = destBi.chunk;
            srcChunkIter->currentSubchunk = &destBi.getSubchunk();
            destChunkList.splice(destChunkList.end(), srcChunkList, srcChunkIter);
            destSubchunkList.splice(destSubchunkList.end(), srcSubchunkList, srcSubchunkIter);
            srcChunkIter = nextSrcIter;
        }
    }
    new_lock_manager.clear();
    for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
    {
        for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
        {
            BlockIterator bi = cbi;
            bi.moveBy(VectorI(dx, 0, dz));
            bi.updateBiomeLock(lock_manager);
            biomes->at(dx)[dz].swap(bi.getBiome().biomeProperties);
        }
    }
    setBlockRange(chunk->basePosition, chunk->basePosition + VectorI(BlockChunk::chunkSizeX - 1, BlockChunk::chunkSizeY - 1, BlockChunk::chunkSizeZ - 1), lock_manager, *blocks, VectorI(0));
}

void World::chunkGeneratingThreadFn()
{
    WorldLockManager lock_manager;
    BlockChunkMap *chunks = &physicsWorld->chunks;
    while(!destructing)
    {
        bool didAnything = false;
        BlockChunk *bestChunk = nullptr;
        bool haveChunk = false;
        float chunkPriority = 0;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            BlockChunk *chunk = &(*chunkIter);
            if(chunk->chunkVariables.generated)
                continue;
            if(chunk->chunkVariables.generateStarted)
                continue;
            if(chunkIter.is_locked())
                chunkIter.unlock();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            float currentChunkPriority = getChunkGeneratePriority(cbi, lock_manager);
            if(std::isnan(currentChunkPriority))
                continue;
            if(!haveChunk || chunkPriority > currentChunkPriority) // low values mean high priority
            {
                haveChunk = true;
                bestChunk = chunk;
                chunkPriority = currentChunkPriority;
            }
        }

        if(haveChunk)
        {
            BlockChunk *chunk = bestChunk;
            if(chunk->chunkVariables.generateStarted.exchange(true)) // someone else got this chunk
                continue;
            getDebugLog() << L"generating " << chunk->basePosition << L" " << chunkPriority << postnl;
            didAnything = true;

            generateChunk(chunk, lock_manager);

            chunk->chunkVariables.generated = true;
        }

        lock_manager.clear();

        if(!didAnything)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

namespace
{
float boxDistanceSquared(VectorF minCorner, VectorF maxCorner, VectorF pos)
{
    VectorF closestPoint(limit(pos.x, minCorner.x, maxCorner.x),
                         limit(pos.y, minCorner.y, maxCorner.y),
                         limit(pos.z, minCorner.z, maxCorner.z));
    return absSquared(closestPoint - pos);
}
}

float World::getChunkGeneratePriority(BlockIterator bi, WorldLockManager &lock_manager) // low values mean high priority, NAN means don't generate
{
    if(bi.position().y != 0)
        return NAN;
    std::unique_lock<std::mutex> lockIt(viewPointsLock);
    float retval;
    bool retvalSet = false;
    PositionF chunkMinCorner = bi.position();
    PositionF chunkMaxCorner = chunkMinCorner + VectorF(BlockChunk::chunkSizeX, BlockChunk::chunkSizeY, BlockChunk::chunkSizeZ);
    PositionF chunkMinCornerXZ = chunkMinCorner;
    PositionF chunkMaxCornerXZ = chunkMaxCorner;
    chunkMinCornerXZ.y = 0;
    chunkMaxCornerXZ.y = 0;
    for(ViewPoint *viewPoint : viewPoints)
    {
        PositionF pos;
        std::int32_t viewDistance;
        viewPoint->getPositionAndViewDistance(pos, viewDistance);
        PositionF posXZ = pos;
        posXZ.y = 0;
        float distSquared = boxDistanceSquared(chunkMinCornerXZ, chunkMaxCornerXZ, posXZ);
        if(pos.d != bi.position().d || distSquared > 2.0f * (float)viewDistance * viewDistance)
            continue;
        if(!retvalSet || retval > distSquared)
        {
            retval = distSquared;
            retvalSet = true;
        }
    }
    if(!retvalSet)
        return NAN;
    return retval;
}

Entity *World::addEntity(EntityDescriptorPointer descriptor, PositionF position, VectorF velocity, WorldLockManager &lock_manager, std::shared_ptr<void> entityData)
{
    assert(descriptor != nullptr);
    BlockIterator bi = getBlockIterator((PositionI)position);
    WrappedEntity *entity = new WrappedEntity(Entity(descriptor, descriptor->physicsObjectConstructor->make(position, velocity, physicsWorld), entityData));
    descriptor->makeData(entity->entity, *this, lock_manager);
    std::unique_lock<std::recursive_mutex> lockChunk(bi.chunk->chunkVariables.entityListLock);
    WrappedEntity::ChunkListType &chunkList = bi.chunk->chunkVariables.entityList;
    bi.updateLock(lock_manager);
    WrappedEntity::SubchunkListType &subchunkList = bi.getSubchunk().entityList;
    entity->currentChunk = bi.chunk;
    entity->currentSubchunk = &bi.getSubchunk();
    chunkList.push_back(entity);
    subchunkList.push_back(entity);
    return &entity->entity;
}

void World::move(double deltaTime, WorldLockManager &lock_manager)
{
    entityRunCount++;
    physicsWorld->stepTime(deltaTime, lock_manager);
    BlockChunkMap *chunks = &physicsWorld->chunks;
    for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
    {
        BlockChunk *chunk = &(*chunkIter);
        if(chunkIter.is_locked())
            chunkIter.unlock();
        BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
        if(chunk->chunkVariables.generated)
        {
            float fRandomTickCount = deltaTime * (20.0f * 3.0f / 16.0f / 16.0f / 16.0f * BlockChunk::chunkSizeX * BlockChunk::chunkSizeY * BlockChunk::chunkSizeZ);
            int randomTickCount = ifloor(fRandomTickCount + std::generate_canonical<float, 20>(getRandomGenerator()));
            for(int i = 0; i < randomTickCount; i++)
            {
                VectorI relativePosition = VectorI(std::uniform_int_distribution<>(0, BlockChunk::chunkSizeX - 1)(getRandomGenerator()),
                                                   std::uniform_int_distribution<>(0, BlockChunk::chunkSizeY - 1)(getRandomGenerator()),
                                                   std::uniform_int_distribution<>(0, BlockChunk::chunkSizeZ - 1)(getRandomGenerator()));
                BlockIterator bi = cbi;
                bi.moveBy(relativePosition);
                Block b = bi.get(lock_manager);
                if(b.good())
                {
                    b.descriptor->randomTick(b, *this, bi, lock_manager);
                }
            }
        }
        std::unique_lock<std::recursive_mutex> lockChunk(chunk->chunkVariables.entityListLock);
        WrappedEntity::ChunkListType &chunkEntityList = chunk->chunkVariables.entityList;
        auto i = chunkEntityList.begin();
        while(i != chunkEntityList.end())
        {
            WrappedEntity &entity = *i;
            if(!entity.entity.good())
            {
                lock_manager.block_lock.set(entity.currentSubchunk->lock);
                WrappedEntity::SubchunkListType &subchunkEntityList = entity.currentSubchunk->entityList;
                subchunkEntityList.erase(subchunkEntityList.to_iterator(&entity));
                i = chunkEntityList.erase(i);
                continue;
            }
            if(entity.lastEntityRunCount >= entityRunCount)
            {
                ++i;
                continue;
            }
            entity.lastEntityRunCount = entityRunCount;
            BlockIterator destBi = cbi;
            destBi.moveTo((PositionI)entity.entity.physicsObject->getPosition());
            entity.entity.descriptor->moveStep(entity.entity, *this, lock_manager, deltaTime);
            if(entity.currentChunk == destBi.chunk && entity.currentSubchunk == &destBi.getSubchunk())
            {
                ++i;
                continue;
            }
            lock_manager.block_lock.set(entity.currentSubchunk->lock);
            WrappedEntity::SubchunkListType &subchunkEntityList = entity.currentSubchunk->entityList;
            subchunkEntityList.detach(subchunkEntityList.to_iterator(&entity));
            destBi.updateLock(lock_manager);
            entity.currentSubchunk = &destBi.getSubchunk();
            WrappedEntity::SubchunkListType &subchunkDestEntityList = entity.currentSubchunk->entityList;
            subchunkDestEntityList.push_back(&entity);
            if(entity.currentChunk == destBi.chunk)
            {
                ++i;
                continue;
            }
            auto nextI = i;
            ++nextI;
            std::unique_lock<std::recursive_mutex> lockChunk(destBi.chunk->chunkVariables.entityListLock);
            WrappedEntity::ChunkListType &chunkDestEntityList = chunk->chunkVariables.entityList;
            chunkDestEntityList.splice(chunkDestEntityList.end(), chunkEntityList, i);
            entity.currentChunk = destBi.chunk;
            i = nextI;
        }
    }
}

RayCasting::Collision World::castRayCheckForEntitiesInSubchunk(BlockIterator bi, RayCasting::Ray ray, WorldLockManager &lock_manager, float maxSearchDistance, const Entity *ignoreEntity)
{
    PositionI subchunkPos = BlockChunk::getSubchunkBaseAbsolutePosition(bi.position());
    RayCasting::Collision retval(*this);
    for(int dx = -1; dx <= 1; dx++)
    {
        for(int dy = -1; dy <= 1; dy++)
        {
            for(int dz = -1; dz <= 1; dz++)
            {
                PositionI currentSubchunkPos = subchunkPos + VectorI(dx * BlockChunk::subchunkSizeXYZ, dy * BlockChunk::subchunkSizeXYZ, dz * BlockChunk::subchunkSizeXYZ);
                bi.moveTo(currentSubchunkPos);
                bi.updateLock(lock_manager);
                for(WrappedEntity &entity : bi.getSubchunk().entityList)
                {
                    if(&entity.entity == ignoreEntity || !entity.entity.good())
                        continue;
                    RayCasting::Collision currentCollision = entity.entity.descriptor->getRayCollision(entity.entity, *this, ray);
                    if(currentCollision < retval)
                        retval = currentCollision;
                }
            }
        }
    }
    if(retval.valid() && retval.t > maxSearchDistance)
        return RayCasting::Collision(*this);
    return retval;
}

RayCasting::Collision World::castRay(RayCasting::Ray ray, WorldLockManager &lock_manager, float maxSearchDistance, RayCasting::BlockCollisionMask blockRayCollisionMask, const Entity *ignoreEntity)
{
    PositionI startPosition = (PositionI)ray.startPosition;
    BlockIterator bi = getBlockIterator(startPosition);
    BlockChunkSubchunk *lastSubchunk = nullptr;
    RayCasting::Collision retval(*this);
    for(auto i = RayCasting::makeRayBlockIterator(ray); std::get<0>(*i) <= maxSearchDistance || std::get<1>(*i) == startPosition; i++)
    {
        bi.moveTo(std::get<1>(*i));
        if(&bi.getSubchunk() != lastSubchunk)
        {
            lastSubchunk = &bi.getSubchunk();
            RayCasting::Collision currentCollision = castRayCheckForEntitiesInSubchunk(bi, ray, lock_manager, maxSearchDistance, ignoreEntity);
            if(currentCollision < retval)
                retval = currentCollision;
        }
        Block b = bi.get(lock_manager);
        if(!b.good())
        {
            if(retval.valid() && retval.t < std::get<0>(*i) && retval.t <= maxSearchDistance)
                return retval;
            return RayCasting::Collision(*this);
        }
        if((b.descriptor->blockRayCollisionMask & blockRayCollisionMask) == 0)
            continue;
        RayCasting::Collision currentCollision = b.descriptor->getRayCollision(b, bi, lock_manager, *this, ray);
        if(currentCollision < retval)
            retval = currentCollision;
        if(retval.valid() && retval.t < maxSearchDistance)
            maxSearchDistance = retval.t;
    }
    if(retval.valid() && retval.t <= maxSearchDistance)
        return retval;
    return RayCasting::Collision(*this);
}
}
}
