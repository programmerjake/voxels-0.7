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
#include "block/builtin/dirt.h"
#include "block/builtin/water.h"
#include <cmath>
#include <random>
#include <limits>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <list>
#include "util/logging.h"
#include "util/global_instance_maker.h"
#include "platform/thread_priority.h"
#include "util/wrapped_entity.h"
#include "generate/decorator.h"
#include "util/decorator_cache.h"
#include "util/wood_descriptor.h"
#include "item/builtin/tools/stone_toolset.h"

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace
{
class BiomesCache final
{
private:
    struct Chunk final
    {
        checked_array<checked_array<BiomeProperties, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> columns;
        std::list<PositionI>::iterator chunksListIterator;
        bool empty = true;
    };
    std::unordered_map<PositionI, Chunk> chunks;
    std::list<PositionI> chunksList;
    void deleteChunk()
    {
        chunks.erase(chunksList.back());
        chunksList.pop_back();
    }
    Chunk &getChunk(PositionI pos, RandomSource &randomSource)
    {
        pos.y = 0;
        if(chunks.size() > 100)
        {
            deleteChunk();
        }
        Chunk &retval = chunks[pos];
        if(retval.empty)
        {
            retval.empty = false;
            for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
            {
                for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
                {
                    retval.columns[dx][dz] = randomSource.getBiomeProperties(pos + VectorI(dx, 0, dz));
                }
            }
            retval.chunksListIterator = chunksList.insert(chunksList.begin(), pos);
        }
        else
        {
            chunksList.splice(chunksList.begin(), chunksList, retval.chunksListIterator);
        }
        return retval;
    }
public:
    void fillWorldChunk(World &world, PositionI chunkBasePosition, WorldLockManager &lock_manager, RandomSource &randomSource)
    {
        Chunk &c = getChunk(chunkBasePosition, randomSource);
        BlockIterator bi = world.getBlockIterator(chunkBasePosition);
        for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
        {
            for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
            {
                bi.moveTo(chunkBasePosition + VectorI(dx, 0, dz));
                world.setBiomeProperties(bi, lock_manager, c.columns[dx][dz]);
            }
        }
    }
};

class GroundChunksCache final
{
private:
    struct Chunk final
    {
        checked_array<checked_array<checked_array<PackedBlock, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> blocks;
        checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> groundHeights;
        std::list<PositionI>::iterator chunksListIterator;
        bool empty = true;
    };
    std::unordered_map<PositionI, Chunk> chunks;
    std::list<PositionI> chunksList;
    void deleteChunk()
    {
        chunks.erase(chunksList.back());
        chunksList.pop_back();
    }
    template <typename T>
    Chunk &getChunk(PositionI pos, T generateFn)
    {
        pos.y = 0;
        if(chunks.size() > 10)
        {
            deleteChunk();
        }
        Chunk &retval = chunks[pos];
        if(retval.empty)
        {
            retval.empty = false;
            static thread_local checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> blocks;
            generateFn(pos, blocks, retval.groundHeights);
            for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
            {
                for(int dy = 0; dy < BlockChunk::chunkSizeY; dy++)
                {
                    for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
                    {
                        retval.blocks[dx][dy][dz] = (PackedBlock)std::move(blocks[dx][dy][dz]);
                        blocks[dx][dy][dz] = Block();
                    }
                }
            }
            retval.chunksListIterator = chunksList.insert(chunksList.begin(), pos);
        }
        else
        {
            chunksList.splice(chunksList.begin(), chunksList, retval.chunksListIterator);
        }
        return retval;
    }
public:
    template <typename T>
    void getChunk(PositionI chunkBasePosition, checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks, checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &groundHeights, T generateFn)
    {
        Chunk &c = getChunk(chunkBasePosition, generateFn);
        for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
        {
            for(int dy = 0; dy < BlockChunk::chunkSizeY; dy++)
            {
                for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
                {
                    blocks[dx][dy][dz] = (Block)c.blocks[dx][dy][dz];
                }
            }
        }
        for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
        {
            for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
            {
                groundHeights[dx][dz] = c.groundHeights[dx][dz];
            }
        }
    }
};

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
private:
    void generateGroundChunkH(checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks,
                             checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &groundHeights,
                             PositionI chunkBasePosition,
                             WorldLockManager &lock_manager, World &world,
                             RandomSource &randomSource) const
    {
        BlockIterator bi = world.getBlockIterator(chunkBasePosition);
        for(int cx = 0; cx < BlockChunk::chunkSizeX; cx++)
        {
            for(int cz = 0; cz < BlockChunk::chunkSizeZ; cz++)
            {
                PositionI columnBasePosition = chunkBasePosition + VectorI(cx, 0, cz);
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
                groundHeights[cx][cz] = groundHeight;
                bp.getDominantBiome()->makeGroundColumn(chunkBasePosition, columnBasePosition, blocks, randomSource, groundHeight);
                for(int cy = 0; cy < BlockChunk::chunkSizeY; cy++)
                {
                    VectorI pos = columnBasePosition + VectorI(0, cy, 0);
                    if(pos.y <= World::SeaLevel)
                    {
                        Block &block = blocks[cx][cy][cz];
                        if(block.descriptor == Blocks::builtin::Air::descriptor())
                        {
                            block = Block(Blocks::builtin::Water::descriptor());
                            block.lighting = Lighting::makeSkyLighting();
                            LightProperties waterLightProperties = Blocks::builtin::Water::descriptor()->lightProperties;
                            for(auto i = pos.y; i <= World::SeaLevel; i++)
                            {
                                block.lighting = waterLightProperties.calculateTransmittedLighting(block.lighting);
                                if(block.lighting == Lighting(0, 0, 0))
                                    break;
                            }
                        }
                        if(cy > 0 && dynamic_cast<const Blocks::builtin::Water *>(block.descriptor) != nullptr)
                        {
                            Block &surfaceBlock = blocks[cx][cy - 1][cz];
                            if(surfaceBlock.descriptor == Blocks::builtin::Grass::descriptor())
                            {
                                surfaceBlock = Block(Blocks::builtin::Dirt::descriptor());
                            }
                        }
                    }
                }
            }
        }
    }
    void generateGroundChunk(checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks,
                             checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &groundHeights,
                             PositionI chunkBasePosition,
                             WorldLockManager &lock_manager, World &world,
                             RandomSource &randomSource) const
    {
        static thread_local std::shared_ptr<BiomesCache> pBiomesCache = nullptr;
        static thread_local std::shared_ptr<GroundChunksCache> pGroundCache = nullptr;
        static thread_local World::SeedType seed = 0;
        if(seed != world.getWorldGeneratorSeed())
            pBiomesCache = nullptr;
        if(pBiomesCache == nullptr)
        {
            pBiomesCache = std::make_shared<BiomesCache>();
            pGroundCache = std::make_shared<GroundChunksCache>();
        }
        pBiomesCache->fillWorldChunk(world, chunkBasePosition, lock_manager, randomSource);
        pGroundCache->getChunk(chunkBasePosition, blocks, groundHeights, [&](PositionI chunkBasePosition, checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks,
                             checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &groundHeights)
        {
            generateGroundChunkH(blocks, groundHeights, chunkBasePosition, lock_manager, world, randomSource);
        });
    }
    std::vector<std::shared_ptr<const DecoratorInstance>> generateDecoratorsInChunk(DecoratorDescriptorPointer descriptor, PositionI chunkBasePosition,
                                 WorldLockManager &lock_manager, World &world,
                                 RandomSource &randomSource) const
    {
        static thread_local checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> blocks;
        static thread_local checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> groundHeights;
        generateGroundChunk(blocks, groundHeights, chunkBasePosition, lock_manager, world, randomSource);
        std::vector<std::shared_ptr<const DecoratorInstance>> retval;
        std::size_t decoratorGenerateNumber = std::hash<DecoratorDescriptorPointer>()(descriptor) + std::hash<PositionI>()(chunkBasePosition);
        std::minstd_rand rg(decoratorGenerateNumber);
        rg.discard(30);
        BlockIterator chunkBaseIterator = world.getBlockIterator(chunkBasePosition);
        for(int x = 0; x < BlockChunk::chunkSizeX; x++)
        {
            for(int z = 0; z < BlockChunk::chunkSizeZ; z++)
            {
                BlockIterator bi = chunkBaseIterator;
                PositionI columnBasePosition = chunkBasePosition + VectorI(x, 0, z);
                bi.moveTo(columnBasePosition);
                const BiomeProperties &bp = bi.getBiomeProperties(lock_manager);
                float generateCountF = 0;
                for(const BiomeWeights::value_type &v : bp.getWeights())
                {
                    BiomeDescriptorPointer biome = std::get<0>(v);
                    float weight = std::get<1>(v);
                    generateCountF += weight * biome->getChunkDecoratorCount(descriptor);
                }
                generateCountF /= BlockChunk::chunkSizeX * BlockChunk::chunkSizeZ;
                int generateCount = ifloor(std::uniform_real_distribution<float>(0, 1)(rg) + generateCountF);
                VectorI relativeColumnSurfacePosition = columnBasePosition + VectorI(0, World::SeaLevel, 0) - chunkBasePosition;
                while(relativeColumnSurfacePosition.y >= 0 && relativeColumnSurfacePosition.y < BlockChunk::chunkSizeY - 1)
                {
                    const Block &b = blocks[relativeColumnSurfacePosition.x][relativeColumnSurfacePosition.y][relativeColumnSurfacePosition.z];
                    if(!b.good())
                        break;
                    if(b.descriptor->isGroundBlock())
                        relativeColumnSurfacePosition.y++;
                    else
                        break;
                }
                while(relativeColumnSurfacePosition.y > 0 && relativeColumnSurfacePosition.y < BlockChunk::chunkSizeY)
                {
                    const Block &b = blocks[relativeColumnSurfacePosition.x][relativeColumnSurfacePosition.y][relativeColumnSurfacePosition.z];
                    if(!b.good())
                        break;
                    if(!b.descriptor->isGroundBlock())
                        relativeColumnSurfacePosition.y--;
                    else
                        break;
                }
                PositionI columnSurfacePosition = relativeColumnSurfacePosition + chunkBasePosition;
                for(int i = 0; i < generateCount; i++)
                {
                    auto instance = descriptor->createInstance(chunkBasePosition, columnBasePosition, columnSurfacePosition,
                                 lock_manager, chunkBaseIterator,
                                 blocks,
                                 randomSource, decoratorGenerateNumber++);
                    if(instance != nullptr)
                        retval.push_back(instance);
                }
            }
        }
        return std::move(retval);
    }
protected:
    virtual void generate(PositionI chunkBasePosition, BlocksArray &blocks, World &world, WorldLockManager &lock_manager, RandomSource &randomSource) const override
    {
        static thread_local std::shared_ptr<DecoratorCache> pDecoratorCache = nullptr;
        static thread_local World::SeedType lastSeed = 0;
        if(lastSeed != world.getWorldGeneratorSeed())
            pDecoratorCache = nullptr;
        if(pDecoratorCache == nullptr)
            pDecoratorCache = std::make_shared<DecoratorCache>();
        DecoratorCache &decoratorCache = *pDecoratorCache;
        checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> groundHeights;
        generateGroundChunk(blocks, groundHeights, chunkBasePosition, lock_manager, world, randomSource);
        for(DecoratorDescriptorPointer descriptor : DecoratorDescriptors)
        {
            int chunkSearchDistance = descriptor->chunkSearchDistance;
            for(VectorI i = VectorI(-chunkSearchDistance, 0, 0); i.x <= chunkSearchDistance; i.x++)
            {
                for(i.z = -chunkSearchDistance; i.z <= chunkSearchDistance; i.z++)
                {
                    VectorI rcpos = VectorI(BlockChunk::chunkSizeX, BlockChunk::chunkSizeY, BlockChunk::chunkSizeZ) * i;
                    PositionI pos = rcpos + chunkBasePosition;
                    auto instances = decoratorCache.getChunkInstances(pos, descriptor);
                    if(!std::get<1>(instances))
                    {
                        decoratorCache.setChunkInstances(pos, descriptor, generateDecoratorsInChunk(descriptor, pos, lock_manager, world, randomSource));
                        instances = decoratorCache.getChunkInstances(pos, descriptor);
                    }
                    for(std::shared_ptr<const DecoratorInstance> instance : std::get<0>(instances))
                    {
                        instance->generateInChunk(chunkBasePosition, lock_manager, world, blocks);
                    }
                }
            }
        }
#ifdef DEBUG_VERSION
        if(chunkBasePosition == PositionI(0, 0, 0, Dimension::Overworld))
        {
            for(int i = 0; i < 2; i++)
            {
                ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::tools::StoneToolset::pointer()->getAxe())), PositionF(0, 80, 0, Dimension::Overworld));
                ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::tools::StoneToolset::pointer()->getHoe())), PositionF(0, 80, 0, Dimension::Overworld));
                ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::tools::StoneToolset::pointer()->getPickaxe())), PositionF(0, 80, 0, Dimension::Overworld));
                ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::tools::StoneToolset::pointer()->getShovel())), PositionF(0, 80, 0, Dimension::Overworld));
            }
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

