/*
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
        for(auto i = BlockChunkRelativePositionIterator::begin(); i != BlockChunkRelativePositionIterator::end(); i++)
        {
            PositionI pos = chunkBasePosition + *i;
            PositionF fpos = pos + VectorF(0.5f);
            Block block;
            if(fpos.y - World::AverageGroundHeight < 3 * randomSource.getFBMValue(fpos * 0.1f))
                block = Block(Blocks::builtin::Stone::descriptor());
            else
            {
                block = Block(Blocks::builtin::Air::descriptor());
                block.lighting = Lighting::makeSkyLighting();
            }
            blocks[i->x][i->y][i->z] = block;
        }
#if 0
        constexpr float newEntityHeight = 3.5f;
        PositionF epos = VectorF(0.5f, newEntityHeight, 0.5f) + chunkBasePosition * VectorF(0, 0, 0);
        if(BlockChunk::getChunkBasePosition((PositionI)epos) == chunkBasePosition)
        {
            world.addEntity(Entities::builtin::items::Stone::descriptor(), epos);
            world.addEntity(Entities::builtin::items::Stone::descriptor(), epos + VectorF(0, 1, 0));
        }
#endif
    }
};
}

World::World(SeedType seed)
    : World(seed, MyWorldGenerator::getInstance())
{

}

namespace
{
World::SeedType makeSeed(wstring seed)
{
    World::SeedType retval = 0;
    for(wchar_t ch : seed)
    {
        retval *= 8191;
        retval += ch;
    }
    return retval;
}
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
    : physicsWorld(std::make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed), destructing(false), lightingStable(false)
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
    : physicsWorld(std::make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed), destructing(false), lightingStable(false)
{
    const int initSize = 5;
    for(int dx = -initSize; dx <= initSize; dx++)
    {
        for(int dz = -initSize; dz <= initSize; dz++)
        {
            getBlockIterator(PositionI(dx * BlockChunk::chunkSizeX, 0, dz * BlockChunk::chunkSizeZ, Dimension::Overworld)); // create chunk
        }
    }
    lightingThread = thread([this]()
    {
        lightingThreadFn();
    });
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
                std::unique_ptr<BlockArrayType> blocks(new BlockArrayType);
                BlockIterator cbi = newWorld.getBlockIterator(chunk->basePosition);
                for(auto i = BlockChunkRelativePositionIterator::begin(); i != BlockChunkRelativePositionIterator::end(); i++)
                {
                    BlockIterator bi = cbi;
                    bi.moveBy(*i);
                    blocks->at(i->x)[i->y][i->z] = bi.get(new_lock_manager);
                }
                new_lock_manager.clear();
                setBlockRange(chunk->basePosition, chunk->basePosition + VectorI(BlockChunk::chunkSizeX - 1, BlockChunk::chunkSizeY - 1, BlockChunk::chunkSizeZ - 1), lock_manager, *blocks, VectorI(0));
            }

            chunk->chunkVariables.generated = true;
        }

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
        if(pos.d != bi.position().d || distSquared > 1.2f * (float)viewDistance * viewDistance)
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
}
}