BlockUpdate *World::removeAllReadyBlockUpdatesInChunk(float deltaTime, BlockIterator bi, WorldLockManager &lock_manager)
{
    BlockChunk *chunk = bi.chunk;
    lock_manager.clear();
    BlockChunkFullLock lockChunk(*chunk);
    auto lockIt = std::unique_lock<decltype(chunk->chunkVariables.blockUpdateListLock)>(chunk->chunkVariables.blockUpdateListLock);
    BlockUpdate *retval = nullptr;
    for(BlockUpdate *node = chunk->chunkVariables.blockUpdateListHead; node != nullptr;)
    {
        if(node->kind == BlockUpdateKind::Lighting)
        {
            node = node->chunk_next;
            continue;
        }
        node->time_left -= deltaTime;
        if(node->time_left <= 0)
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
    WorldLockManager lock_manager;
    for(;;)
    {
        RandomSource randomSource(getWorldGeneratorSeed());
        BiomeProperties bp = randomSource.getBiomeProperties(PositionI(0, 0, 0, Dimension::Overworld));
        if(!bp.getDominantBiome()->isGoodStartingPosition())
        {
            auto &rg = getRandomGenerator();
            worldGeneratorSeed = rg();
            continue;
        }
        break;
    }
    lightingThread = thread([this]()
    {
        lightingThreadFn();
    });
    for(int dx = 0; dx >= -1; dx--)
    {
        for(int dz = 0; dz >= -1; dz--)
        {
            BlockChunk *initialChunk = getBlockIterator(PositionI(dx * BlockChunk::chunkSizeX, 0, dz * BlockChunk::chunkSizeZ, Dimension::Overworld)).chunk;
            initialChunk->chunkVariables.generateStarted = true;
            getDebugLog() << L"generating " << initialChunk->basePosition << postnl;
            generateChunk(initialChunk, lock_manager);
            initialChunk->chunkVariables.generated = true;
        }
    }
    blockUpdateThread = thread([this]()
    {
        blockUpdateThreadFn();
    });
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
    if(blockUpdateThread.joinable())
        blockUpdateThread.join();
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

void World::blockUpdateThreadFn()
{
    setThreadPriority(ThreadPriority::Low);
    WorldLockManager lock_manager;
    BlockChunkMap *chunks = &physicsWorld->chunks;
    auto lastUpdateTime = std::chrono::steady_clock::now();
    while(!destructing)
    {
        bool didAnything = false;
        auto currentUpdateTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentUpdateTime - lastUpdateTime).count();
        lastUpdateTime = currentUpdateTime;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            if(destructing)
                break;
            BlockChunk *chunk = &(*chunkIter);
            if(chunkIter.is_locked())
                chunkIter.unlock();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            for(BlockUpdate *node = removeAllReadyBlockUpdatesInChunk(deltaTime, cbi, lock_manager); node != nullptr; node = removeAllReadyBlockUpdatesInChunk(0, cbi, lock_manager))
            {
                didAnything = true;
                while(node != nullptr)
                {
                    BlockIterator bi = cbi;
                    bi.moveTo(node->position);
                    Block b = bi.get(lock_manager);
                    if(b.good())
                    {
                        BlockUpdateSet blockUpdateSet;
                        b.descriptor->tick(blockUpdateSet, *this, b, bi, lock_manager, node->kind);
                        for(BlockUpdateSet::value_type v : blockUpdateSet)
                        {
                            bi = cbi;
                            bi.moveTo(std::get<0>(v));
                            setBlock(bi, lock_manager, std::get<1>(v));
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
            new_lock_manager.block_biome_lock.set(srcSubchunk->lock);
            WrappedEntity::SubchunkListType &srcSubchunkList = srcSubchunk->entityList;
            WrappedEntity::SubchunkListType::iterator srcSubchunkIter = srcSubchunkList.to_iterator(&*srcChunkIter);
            if(!srcChunkIter->entity.good())
            {
                srcSubchunkList.erase(srcSubchunkIter);
                srcChunkIter = srcChunkList.erase(srcChunkIter);
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
                lock_manager.block_biome_lock.set(entity.currentSubchunk->lock);
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
            lock_manager.block_biome_lock.set(entity.currentSubchunk->lock);
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
