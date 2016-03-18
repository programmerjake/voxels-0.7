/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "world/world.h"
#include "generate/random_world_generator.h"
#include "world/view_point.h"
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
#include "platform/thread_name.h"
#include "platform/thread_priority.h"
#include "util/wrapped_entity.h"
#include "generate/decorator.h"
#include "util/decorator_cache.h"
#include "util/wood_descriptor.h"
#include "item/builtin/tools/mineral_toolsets.h"
#include "item/builtin/minerals.h"
#include "item/builtin/ores.h"
#include "item/builtin/cobblestone.h"
#include "item/builtin/furnace.h"
#include "item/builtin/bucket.h"
#include "util/util.h"
#include "stream/compressed_stream.h"
#include "util/game_version.h"
#include "player/player.h"
#include "util/chunk_cache.h"
#include "util/tls.h"
#include "util/usage_monitor.h"

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace
{
std::size_t roundUpThreadCount(std::size_t v)
{
    if(v == 0)
        return 1;
    return v;
}

struct ThreadCounts final
{
    const std::size_t lightingThreadCount = roundUpThreadCount(getProcessorCount() / 2);
    const std::size_t blockUpdateThreadCount = roundUpThreadCount(getProcessorCount() / 2);
#if 1 || defined(NDEBUG)
    const std::size_t generateThreadCount = roundUpThreadCount(getProcessorCount() / 2);
#else
    const std::size_t generateThreadCount = 1;
#endif
    static const ThreadCounts &get()
    {
        static ThreadCounts retval;
        return retval;
    }
};

class BiomesCache final
{
private:
    struct Chunk final
    {
        checked_array<checked_array<BiomeProperties, BlockChunk::chunkSizeZ>,
                      BlockChunk::chunkSizeX> columns;
        std::list<PositionI>::iterator chunksListIterator;
        bool empty = true;
        Chunk() : columns(), chunksListIterator()
        {
        }
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
        if(chunks.size() > 50)
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
                    retval.columns[dx][dz] =
                        randomSource.getBiomeProperties(pos + VectorI(dx, 0, dz));
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
    BiomesCache() : chunks(), chunksList()
    {
    }
    void fillWorldChunk(World &world,
                        PositionI chunkBasePosition,
                        WorldLockManager &lock_manager,
                        RandomSource &randomSource)
    {
        Chunk &c = getChunk(chunkBasePosition, randomSource);
        world.setBiomePropertiesRange<false>(
            chunkBasePosition,
            chunkBasePosition + VectorI(BlockChunk::chunkSizeX - 1,
                                        BlockChunk::chunkSizeY - 1,
                                        BlockChunk::chunkSizeZ - 1),
            lock_manager,
            c.columns,
            VectorI(0, 0, 0));
    }
};

class GroundChunksCache final
{
private:
    struct Chunk final
    {
        GenericBlocksGenerateArray<PackedBlock> blocks;
        checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX>
            groundHeights;
        std::list<PositionI>::iterator chunksListIterator;
        bool empty = true;
        Chunk() : blocks(), groundHeights(), chunksListIterator()
        {
        }
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
        if(chunks.size() > 20)
        {
            deleteChunk();
        }
        Chunk &retval = chunks[pos];
        if(retval.empty)
        {
            retval.empty = false;
            struct BlocksTLSTag
            {
            };
            thread_local_variable<BlocksGenerateArray, BlocksTLSTag> blocks(TLS::getSlow());
            generateFn(pos, blocks.get(), retval.groundHeights);
            for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
            {
                for(int dy = 0; dy < BlockChunk::chunkSizeY; dy++)
                {
                    for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
                    {
                        retval.blocks[dx][dy][dz] =
                            static_cast<PackedBlock>(std::move(blocks.get()[dx][dy][dz]));
                        // blocks[dx][dy][dz] = Block();
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
    GroundChunksCache() : chunks(), chunksList()
    {
    }
    template <typename T>
    void getChunk(PositionI chunkBasePosition,
                  BlocksGenerateArray &blocks,
                  checked_array<checked_array<int, BlockChunk::chunkSizeZ>,
                                BlockChunk::chunkSizeX> &groundHeights,
                  T generateFn)
    {
        Chunk &c = getChunk(chunkBasePosition, generateFn);
        for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
        {
            for(int dy = 0; dy < BlockChunk::chunkSizeY; dy++)
            {
                for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
                {
                    blocks[dx][dy][dz] = static_cast<Block>(c.blocks[dx][dy][dz]);
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
    void generateGroundChunkH(BlocksGenerateArray &blocks,
                              checked_array<checked_array<int, BlockChunk::chunkSizeZ>,
                                            BlockChunk::chunkSizeX> &groundHeights,
                              PositionI chunkBasePosition,
                              WorldLockManager &lock_manager,
                              World &world,
                              RandomSource &randomSource,
                              const std::atomic_bool *abortFlag) const
    {
        BlockIterator bi = world.getBlockIterator(chunkBasePosition, lock_manager.tls);
        for(int cx = 0; cx < BlockChunk::chunkSizeX; cx++)
        {
            for(int cz = 0; cz < BlockChunk::chunkSizeZ; cz++)
            {
                checkForAbort(abortFlag);
                PositionI columnBasePosition = chunkBasePosition + VectorI(cx, 0, cz);
                bi.moveTo(columnBasePosition, lock_manager.tls);
                const BiomeProperties &bp = bi.getBiomeProperties(lock_manager);
                float groundHeightF = 0;
                for(const BiomeWeights::value_type &v : bp.getWeights())
                {
                    BiomeDescriptorPointer biome = std::get<0>(v);
                    float weight = std::get<1>(v);
                    groundHeightF +=
                        weight * biome->getGroundHeight(columnBasePosition, randomSource);
                }
                int groundHeight = ifloor(0.5f + groundHeightF);
                groundHeights[cx][cz] = groundHeight;
                bp.getDominantBiome()->makeGroundColumn(
                    chunkBasePosition, columnBasePosition, blocks, randomSource, groundHeight);
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
                            LightProperties waterLightProperties =
                                Blocks::builtin::Water::descriptor()->lightProperties;
                            for(auto i = pos.y; i <= World::SeaLevel; i++)
                            {
                                block.lighting = waterLightProperties.calculateTransmittedLighting(
                                    block.lighting);
                                if(block.lighting == Lighting(0, 0, 0))
                                    break;
                            }
                        }
                        if(cy > 0
                           && dynamic_cast<const Blocks::builtin::Water *>(block.descriptor)
                                  != nullptr)
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
    void generateGroundChunk(BlocksGenerateArray &blocks,
                             checked_array<checked_array<int, BlockChunk::chunkSizeZ>,
                                           BlockChunk::chunkSizeX> &groundHeights,
                             PositionI chunkBasePosition,
                             WorldLockManager &lock_manager,
                             World &world,
                             RandomSource &randomSource,
                             const std::atomic_bool *abortFlag) const
    {
        struct PBiomesCacheTag
        {
        };
        thread_local_variable<std::shared_ptr<BiomesCache>, PBiomesCacheTag> pBiomesCache(
            lock_manager.tls, nullptr);
        struct PGroundCacheTag
        {
        };
        thread_local_variable<std::shared_ptr<GroundChunksCache>, PGroundCacheTag> pGroundCache(
            lock_manager.tls, nullptr);
        struct SeedTag
        {
        };
        thread_local_variable<World::SeedType, SeedTag> seed(lock_manager.tls, 0);
        if(seed.get() != world.getWorldGeneratorSeed())
            pBiomesCache.get() = nullptr;
        if(pBiomesCache.get() == nullptr)
        {
            pBiomesCache.get() = std::make_shared<BiomesCache>();
            pGroundCache.get() = std::make_shared<GroundChunksCache>();
            seed.get() = world.getWorldGeneratorSeed();
        }
        pBiomesCache.get()->fillWorldChunk(world, chunkBasePosition, lock_manager, randomSource);
        checkForAbort(abortFlag);
        pGroundCache.get()->getChunk(chunkBasePosition,
                                     blocks,
                                     groundHeights,
                                     [&](PositionI chunkBasePosition,
                                         BlocksGenerateArray &blocks,
                                         checked_array<checked_array<int, BlockChunk::chunkSizeZ>,
                                                       BlockChunk::chunkSizeX> &groundHeights)
                                     {
                                         generateGroundChunkH(blocks,
                                                              groundHeights,
                                                              chunkBasePosition,
                                                              lock_manager,
                                                              world,
                                                              randomSource,
                                                              abortFlag);
                                     });
    }
    std::vector<std::shared_ptr<const DecoratorInstance>> generateDecoratorsInChunk(
        DecoratorDescriptorPointer descriptor,
        PositionI chunkBasePosition,
        WorldLockManager &lock_manager,
        World &world,
        RandomSource &randomSource,
        const std::atomic_bool *abortFlag) const
    {
        struct PBlocksTag
        {
        };
        thread_local_variable<std::unique_ptr<BlocksGenerateArray>, PBlocksTag> pBlocks(
            lock_manager.tls);
        if(!pBlocks.get())
            pBlocks.get().reset(new BlocksGenerateArray);
        BlocksGenerateArray &blocks = *pBlocks.get();
        typedef checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX>
            GroundHeightsType;
        struct PGroundHeightsTag
        {
        };
        thread_local_variable<std::unique_ptr<GroundHeightsType>, PGroundHeightsTag> pGroundHeights(
            lock_manager.tls);
        if(!pGroundHeights.get())
            pGroundHeights.get().reset(new GroundHeightsType);
        GroundHeightsType &groundHeights = *pGroundHeights.get();
        generateGroundChunk(
            blocks, groundHeights, chunkBasePosition, lock_manager, world, randomSource, abortFlag);
        std::vector<std::shared_ptr<const DecoratorInstance>> retval;
        std::uint32_t decoratorGenerateNumber =
            descriptor->getInitialDecoratorGenerateNumber()
            + (std::uint32_t)std::hash<PositionI>()(chunkBasePosition);
        std::minstd_rand rg(decoratorGenerateNumber);
        rg.discard(30);
        BlockIterator chunkBaseIterator =
            world.getBlockIterator(chunkBasePosition, lock_manager.tls);
        for(int x = 0; x < BlockChunk::chunkSizeX; x++)
        {
            for(int z = 0; z < BlockChunk::chunkSizeZ; z++)
            {
                checkForAbort(abortFlag);
                BlockIterator bi = chunkBaseIterator;
                PositionI columnBasePosition = chunkBasePosition + VectorI(x, 0, z);
                bi.moveTo(columnBasePosition, lock_manager.tls);
                const BiomeProperties &bp = bi.getBiomeProperties(lock_manager);
                float generateCountF = 0;
                for(const BiomeWeights::value_type &v : bp.getWeights())
                {
                    BiomeDescriptorPointer biome = std::get<0>(v);
                    float weight = std::get<1>(v);
                    generateCountF += weight * biome->getChunkDecoratorCount(descriptor);
                }
                generateCountF /= BlockChunk::chunkSizeX * BlockChunk::chunkSizeZ;
                int generateCount =
                    ifloor(std::generate_canonical<float, 1000>(rg) + generateCountF);
                VectorI relativeColumnSurfacePosition =
                    columnBasePosition + VectorI(0, World::SeaLevel, 0) - chunkBasePosition;
                while(relativeColumnSurfacePosition.y >= 0
                      && relativeColumnSurfacePosition.y < BlockChunk::chunkSizeY - 1)
                {
                    const Block &b =
                        blocks[relativeColumnSurfacePosition.x][relativeColumnSurfacePosition.y]
                              [relativeColumnSurfacePosition.z];
                    if(!b.good())
                        break;
                    if(b.descriptor->isGroundBlock())
                        relativeColumnSurfacePosition.y++;
                    else
                        break;
                }
                while(relativeColumnSurfacePosition.y > 0
                      && relativeColumnSurfacePosition.y < BlockChunk::chunkSizeY)
                {
                    const Block &b =
                        blocks[relativeColumnSurfacePosition.x][relativeColumnSurfacePosition.y]
                              [relativeColumnSurfacePosition.z];
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
                    auto instance = descriptor->createInstance(chunkBasePosition,
                                                               columnBasePosition,
                                                               columnSurfacePosition,
                                                               lock_manager,
                                                               chunkBaseIterator,
                                                               blocks,
                                                               randomSource,
                                                               decoratorGenerateNumber++);
                    if(instance != nullptr)
                        retval.push_back(instance);
                }
            }
        }
        return std::move(retval);
    }

protected:
    virtual void generate(PositionI chunkBasePosition,
                          BlocksGenerateArray &blocks,
                          World &world,
                          WorldLockManager &lock_manager,
                          RandomSource &randomSource,
                          const std::atomic_bool *abortFlag) const override
    {
        struct PDecoratorCacheTag
        {
        };
        thread_local_variable<std::shared_ptr<DecoratorCache>, PDecoratorCacheTag> pDecoratorCache(
            lock_manager.tls, nullptr);
        struct LastSeedTag
        {
        };
        thread_local_variable<World::SeedType, LastSeedTag> lastSeed(lock_manager.tls, 0);
        if(lastSeed.get() != world.getWorldGeneratorSeed())
            pDecoratorCache.get() = nullptr;
        if(pDecoratorCache.get() == nullptr)
        {
            pDecoratorCache.get() = std::make_shared<DecoratorCache>();
            lastSeed.get() = world.getWorldGeneratorSeed();
        }
        DecoratorCache &decoratorCache = *pDecoratorCache.get();
        typedef checked_array<checked_array<int, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX>
            GroundHeightsType;
        struct PGroundHeightsTag
        {
        };
        thread_local_variable<std::unique_ptr<GroundHeightsType>, PGroundHeightsTag> pGroundHeights(
            lock_manager.tls);
        if(!pGroundHeights.get())
            pGroundHeights.get().reset(new GroundHeightsType);
        GroundHeightsType &groundHeights = *pGroundHeights.get();
        generateGroundChunk(
            blocks, groundHeights, chunkBasePosition, lock_manager, world, randomSource, abortFlag);
        for(DecoratorDescriptorPointer descriptor : DecoratorDescriptors)
        {
            int chunkSearchDistance = descriptor->chunkSearchDistance;
            for(VectorI i = VectorI(-chunkSearchDistance, 0, 0); i.x <= chunkSearchDistance; i.x++)
            {
                for(i.z = -chunkSearchDistance; i.z <= chunkSearchDistance; i.z++)
                {
                    checkForAbort(abortFlag);
                    VectorI rcpos = VectorI(BlockChunk::chunkSizeX,
                                            BlockChunk::chunkSizeY,
                                            BlockChunk::chunkSizeZ) * i;
                    PositionI pos = rcpos + chunkBasePosition;
                    auto instances = decoratorCache.getChunkInstances(pos, descriptor);
                    if(!std::get<1>(instances))
                    {
                        decoratorCache.setChunkInstances(
                            pos,
                            descriptor,
                            generateDecoratorsInChunk(
                                descriptor, pos, lock_manager, world, randomSource, abortFlag));
                        instances = decoratorCache.getChunkInstances(pos, descriptor);
                    }
                    for(std::shared_ptr<const DecoratorInstance> instance : std::get<0>(instances))
                    {
                        checkForAbort(abortFlag);
                        instance->generateInChunk(chunkBasePosition, lock_manager, world, blocks);
                    }
                }
            }
        }
#ifdef DEBUG_VERSION
#if 0
        for(int x = 0; x < BlockChunk::chunkSizeX; x++)
        {
            for(int z = 0; z < BlockChunk::chunkSizeZ; z++)
            {
                bool canReset = false;
                for(int y = BlockChunk::chunkSizeY - 1; y >= 0; y--)
                {
                    Block &b = blocks[x][y][z];
                    if(b.descriptor == Blocks::builtin::Stone::descriptor() && canReset)
                    {
                        b = Block(Blocks::builtin::Air::descriptor());
                        continue;
                    }
                    canReset = true;
                    if(b.descriptor != Blocks::builtin::Stone::descriptor())
                        canReset = false;
                }
            }
        }
#endif
#endif
    }
};
}

World::World(SeedType seed, std::atomic_bool *abortFlag)
    : World(seed, MyWorldGenerator::getInstance(), abortFlag)
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
    : randomGenerator(seed),
      randomGeneratorLock(),
      wrng(*this),
      physicsWorld(std::make_shared<PhysicsWorld>()),
      worldGenerator(worldGenerator),
      worldGeneratorSeed(seed),
      lightingThreads(),
      blockUpdateThreads(),
      particleGeneratingThread(),
      moveEntitiesThread(),
      chunkGeneratingThreads(),
      destructing(false),
      lightingStable(false),
      viewPointsLock(),
      viewPoints(),
      timeOfDayLock(),
      playerList(std::shared_ptr<PlayerList>(new PlayerList())),
      stateLock(),
      stateCond(),
      chunkUnloaderThread(),
      blockUpdateCurrentPhase(BlockUpdatePhase::InitialPhase),
      blockUpdateCurrentPhaseCount(ThreadCounts::get().blockUpdateThreadCount),
      blockUpdateNextPhaseCount(0),
      blockUpdateDidAnything(false)
{
}

World::World(std::atomic_bool *abortFlag) : World(makeRandomSeed(), abortFlag)
{
}

BlockUpdate *World::removeAllBlockUpdatesInChunk(BlockUpdateKind kind,
                                                 BlockIterator bi,
                                                 WorldLockManager &lock_manager)
{
    std::shared_ptr<BlockChunk> chunk = bi.chunk;
    lock_manager.clear();
    BlockChunkFullLock lockChunk(*chunk);
    auto lockIt = std::unique_lock<decltype(chunk->getChunkVariables().blockUpdateListLock)>(
        chunk->getChunkVariables().blockUpdateListLock);
    BlockUpdate *retval = nullptr;
    std::size_t &blockUpdatesLeft =
        bi.chunk->getChunkVariables().blockUpdatesPerPhase[BlockUpdateKindPhase(kind)];
    for(BlockUpdate *node = chunk->getChunkVariables().blockUpdateListHead; node != nullptr;)
    {
        if(node->kind == kind)
        {
            if(node->chunk_prev != nullptr)
                node->chunk_prev->chunk_next = node->chunk_next;
            else
                chunk->getChunkVariables().blockUpdateListHead = node->chunk_next;
            if(node->chunk_next != nullptr)
                node->chunk_next->chunk_prev = node->chunk_prev;
            else
                chunk->getChunkVariables().blockUpdateListTail = node->chunk_prev;
            BlockUpdate *next_node = node->chunk_next;
            node->chunk_prev = nullptr;
            node->chunk_next = retval;
            if(retval != nullptr)
                retval->chunk_prev = node;
            retval = node;
            blockUpdatesLeft--;
            node = next_node;
            VectorI subchunkIndex = BlockChunk::getSubchunkIndexFromPosition(retval->position);
            VectorI subchunkRelativePosition =
                BlockChunk::getSubchunkRelativePosition(retval->position);
            VectorI chunkRelativePosition = BlockChunk::getChunkRelativePosition(retval->position);
            BlockChunkSubchunk &subchunk =
                chunk->subchunks[subchunkIndex.x][subchunkIndex.y][subchunkIndex.z];
            BlockChunkBlock &block =
                chunk->blocks[chunkRelativePosition.x][chunkRelativePosition
                                                           .y][chunkRelativePosition.z];
            BlockOptionalData *blockOptionalData = nullptr;
            if(block.hasOptionalData)
                blockOptionalData = subchunk.blockOptionalData.get(subchunkRelativePosition);
            if(blockOptionalData != nullptr)
            {
                for(BlockUpdate **pnode = &blockOptionalData->updateListHead; *pnode != nullptr;
                    pnode = &(*pnode)->block_next)
                {
                    if(*pnode == retval)
                    {
                        *pnode = retval->block_next;
                        break;
                    }
                }
                if(blockOptionalData->empty())
                {
                    subchunk.blockOptionalData.erase(subchunkRelativePosition, lock_manager.tls);
                    block.hasOptionalData = false;
                }
            }
            retval->block_next = nullptr;
        }
        else
            node = node->chunk_next;
    }
    return retval;
}

BlockUpdate *World::removeAllReadyBlockUpdatesInChunk(BlockUpdatePhase phase,
                                                      BlockIterator bi,
                                                      WorldLockManager &lock_manager)
{
    std::shared_ptr<BlockChunk> chunk = bi.chunk;
    lock_manager.clear();
    BlockChunkFullLock lockChunk(*chunk);
    auto lockIt = std::unique_lock<decltype(chunk->getChunkVariables().blockUpdateListLock)>(
        chunk->getChunkVariables().blockUpdateListLock);
    float deltaTime = 0;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if(chunk->getChunkVariables().lastBlockUpdateTimeValid)
        deltaTime =
            static_cast<float>(std::chrono::duration_cast<std::chrono::duration<double>>(
                                   now - chunk->getChunkVariables().lastBlockUpdateTime).count());
    chunk->getChunkVariables().lastBlockUpdateTimeValid = true;
    chunk->getChunkVariables().lastBlockUpdateTime = now;
    BlockUpdate *retval = nullptr;
    std::size_t &blockUpdatesLeft = bi.chunk->getChunkVariables().blockUpdatesPerPhase[phase];
    for(BlockUpdate *node = chunk->getChunkVariables().blockUpdateListHead; node != nullptr;)
    {
        if(node->kind == BlockUpdateKind::Lighting)
        {
            node = node->chunk_next;
            continue;
        }
        auto nodePhase = BlockUpdateKindPhase(node->kind);
        if(nodePhase != BlockUpdatePhase::Asynchronous && nodePhase != phase)
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
                chunk->getChunkVariables().blockUpdateListHead = node->chunk_next;
            if(node->chunk_next != nullptr)
                node->chunk_next->chunk_prev = node->chunk_prev;
            else
                chunk->getChunkVariables().blockUpdateListTail = node->chunk_prev;
            BlockUpdate *next_node = node->chunk_next;
            node->chunk_prev = nullptr;
            node->chunk_next = retval;
            if(retval != nullptr)
                retval->chunk_prev = node;
            retval = node;
            node = next_node;
            blockUpdatesLeft--;
            VectorI subchunkIndex = BlockChunk::getSubchunkIndexFromPosition(retval->position);
            VectorI subchunkRelativePosition =
                BlockChunk::getSubchunkRelativePosition(retval->position);
            VectorI chunkRelativePosition = BlockChunk::getChunkRelativePosition(retval->position);
            BlockChunkSubchunk &subchunk =
                chunk->subchunks[subchunkIndex.x][subchunkIndex.y][subchunkIndex.z];
            BlockChunkBlock &block =
                chunk->blocks[chunkRelativePosition.x][chunkRelativePosition
                                                           .y][chunkRelativePosition.z];
            BlockOptionalData *blockOptionalData = nullptr;
            if(block.hasOptionalData)
                blockOptionalData = subchunk.blockOptionalData.get(subchunkRelativePosition);
            if(blockOptionalData != nullptr)
            {
                for(BlockUpdate **pnode = &blockOptionalData->updateListHead; *pnode != nullptr;
                    pnode = &(*pnode)->block_next)
                {
                    if(*pnode == retval)
                    {
                        *pnode = retval->block_next;
                        break;
                    }
                }
                if(blockOptionalData->empty())
                {
                    subchunk.blockOptionalData.erase(subchunkRelativePosition, lock_manager.tls);
                    block.hasOptionalData = false;
                }
            }
            retval->block_next = nullptr;
        }
        else
            node = node->chunk_next;
    }
    return retval;
}

World::World(SeedType seed, const WorldGenerator *worldGenerator, std::atomic_bool *abortFlag)
    : randomGenerator(seed),
      randomGeneratorLock(),
      wrng(*this),
      physicsWorld(std::make_shared<PhysicsWorld>()),
      worldGenerator(worldGenerator),
      worldGeneratorSeed(seed),
      lightingThreads(),
      blockUpdateThreads(),
      particleGeneratingThread(),
      moveEntitiesThread(),
      chunkGeneratingThreads(),
      destructing(false),
      lightingStable(false),
      viewPointsLock(),
      viewPoints(),
      timeOfDayLock(),
      playerList(std::shared_ptr<PlayerList>(new PlayerList())),
      stateLock(),
      stateCond(),
      chunkUnloaderThread(),
      blockUpdateCurrentPhase(BlockUpdatePhase::InitialPhase),
      blockUpdateCurrentPhaseCount(ThreadCounts::get().blockUpdateThreadCount),
      blockUpdateNextPhaseCount(0),
      blockUpdateDidAnything(false)
{
    TLS &tls = TLS::getSlow();
    ([this, abortFlag, &tls]()
     {
         getDebugLog() << L"generating initial world..." << postnl;
         for(;;)
         {
             RandomSource randomSource(getWorldGeneratorSeed());
             BiomeProperties bp =
                 randomSource.getBiomeProperties(PositionI(0, 0, 0, Dimension::Overworld));
             if(!bp.getDominantBiome()->isGoodStartingPosition())
             {
                 auto &rg = getRandomGenerator();
                 worldGeneratorSeed = rg();
                 if(abortFlag && *abortFlag)
                 {
                     destructing = true;
                     return;
                 }
                 continue;
             }
             break;
         }
         if(abortFlag && *abortFlag)
         {
             destructing = true;
             return;
         }
         chunkUnloaderThread = thread([this]()
                                      {
                                          setThreadName(L"chunk unloader");
                                          TLS tls;
                                          chunkUnloaderThreadFn(tls);
                                      });
         for(std::size_t i = 0; i < ThreadCounts::get().lightingThreadCount; i++)
         {
             lightingThreads.emplace_back([this]()
                                          {
                                              setThreadName(L"lighting");
                                              TLS tls;
                                              lightingThreadFn(tls);
                                          });
         }
         std::size_t initialChunkGenerateCount = 0;
         for(PositionI position(
                 -BlockChunk::chunkSizeX, 0, -BlockChunk::chunkSizeZ, Dimension::Overworld);
             position.z <= 0;
             position.z += BlockChunk::chunkSizeZ)
         {
             for(position.x = -BlockChunk::chunkSizeX; position.x <= 0;
                 position.x += BlockChunk::chunkSizeX)
             {
                 getBlockIterator(position, tls); // adds chunk to chunks to look through
                 initialChunkGenerateCount++;
             }
         }
         std::shared_ptr<InitialChunkGenerateStruct> initialChunkGenerateStruct =
             std::make_shared<InitialChunkGenerateStruct>(initialChunkGenerateCount, abortFlag);
         for(std::size_t i = 0; i < ThreadCounts::get().generateThreadCount; i++)
         {
             chunkGeneratingThreads.emplace_back(
                 [this, initialChunkGenerateStruct]()
                 {
                     setThreadName(L"chunk generate");
                     setThreadPriority(ThreadPriority::Low);
                     TLS tls;
                     chunkGeneratingThreadFn(std::move(initialChunkGenerateStruct), tls);
                 });
         }
         {
             std::unique_lock<std::mutex> lockIt(initialChunkGenerateStruct->lock);
             while(initialChunkGenerateStruct->count > 0)
             {
                 if(abortFlag && *abortFlag)
                 {
                     destructing = true;
                     initialChunkGenerateStruct->generatorWait = false;
                     initialChunkGenerateStruct->generatorWaitDoneCond.notify_all();
                     return;
                 }
                 initialChunkGenerateStruct->initialGenerationDoneCond.wait(lockIt);
             }
         }
         if(abortFlag && *abortFlag)
         {
             destructing = true;
             std::unique_lock<std::mutex> lockIt(initialChunkGenerateStruct->lock);
             initialChunkGenerateStruct->generatorWait = false;
             initialChunkGenerateStruct->generatorWaitDoneCond.notify_all();
             return;
         }
         for(std::size_t i = 0; i < ThreadCounts::get().blockUpdateThreadCount; i++)
         {
             bool isPhaseManager = i == 0;
             blockUpdateThreads.emplace_back([this, isPhaseManager]()
                                             {
                                                 setThreadName(L"block update");
                                                 TLS tls;
                                                 blockUpdateThreadFn(tls, isPhaseManager);
                                             });
         }
         getDebugLog() << L"lighting initial world..." << postnl;
         for(int i = 0; i < 500 && !isLightingStable(); i++)
         {
             if(abortFlag && *abortFlag)
             {
                 destructing = true;
                 std::unique_lock<std::mutex> lockIt(initialChunkGenerateStruct->lock);
                 initialChunkGenerateStruct->generatorWait = false;
                 initialChunkGenerateStruct->generatorWaitDoneCond.notify_all();
                 return;
             }
             std::this_thread::sleep_for(std::chrono::milliseconds(10));
         }
         if(abortFlag && *abortFlag)
         {
             destructing = true;
             std::unique_lock<std::mutex> lockIt(initialChunkGenerateStruct->lock);
             initialChunkGenerateStruct->generatorWait = false;
             initialChunkGenerateStruct->generatorWaitDoneCond.notify_all();
             return;
         }
         getDebugLog() << L"generated initial world." << postnl;
         {
             std::unique_lock<std::mutex> lockIt(initialChunkGenerateStruct->lock);
             initialChunkGenerateStruct->generatorWait = false;
             initialChunkGenerateStruct->generatorWaitDoneCond.notify_all();
         }
         initialChunkGenerateStruct = nullptr;
         particleGeneratingThread = thread([this]()
                                           {
                                               setThreadName(L"particle generate");
                                               TLS tls;
                                               particleGeneratingThreadFn(tls);
                                           });
         moveEntitiesThread = thread([this]()
                                     {
                                         setThreadName(L"move entities");
                                         TLS tls;
                                         moveEntitiesThreadFn(tls);
                                     });
     })();
    if(abortFlag && *abortFlag)
    {
        destructing = true;
        std::unique_lock<std::mutex> lockStateLock(stateLock);
        stateCond.notify_all();
        lockStateLock.unlock();
        for(auto &t : lightingThreads)
            if(t.joinable())
                t.join();
        for(auto &t : blockUpdateThreads)
            if(t.joinable())
                t.join();
        for(auto &t : chunkGeneratingThreads)
            if(t.joinable())
                t.join();
        if(particleGeneratingThread.joinable())
            particleGeneratingThread.join();
        if(moveEntitiesThread.joinable())
            moveEntitiesThread.join();
        LockedPlayers lockedPlayers = players().lock();
        std::vector<std::shared_ptr<Player>> copiedPlayerList(
            lockedPlayers.begin(), lockedPlayers.end()); // hold another reference to players so we
        // don't try to remove from players list
        // while destructing a list element
        players().players.clear();
        throw WorldConstructionAborted();
    }
}

World::~World()
{
    assert(viewPoints.empty());
    destructing = true;
    std::unique_lock<std::mutex> lockStateLock(stateLock);
    stateCond.notify_all();
    lockStateLock.unlock();
    for(auto &t : lightingThreads)
        if(t.joinable())
            t.join();
    for(auto &t : blockUpdateThreads)
        if(t.joinable())
            t.join();
    for(auto &t : chunkGeneratingThreads)
        if(t.joinable())
            t.join();
    if(particleGeneratingThread.joinable())
        particleGeneratingThread.join();
    if(moveEntitiesThread.joinable())
        moveEntitiesThread.join();
    if(chunkUnloaderThread.joinable())
        chunkUnloaderThread.join();
    std::vector<std::shared_ptr<Player>> copiedPlayerList; // hold another reference to players so
    // we don't try to remove from players
    // list while destructing a list element
    {
        LockedPlayers lockedPlayers = players().lock();
        copiedPlayerList.assign(lockedPlayers.begin(),
                                lockedPlayers.end()); // hold another reference to players so we
        // don't try to remove from players list while
        // destructing a list element
        players().players.clear();
    }
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
        UNREACHABLE();
        return Lighting();
    }
    return b.lighting;
}

void World::lightingThreadFn(TLS &tls)
{
    setThreadPriority(ThreadPriority::Low);
    ThreadPauseGuard pauseGuard(*this);
    WorldLockManager lock_manager(tls);
    BlockChunkMap *chunks = &physicsWorld->chunks;
    while(!destructing)
    {
        bool didAnything = false;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            if(destructing)
                break;
            if(!chunkIter->isLoaded())
                continue;
            std::shared_ptr<BlockChunk> chunk = chunkIter->getOrLoad(lock_manager.tls);
            if(chunkIter.is_locked())
                chunkIter.unlock();
            lock_manager.clear();
            pauseGuard.checkForPause();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            for(BlockUpdate *node =
                    removeAllBlockUpdatesInChunk(BlockUpdateKind::Lighting, cbi, lock_manager);
                node != nullptr;
                node = removeAllBlockUpdatesInChunk(BlockUpdateKind::Lighting, cbi, lock_manager))
            {
                didAnything = true;
                while(node != nullptr)
                {
                    BlockIterator bi = cbi;
                    bi.moveTo(node->position, lock_manager.tls);
                    Block b = bi.get(lock_manager);
                    if(b.good())
                    {
                        BlockIterator binx = bi;
                        binx.moveTowardNX(lock_manager.tls);
                        BlockIterator bipx = bi;
                        bipx.moveTowardPX(lock_manager.tls);
                        BlockIterator biny = bi;
                        biny.moveTowardNY(lock_manager.tls);
                        BlockIterator bipy = bi;
                        bipy.moveTowardPY(lock_manager.tls);
                        BlockIterator binz = bi;
                        binz.moveTowardNZ(lock_manager.tls);
                        BlockIterator bipz = bi;
                        bipz.moveTowardPZ(lock_manager.tls);
                        Lighting newLighting = b.descriptor->lightProperties.eval(
                            getBlockLighting(binx, lock_manager, false),
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
                    BlockUpdate::free(deleteMe, lock_manager.tls);
                }
                if(destructing)
                    break;
            }
        }
        lock_manager.clear();
        pauseGuard.checkForPause();
        if(!didAnything)
        {
            lightingStable = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void World::blockUpdateThreadFn(TLS &tls, bool isPhaseManager)
{
    setThreadPriority(ThreadPriority::Low);
    ThreadPauseGuard pauseGuard(*this);
    BlockChunkMap *chunks = &physicsWorld->chunks;
    bool didAnything = true;
    while(!destructing)
    {
        BlockUpdatePhase phase;
        if(isPhaseManager)
        {
            std::unique_lock<std::mutex> lockIt(stateLock);
            blockUpdateCurrentPhaseCount--;
            blockUpdateNextPhaseCount++;
            while(blockUpdateCurrentPhaseCount != 0)
            {
                stateCond.wait(lockIt);
                if(destructing)
                    return;
                pauseGuard.checkForPause(lockIt);
            }
            if(blockUpdateDidAnything)
                didAnything = true;
            if(didAnything && blockUpdateCurrentPhase == BlockUpdatePhase::InitialPhase)
            {
                blockUpdateDidAnything = false;
                lockIt.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                lockIt.lock();
            }
            std::swap(blockUpdateCurrentPhaseCount, blockUpdateNextPhaseCount);
            blockUpdateCurrentPhase = BlockUpdatePhaseNext(blockUpdateCurrentPhase);
            phase = blockUpdateCurrentPhase;
            stateCond.notify_all();
        }
        else
        {
            std::unique_lock<std::mutex> lockIt(stateLock);
            blockUpdateCurrentPhaseCount--;
            blockUpdateNextPhaseCount++;
            if(didAnything)
                blockUpdateDidAnything = true;
            stateCond.notify_all();
            phase = blockUpdateCurrentPhase;
            while(phase == blockUpdateCurrentPhase)
            {
                stateCond.wait(lockIt);
                if(destructing)
                    return;
                pauseGuard.checkForPause(lockIt);
            }
            phase = blockUpdateCurrentPhase;
        }
        if(destructing)
            return;
        didAnything = false;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            if(destructing)
                break;
            if(!chunkIter->isLoaded())
                continue;
            std::shared_ptr<BlockChunk> chunk = chunkIter->getOrLoad(tls);
            if(chunkIter.is_locked())
                chunkIter.unlock();
            pauseGuard.checkForPause();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            WorldLockManager lock_manager(tls);
            std::size_t ranUpdateCount = 0;
            for(BlockUpdate *node = removeAllReadyBlockUpdatesInChunk(phase, cbi, lock_manager);
                node != nullptr;
                node = removeAllReadyBlockUpdatesInChunk(phase, cbi, lock_manager))
            {
                didAnything = true;
                while(node != nullptr)
                {
                    BlockIterator bi = cbi;
                    bi.moveTo(node->position, lock_manager.tls);
                    Block b = bi.get(lock_manager);
                    if(b.good())
                    {
                        b.descriptor->tick(*this, b, bi, lock_manager, node->kind);
                    }

                    BlockUpdate *deleteMe = node;
                    node = node->chunk_next;
                    if(node != nullptr)
                        node->chunk_prev = nullptr;
                    BlockUpdate::free(deleteMe, lock_manager.tls);
                    if(ranUpdateCount++ > 500)
                    {
                        lock_manager.clear();
                        ranUpdateCount = 0;
                    }
                }
                if(destructing)
                    break;
            }
        }
        pauseGuard.checkForPause();
    }
}

void World::generateChunk(std::shared_ptr<BlockChunk> chunk,
                          WorldLockManager &lock_manager,
                          const std::atomic_bool *abortFlag,
                          std::unique_ptr<ThreadPauseGuard> &pauseGuard)
{
    lock_manager.clear();
    pauseGuard = nullptr;
    WorldLockManager new_lock_manager(lock_manager.tls);
    World newWorld(worldGeneratorSeed, nullptr, internal_construct_flag());
    worldGenerator->generateChunk(chunk->basePosition, newWorld, new_lock_manager, abortFlag);
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

    typedef BlocksGenerateArray BlockArrayType;
    typedef checked_array<checked_array<BiomeProperties, BlockChunk::chunkSizeZ>,
                          BlockChunk::chunkSizeX> BiomeArrayType;
    std::unique_ptr<BlockArrayType> blocks(new BlockArrayType);
    std::unique_ptr<BiomeArrayType> biomes(new BiomeArrayType);
    BlockIterator gcbi = newWorld.getBlockIterator(chunk->basePosition, new_lock_manager.tls);
    for(auto i = BlockChunkRelativePositionIterator::begin();
        i != BlockChunkRelativePositionIterator::end();
        i++)
    {
        BlockIterator bi = gcbi;
        bi.moveBy(*i, new_lock_manager.tls);
        blocks->at(i->x)[i->y][i->z] = bi.get(new_lock_manager);
    }
    for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
    {
        for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
        {
            BlockIterator bi = gcbi;
            bi.moveBy(VectorI(dx, 0, dz), new_lock_manager.tls);
            bi.updateLock(new_lock_manager);
            biomes->at(dx)[dz].swap(bi.getBiome().biomeProperties);
        }
    }
    pauseGuard = std::unique_ptr<ThreadPauseGuard>(new ThreadPauseGuard(*this));
    BlockIterator cbi = getBlockIterator(chunk->basePosition, lock_manager.tls);
    {
        std::unique_lock<std::recursive_mutex> lockSrcChunk(
            gcbi.chunk->getChunkVariables().entityListLock, std::defer_lock);
        std::unique_lock<std::recursive_mutex> lockDestChunk(
            cbi.chunk->getChunkVariables().entityListLock, std::defer_lock);
        std::lock(lockSrcChunk, lockDestChunk);
        WrappedEntity::ChunkListType &srcChunkList = gcbi.chunk->getChunkVariables().entityList;
        WrappedEntity::ChunkListType &destChunkList = cbi.chunk->getChunkVariables().entityList;
        for(WrappedEntity::ChunkListType::iterator srcChunkIter = srcChunkList.begin();
            srcChunkIter != srcChunkList.end();)
        {
            BlockChunkSubchunk *srcSubchunk = srcChunkIter->currentSubchunk;
            assert(srcSubchunk);
            new_lock_manager.block_biome_lock.set(srcSubchunk->lock);
            WrappedEntity::SubchunkListType &srcSubchunkList = srcSubchunk->entityList;
            WrappedEntity::SubchunkListType::iterator srcSubchunkIter =
                srcSubchunkList.to_iterator(&*srcChunkIter);
            if(!srcChunkIter->entity.good())
            {
                srcSubchunkList.erase(
                    WrappedEntity::SubchunkListType::const_iterator(srcSubchunkIter));
                srcChunkIter =
                    srcChunkList.erase(WrappedEntity::ChunkListType::const_iterator(srcChunkIter));
                continue;
            }
            srcChunkIter->lastEntityRunCount = 0;
            PositionF position = srcChunkIter->entity.physicsObject->getPosition();
            VectorF velocity = srcChunkIter->entity.physicsObject->getVelocity();
            srcChunkIter->entity.physicsObject->destroy();
            srcChunkIter->entity.physicsObject = srcChunkIter->entity.descriptor->makePhysicsObject(
                srcChunkIter->entity, position, velocity, physicsWorld);
            WrappedEntity::ChunkListType::iterator nextSrcIter = srcChunkIter;
            ++nextSrcIter;
            BlockIterator destBi = cbi;
            destBi.moveTo((PositionI)position, lock_manager.tls);
            if(!destBi.tryUpdateLock(lock_manager))
            {
                new_lock_manager.block_biome_lock.clear();
                auto lock_range = unit_range(srcSubchunk->lock);
                destBi.updateLock(lock_manager, lock_range.begin(), lock_range.end());
                new_lock_manager.block_biome_lock.adopt(srcSubchunk->lock);
            }
            WrappedEntity::SubchunkListType &destSubchunkList = destBi.getSubchunk().entityList;
            srcChunkIter->currentChunk = destBi.chunk.get();
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
            bi.moveBy(VectorI(dx, 0, dz), lock_manager.tls);
            bi.updateLock(lock_manager);
            biomes->at(dx)[dz].swap(bi.getBiome().biomeProperties);
        }
    }
    setBlockRange(chunk->basePosition,
                  chunk->basePosition + VectorI(BlockChunk::chunkSizeX - 1,
                                                BlockChunk::chunkSizeY - 1,
                                                BlockChunk::chunkSizeZ - 1),
                  lock_manager,
                  *blocks,
                  VectorI(0));
}

bool World::isInitialGenerateChunk(PositionI position)
{
    if(position == PositionI(0, 0, 0, Dimension::Overworld)
       || position == PositionI(-BlockChunk::chunkSizeX, 0, 0, Dimension::Overworld)
       || position == PositionI(0, 0, -BlockChunk::chunkSizeZ, Dimension::Overworld)
       || position
              == PositionI(
                     -BlockChunk::chunkSizeX, 0, -BlockChunk::chunkSizeZ, Dimension::Overworld))
    {
        return true;
    }
    return false;
}

void World::chunkGeneratingThreadFn(
    std::shared_ptr<InitialChunkGenerateStruct> initialChunkGenerateStruct, TLS &tls)
{
    std::unique_ptr<ThreadPauseGuard> pauseGuard =
        std::unique_ptr<ThreadPauseGuard>(new ThreadPauseGuard(*this));
    WorldLockManager lock_manager(tls);
    BlockChunkMap *chunks = &physicsWorld->chunks;
    while(!destructing && (!initialChunkGenerateStruct || !initialChunkGenerateStruct->abortFlag
                           || !*initialChunkGenerateStruct->abortFlag))
    {
        lock_manager.clear();
        pauseGuard->checkForPause();
        bool didAnything = false;
        std::shared_ptr<BlockChunk> bestChunk = nullptr;
        bool haveChunk = false;
        float chunkPriority = 0;
        bool isChunkInitialGenerate = false;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            BlockChunkChunkVariables &chunkVariables = chunkIter->chunkVariables;
            if(chunkVariables.generated)
                continue;
            if(chunkVariables.generateStarted)
                continue;
            std::shared_ptr<BlockChunk> chunk = chunkIter->getOrLoad(lock_manager.tls);
            if(chunkIter.is_locked())
                chunkIter.unlock();
            bool isCurrentChunkInitialGenerate = isInitialGenerateChunk(chunk->basePosition);
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            float currentChunkPriority = getChunkGeneratePriority(cbi, lock_manager);
            if(!isCurrentChunkInitialGenerate && std::isnan(currentChunkPriority))
                continue;
            if(!haveChunk || (isCurrentChunkInitialGenerate && !isChunkInitialGenerate)
               || chunkPriority > currentChunkPriority) // low values mean high priority
            {
                haveChunk = true;
                bestChunk = chunk;
                chunkPriority = currentChunkPriority;
                isChunkInitialGenerate = isCurrentChunkInitialGenerate;
            }
        }

        if(haveChunk)
        {
            std::shared_ptr<BlockChunk> chunk = bestChunk;
            if(chunk->getChunkVariables().generateStarted.exchange(
                   true)) // someone else got this chunk
                continue;
            getDebugLog() << L"generating " << chunk->basePosition << L" " << chunkPriority
                          << postnl;
            didAnything = true;

            if(isChunkInitialGenerate && initialChunkGenerateStruct->abortFlag)
                generateChunk(
                    chunk, lock_manager, initialChunkGenerateStruct->abortFlag, pauseGuard);
            else
                generateChunk(chunk, lock_manager, &destructing, pauseGuard);

            chunk->getChunkVariables().generated = true;
            lock_manager.clear();
            if(isChunkInitialGenerate && initialChunkGenerateStruct != nullptr)
            {
                std::unique_lock<std::mutex> lockIt(initialChunkGenerateStruct->lock);
                if(initialChunkGenerateStruct->count > 0)
                {
                    initialChunkGenerateStruct->count--;
                    if(initialChunkGenerateStruct->count == 0)
                    {
                        initialChunkGenerateStruct->initialGenerationDoneCond.notify_all();
                        while(initialChunkGenerateStruct->generatorWait)
                            initialChunkGenerateStruct->generatorWaitDoneCond.wait(lockIt);
                        lockIt.unlock();
                        lockIt.release();
                        initialChunkGenerateStruct = nullptr;
                    }
                }
                else
                {
                    while(initialChunkGenerateStruct->generatorWait)
                        initialChunkGenerateStruct->generatorWaitDoneCond.wait(lockIt);
                    lockIt.unlock();
                    lockIt.release();
                    initialChunkGenerateStruct = nullptr;
                }
            }
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

float World::getChunkGeneratePriority(
    BlockIterator bi,
    WorldLockManager &lock_manager) // low values mean high priority, NAN means don't generate
{
    if(isInitialGenerateChunk(bi.position()))
    {
        return -1e30;
    }
    if(bi.position().y != 0)
        return NAN;
    std::unique_lock<std::mutex> lockIt(viewPointsLock);
    float retval = 0;
    bool retvalSet = false;
    PositionF chunkMinCorner = bi.position();
    PositionF chunkMaxCorner =
        chunkMinCorner
        + VectorI(BlockChunk::chunkSizeX, BlockChunk::chunkSizeY, BlockChunk::chunkSizeZ);
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

namespace
{
std::shared_ptr<BlockChunk> &getLoadIntoChunk(TLS &tls)
{
    struct retval_tls_tag
    {
    };
    thread_local_variable<std::shared_ptr<BlockChunk>, retval_tls_tag> retval(tls);
    return retval.get();
}
}

BlockIterator World::getBlockIteratorForWorldAddEntity(PositionI pos, TLS &tls)
{
    std::shared_ptr<BlockChunk> chunk = getLoadIntoChunk(tls);
    if(chunk == nullptr)
    {
        return getBlockIterator(pos, tls);
    }
    else // if pos is not in chunk then put in chunk anyway to avoid locking; will be fixed by next
    // world move
    {
        return BlockIterator(chunk,
                             &physicsWorld->chunks,
                             chunk->basePosition,
                             BlockChunk::getChunkRelativePosition(pos));
    }
}

Entity *World::addEntity(EntityDescriptorPointer descriptor,
                         PositionF position,
                         VectorF velocity,
                         WorldLockManager &lock_manager,
                         std::shared_ptr<void> entityData)
{
    assert(descriptor != nullptr);
    BlockIterator bi = getBlockIteratorForWorldAddEntity((PositionI)position, lock_manager.tls);
    lock_manager.block_biome_lock.clear();
    WrappedEntity *entity = new WrappedEntity(Entity(descriptor, nullptr, entityData));
    entity->entity.physicsObject =
        descriptor->makePhysicsObject(entity->entity, position, velocity, physicsWorld);
    descriptor->makeData(entity->entity, *this, lock_manager);
    lock_manager.block_biome_lock.clear();
    std::unique_lock<std::recursive_mutex> lockChunk(bi.chunk->getChunkVariables().entityListLock);
    bi.updateLock(lock_manager);
    WrappedEntity::ChunkListType &chunkList = bi.chunk->getChunkVariables().entityList;
    WrappedEntity::SubchunkListType &subchunkList = bi.getSubchunk().entityList;
    entity->currentChunk = bi.chunk.get();
    entity->currentSubchunk = &bi.getSubchunk();
    chunkList.push_back(entity);
    subchunkList.push_back(entity);
    entity->verify();
    return &entity->entity;
}

void World::move(double deltaTime, WorldLockManager &lock_manager)
{
    lock_manager.clear();
    std::unique_lock<std::mutex> lockStateLock(stateLock);
    if(isPaused)
        return;
    while(waitingForMoveEntities && !destructing)
    {
        stateCond.wait(lockStateLock);
    }
    advanceTimeOfDay(deltaTime);
    waitingForMoveEntities = true;
    moveEntitiesDeltaTime = deltaTime;
    stateCond.notify_all();
}

void World::moveEntitiesThreadFn(TLS &tls)
{
    ThreadPauseGuard pauseGuard(*this);
    ThreadUsageMonitor usageMonitor(L"move entities", 0.5f);
    auto lastUsageReportTime = std::chrono::steady_clock::now();
    while(!destructing)
    {
        std::unique_lock<std::mutex> lockStateLock(stateLock);
        waitingForMoveEntities = false;
        stateCond.notify_all();
        while(!destructing && !waitingForMoveEntities)
        {
            usageMonitor.unusedCall([&]()
                                    {
                                        stateCond.wait(lockStateLock);
                                        pauseGuard.checkForPause(lockStateLock);
                                    });
        }
        if(destructing)
            break;
        double deltaTime = moveEntitiesDeltaTime;
        lockStateLock.unlock();
        {
            auto currentTime = std::chrono::steady_clock::now();
            if(currentTime - lastUsageReportTime >= std::chrono::seconds(1))
            {
                usageMonitor.dump(tls);
                lastUsageReportTime = currentTime;
            }
        }
        WorldLockManager lock_manager(tls);
        entityRunCount++;
        physicsWorld->stepTime(deltaTime, lock_manager);
        BlockChunkMap *chunks = &physicsWorld->chunks;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            BlockChunkChunkVariables &chunkVariables = chunkIter->chunkVariables;
            bool isGenerated = chunkVariables.generated;
            bool isChunkCloseEnough =
                isChunkCloseEnoughToPlayerToGetRandomUpdates(chunkIter->basePosition);
            if(!chunkIter->isLoaded() && (!isGenerated || !isChunkCloseEnough))
                continue;
            std::shared_ptr<BlockChunk> chunk = chunkIter->getOrLoad(lock_manager.tls);
            if(chunkIter.is_locked())
                chunkIter.unlock();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            if(isGenerated && isChunkCloseEnough)
            {
                double fRandomTickCount =
                    deltaTime * (20.0f * 3.0f / 16.0f / 16.0f / 16.0f * BlockChunk::chunkSizeX
                                 * BlockChunk::chunkSizeY * BlockChunk::chunkSizeZ);
                // fRandomTickCount *= 5;
                int randomTickCount = static_cast<int>(std::floor(
                    fRandomTickCount + std::generate_canonical<float, 20>(getRandomGenerator())));
                for(int i = 0; i < randomTickCount; i++)
                {
                    VectorI relativePosition =
                        VectorI(std::uniform_int_distribution<>(
                                    0, BlockChunk::chunkSizeX - 1)(getRandomGenerator()),
                                std::uniform_int_distribution<>(
                                    0, BlockChunk::chunkSizeY - 1)(getRandomGenerator()),
                                std::uniform_int_distribution<>(
                                    0, BlockChunk::chunkSizeZ - 1)(getRandomGenerator()));
                    BlockIterator bi = cbi;
                    bi.moveBy(relativePosition, lock_manager.tls);
                    Block b = bi.get(lock_manager);
                    if(b.good())
                    {
                        b.descriptor->randomTick(b, *this, bi, lock_manager);
                    }
                }
            }
            std::unique_lock<std::recursive_mutex> lockChunk(
                chunk->getChunkVariables().entityListLock);
            WrappedEntity::ChunkListType &chunkEntityList = chunk->getChunkVariables().entityList;
            auto i = chunkEntityList.begin();
            while(i != chunkEntityList.end())
            {
                WrappedEntity &entity = *i;
                if(!entity.entity.good())
                {
                    lock_manager.block_biome_lock.set(entity.currentSubchunk->lock);
                    WrappedEntity::SubchunkListType &subchunkEntityList =
                        entity.currentSubchunk->entityList;
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
                entity.verify();
                BlockIterator destBi = cbi;
                destBi.moveTo((PositionI)entity.entity.physicsObject->getPosition(),
                              lock_manager.tls);
                entity.entity.descriptor->moveStep(entity.entity, *this, lock_manager, deltaTime);
                if(entity.currentChunk == destBi.chunk.get()
                   && entity.currentSubchunk == &destBi.getSubchunk())
                {
                    ++i;
                    entity.verify();
                    continue;
                }
                lock_manager.block_biome_lock.set(entity.currentSubchunk->lock);
                WrappedEntity::SubchunkListType &subchunkEntityList =
                    entity.currentSubchunk->entityList;
                subchunkEntityList.detach(subchunkEntityList.to_iterator(&entity));
                destBi.updateLock(lock_manager);
                entity.currentSubchunk = &destBi.getSubchunk();
                WrappedEntity::SubchunkListType &subchunkDestEntityList =
                    entity.currentSubchunk->entityList;
                subchunkDestEntityList.push_back(&entity);
                if(entity.currentChunk == destBi.chunk.get())
                {
                    ++i;
                    entity.verify();
                    continue;
                }
                auto nextI = i;
                ++nextI;
                std::unique_lock<std::recursive_mutex> lockChunk(
                    destBi.chunk->getChunkVariables().entityListLock);
                WrappedEntity::ChunkListType &chunkDestEntityList =
                    chunk->getChunkVariables().entityList;
                chunkDestEntityList.splice(chunkDestEntityList.end(), chunkEntityList, i);
                entity.currentChunk = destBi.chunk.get();
                i = nextI;
                entity.verify();
            }
        }
    }
}

RayCasting::Collision World::castRayCheckForEntitiesInSubchunk(BlockIterator bi,
                                                               RayCasting::Ray ray,
                                                               WorldLockManager &lock_manager,
                                                               float maxSearchDistance,
                                                               const Entity *ignoreEntity)
{
    PositionI subchunkPos = BlockChunk::getSubchunkBaseAbsolutePosition(bi.position());
    RayCasting::Collision retval(*this);
    for(int dx = -1; dx <= 1; dx++)
    {
        for(int dy = -1; dy <= 1; dy++)
        {
            for(int dz = -1; dz <= 1; dz++)
            {
                PositionI currentSubchunkPos =
                    subchunkPos + VectorI(dx * BlockChunk::subchunkSizeXYZ,
                                          dy * BlockChunk::subchunkSizeXYZ,
                                          dz * BlockChunk::subchunkSizeXYZ);
                bi.moveTo(currentSubchunkPos, lock_manager.tls);
                bi.updateLock(lock_manager);
                for(WrappedEntity &entity : bi.getSubchunk().entityList)
                {
                    if(&entity.entity == ignoreEntity || !entity.entity.good())
                        continue;
                    RayCasting::Collision currentCollision =
                        entity.entity.descriptor->getRayCollision(entity.entity, *this, ray);
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

RayCasting::Collision World::castRay(RayCasting::Ray ray,
                                     WorldLockManager &lock_manager,
                                     float maxSearchDistance,
                                     RayCasting::BlockCollisionMask blockRayCollisionMask,
                                     const Entity *ignoreEntity)
{
    PositionI startPosition = (PositionI)ray.startPosition;
    BlockIterator bi = getBlockIterator(startPosition, lock_manager.tls);
    BlockChunkSubchunk *lastSubchunk = nullptr;
    RayCasting::Collision retval(*this);
    for(auto i = RayCasting::makeRayBlockIterator(ray);
        std::get<0>(*i) <= maxSearchDistance || std::get<1>(*i) == startPosition;
        i++)
    {
        bi.moveTo(std::get<1>(*i), lock_manager.tls);
        if(&bi.getSubchunk() != lastSubchunk)
        {
            lastSubchunk = &bi.getSubchunk();
            RayCasting::Collision currentCollision = castRayCheckForEntitiesInSubchunk(
                bi, ray, lock_manager, maxSearchDistance, ignoreEntity);
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
        RayCasting::Collision currentCollision =
            b.descriptor->getRayCollision(b, bi, lock_manager, *this, ray);
        if(currentCollision < retval)
            retval = currentCollision;
        if(retval.valid() && retval.t < maxSearchDistance)
            maxSearchDistance = retval.t;
    }
    if(retval.valid() && retval.t <= maxSearchDistance)
        return retval;
    return RayCasting::Collision(*this);
}

void World::generateParticlesInSubchunk(BlockIterator sbi,
                                        WorldLockManager &lock_manager,
                                        double currentTime,
                                        double deltaTime,
                                        std::vector<PositionI> &positions)
{
    positions.clear();
    BlockChunkSubchunk &subchunk = sbi.getSubchunk();
    sbi.updateLock(lock_manager);
    subchunk.addToParticleGeneratingBlockList(positions);
    for(PositionI pos : positions)
    {
        BlockIterator bi = sbi;
        bi.moveTo(pos, lock_manager.tls);
        Block b = bi.get(lock_manager);
        if(b.good())
            b.descriptor->generateParticles(*this, b, bi, lock_manager, currentTime, deltaTime);
    }
}

void World::particleGeneratingThreadFn(TLS &tls)
{
    double currentTime = 0;
    ThreadPauseGuard pauseGuard(*this);
    auto lastTimePoint = std::chrono::steady_clock::now();
    std::vector<PositionI> positionsBuffer;
    WorldLockManager lock_manager(tls);
    while(!destructing)
    {
        lock_manager.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto beforePauseTimePoint = std::chrono::steady_clock::now();
        pauseGuard.checkForPause();
        auto currentTimePoint = std::chrono::steady_clock::now();
        double deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(
                               beforePauseTimePoint - lastTimePoint).count();
        lastTimePoint = currentTimePoint;
        std::vector<std::pair<PositionF, float>> viewPointsVector;
        {
            std::unique_lock<std::mutex> lockIt(viewPointsLock);
            for(ViewPoint *viewPoint : viewPoints)
            {
                PositionF position;
                std::int32_t viewDistance;
                viewPoint->getPositionAndViewDistance(position, viewDistance);
                float generateDistance = 30.0f;
                viewPointsVector.emplace_back(position, generateDistance);
            }
        }
        for(std::size_t viewPointIndex = 0; viewPointIndex < viewPointsVector.size();
            viewPointIndex++)
        {
            PositionF centerPosition = std::get<0>(viewPointsVector[viewPointIndex]);
            float generateDistance = std::get<1>(viewPointsVector[viewPointIndex]);
            BlockIterator gbi = getBlockIterator((PositionI)centerPosition, lock_manager.tls);
            PositionF minPositionF = centerPosition - VectorF(generateDistance);
            PositionF maxPositionF = centerPosition + VectorF(generateDistance);
            PositionI minP = BlockChunk::getSubchunkBaseAbsolutePosition((PositionI)minPositionF);
            PositionI maxP = BlockChunk::getSubchunkBaseAbsolutePosition((PositionI)maxPositionF);
            for(PositionI pos = minP; pos.x <= maxP.x; pos.x += BlockChunk::subchunkSizeXYZ)
            {
                for(pos.y = minP.y; pos.y <= maxP.y; pos.y += BlockChunk::subchunkSizeXYZ)
                {
                    for(pos.z = minP.z; pos.z <= maxP.z; pos.z += BlockChunk::subchunkSizeXYZ)
                    {
                        bool generatedBefore = false;
                        for(std::size_t i = 0; i < viewPointIndex; i++)
                        {
                            PositionF centerPosition2 = std::get<0>(viewPointsVector[i]);
                            float generateDistance2 = std::get<1>(viewPointsVector[i]);
                            PositionF minPositionF2 = centerPosition2 - VectorF(generateDistance2);
                            PositionF maxPositionF2 = centerPosition2 + VectorF(generateDistance2);
                            PositionI minP2 = BlockChunk::getSubchunkBaseAbsolutePosition(
                                (PositionI)minPositionF2);
                            PositionI maxP2 = BlockChunk::getSubchunkBaseAbsolutePosition(
                                (PositionI)maxPositionF2);
                            if(pos.x >= minP2.x && pos.x <= maxP2.x && pos.y >= minP2.y
                               && pos.y <= maxP2.y
                               && pos.z >= minP2.z
                               && pos.z <= maxP2.z)
                            {
                                generatedBefore = true;
                                break;
                            }
                        }
                        if(generatedBefore)
                            continue;
                        BlockIterator sbi = gbi;
                        sbi.moveTo(pos, lock_manager.tls);
                        generateParticlesInSubchunk(
                            sbi, lock_manager, currentTime, deltaTime, positionsBuffer);
                    }
                }
            }
        }

        currentTime += deltaTime;
    }
}

void World::invalidateBlockRange(BlockIterator blockIterator,
                                 VectorI minCorner,
                                 VectorI maxCorner,
                                 WorldLockManager &lock_manager)
{
    BlockChunkSubchunk *lastSubchunk = nullptr;
    std::unique_lock<std::mutex> lockIt;
    VectorI minSubchunk = BlockChunk::getSubchunkBaseAbsolutePosition(minCorner);
    VectorI maxSubchunk = BlockChunk::getSubchunkBaseAbsolutePosition(maxCorner);
    for(VectorI sp = minSubchunk; sp.x <= maxSubchunk.x; sp.x += BlockChunk::subchunkSizeXYZ)
    {
        for(sp.y = minSubchunk.y; sp.y <= maxSubchunk.y; sp.y += BlockChunk::subchunkSizeXYZ)
        {
            for(sp.z = minSubchunk.z; sp.z <= maxSubchunk.z; sp.z += BlockChunk::subchunkSizeXYZ)
            {
                PositionI chunkBasePosition =
                    BlockChunk::getChunkBasePosition(PositionI(sp, blockIterator.position().d));
                IndirectBlockChunk &ibc = physicsWorld->chunks[chunkBasePosition];
                if(!ibc.isLoaded())
                    continue;
                BlockIterator bi = blockIterator;
                bi.moveTo(sp, lock_manager.tls);
                VectorI currentMinCorner = sp;
                VectorI currentMaxCorner = sp + VectorI(BlockChunk::subchunkSizeXYZ - 1);
                currentMinCorner.x = std::max(currentMinCorner.x, minCorner.x);
                currentMinCorner.y = std::max(currentMinCorner.y, minCorner.y);
                currentMinCorner.z = std::max(currentMinCorner.z, minCorner.z);
                currentMaxCorner.x = std::min(currentMaxCorner.x, maxCorner.x);
                currentMaxCorner.y = std::min(currentMaxCorner.y, maxCorner.y);
                currentMaxCorner.z = std::min(currentMaxCorner.z, maxCorner.z);
                for(VectorI p = currentMinCorner; p.x <= currentMaxCorner.x; p.x++)
                {
                    for(p.y = currentMinCorner.y; p.y <= currentMaxCorner.y; p.y++)
                    {
                        for(p.z = currentMinCorner.z; p.z <= currentMaxCorner.z; p.z++)
                        {
                            BlockIterator biXYZ = bi;
                            biXYZ.moveTo(p, lock_manager.tls);
                            BlockChunkSubchunk &subchunk = biXYZ.getSubchunk();
                            if(&subchunk != lastSubchunk && lastSubchunk != nullptr)
                            {
                                lockIt.unlock();
                            }
                            BlockChunkBlock &b = biXYZ.getBlock(lock_manager);
                            if(&subchunk != lastSubchunk)
                            {
                                lastSubchunk = &subchunk;
                                lockIt = std::unique_lock<std::mutex>(
                                    biXYZ.chunk->getChunkVariables().blockUpdateListLock);
                            }
                            b.invalidate();
                            biXYZ.getSubchunk().invalidate();
                            biXYZ.chunk->getChunkVariables().invalidate();
                            BlockOptionalData *blockOptionalData =
                                subchunk.blockOptionalData.get_or_make(
                                    BlockChunk::getSubchunkRelativePosition(
                                        biXYZ.currentRelativePosition),
                                    lock_manager.tls);
                            biXYZ.getBlock().hasOptionalData = true;
                            enum_array<bool, BlockUpdateKind> neededBlockUpdates;
                            for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
                            {
                                neededBlockUpdates[kind] =
                                    (BlockUpdateKindDefaultPeriod(kind) == 0);
                            }
                            BlockUpdate **ppnode = &blockOptionalData->updateListHead;
                            BlockUpdate *pnode = *ppnode;
                            while(pnode != nullptr)
                            {
                                if(neededBlockUpdates[pnode->kind])
                                {
                                    if(pnode->time_left > 0)
                                        pnode->time_left = 0;
                                    neededBlockUpdates[pnode->kind] = false;
                                }
                                ppnode = &pnode->block_next;
                                pnode = *ppnode;
                            }
                            for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
                            {
                                if(neededBlockUpdates[kind])
                                {
                                    BlockUpdate *pnode =
                                        BlockUpdate::allocate(lock_manager.tls,
                                                              kind,
                                                              biXYZ.position(),
                                                              0.0f,
                                                              blockOptionalData->updateListHead);
                                    bi.chunk->getChunkVariables()
                                        .blockUpdatesPerPhase[BlockUpdateKindPhase(kind)]++;
                                    blockOptionalData->updateListHead = pnode;
                                    pnode->chunk_next =
                                        biXYZ.chunk->getChunkVariables().blockUpdateListHead;
                                    pnode->chunk_prev = nullptr;
                                    if(biXYZ.chunk->getChunkVariables().blockUpdateListHead
                                       != nullptr)
                                        biXYZ.chunk->getChunkVariables()
                                            .blockUpdateListHead->chunk_prev = pnode;
                                    else
                                        biXYZ.chunk->getChunkVariables().blockUpdateListTail =
                                            pnode;
                                    biXYZ.chunk->getChunkVariables().blockUpdateListHead = pnode;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void World::write(stream::Writer &writerIn, WorldLockManager &lock_manager)
{
    bool wasPaused = paused();
    paused(true, lock_manager);
    try
    {
        stream::write<std::uint32_t>(writerIn, GameVersion::FILE_VERSION);
        stream::MemoryWriter memWriter;
        {
            stream::CompressWriter writer(memWriter);
            StreamWorldGuard streamWorldGuard(writer, *this, lock_manager);
            stream::write<SeedType>(writer, worldGeneratorSeed);
            {
                std::unique_lock<std::mutex> lockIt(randomGeneratorLock);
                stream::write<rc4_random_engine>(writer, randomGenerator);
            }
            LockedPlayers lockedPlayers = players().lock();
            stream::write<std::uint64_t>(writer, players().players.size());
            for(std::shared_ptr<Player> player : lockedPlayers)
            {
                player->write(writer);
            }
            std::vector<std::shared_ptr<BlockChunk>> chunks;
            BlockChunkMap *chunksMap = &physicsWorld->chunks;
            {
                for(auto chunkIter = chunksMap->begin(); chunkIter != chunksMap->end(); chunkIter++)
                {
                    if(!chunkIter->chunkVariables.generated)
                        continue;
                    std::shared_ptr<BlockChunk> chunk = chunkIter->getOrLoad(lock_manager.tls);
                    if(chunkIter.is_locked())
                        chunkIter.unlock();
                    chunks.push_back(chunk);
                }
            }
            std::uint64_t chunkCount = chunks.size();
            stream::write<std::uint64_t>(writer, chunkCount);
            std::vector<std::tuple<PositionI, float, BlockUpdateKind>> blockUpdates;
            for(std::shared_ptr<BlockChunk> chunk : chunks)
            {
                stream::write<PositionI>(writer, chunk->basePosition);
                std::vector<WrappedEntity *> entities;
                {
                    std::unique_lock<std::recursive_mutex> lockChunk(
                        chunk->getChunkVariables().entityListLock);
                    WrappedEntity::ChunkListType &chunkEntityList =
                        chunk->getChunkVariables().entityList;
                    for(auto i = chunkEntityList.begin(); i != chunkEntityList.end(); ++i)
                    {
                        WrappedEntity &entity = *i;
                        if(!entity.entity.good())
                        {
                            continue;
                        }
                        entities.push_back(&entity);
                    }
                }
                stream::write<std::uint64_t>(writer, entities.size());
                for(WrappedEntity *entity : entities)
                {
                    entity->entity.write(writer);
                }
                entities.clear();
                BlockIterator cbi(chunk, chunksMap, chunk->basePosition, VectorI(0, 0, 0));
                for(std::size_t x = 0; x < BlockChunk::chunkSizeX; x++)
                {
                    for(std::size_t z = 0; z < BlockChunk::chunkSizeZ; z++)
                    {
                        BlockIterator columnBlockIterator = cbi;
                        columnBlockIterator.moveBy(VectorI(x, 0, z), lock_manager.tls);
                        stream::write<BiomeProperties>(
                            writer, columnBlockIterator.getBiomeProperties(lock_manager));
                        BlockIterator bi = columnBlockIterator;
                        blockUpdates.clear();
                        for(std::size_t y = 0; y < BlockChunk::chunkSizeY;
                            y++, bi.moveTowardPY(lock_manager.tls))
                        {
                            stream::write<Block>(writer, bi.get(lock_manager));
                            for(BlockUpdateIterator iter = bi.updatesBegin(lock_manager);
                                iter != bi.updatesEnd(lock_manager);
                                ++iter)
                            {
                                blockUpdates.emplace_back(
                                    iter->getPosition(), iter->getTimeLeft(), iter->getKind());
                            }
                        }
                        stream::write<std::uint32_t>(
                            writer, static_cast<std::uint32_t>(blockUpdates.size()));
                        for(auto update : blockUpdates)
                        {
                            stream::write<PositionI>(writer, std::get<0>(update));
                            stream::write<float32_t>(writer, std::get<1>(update));
                            stream::write<BlockUpdateKind>(writer, std::get<2>(update));
                        }
                    }
                }
            }
            std::unique_lock<std::recursive_mutex> lockTimeOfDay(timeOfDayLock);
            stream::write<float32_t>(writer, timeOfDayInSeconds);
            stream::write<std::uint8_t>(writer, moonPhase);
            writer.flush();
        }
        lock_manager.clear();
        writerIn.writeBytes(memWriter.getBuffer().data(), memWriter.getBuffer().size());
    }
    catch(stream::IOException &)
    {
        paused(wasPaused, lock_manager);
        throw;
    }
    paused(wasPaused, lock_manager);
}

namespace
{
struct file_version_tag_t
{
};

void setStreamFileVersion(stream::Reader &reader, std::uint32_t fileVersion)
{
    reader.setAssociatedValue<std::uint32_t, file_version_tag_t>(
        std::make_shared<std::uint32_t>(fileVersion));
}
}

std::uint32_t World::getStreamFileVersion(stream::Reader &reader)
{
    std::shared_ptr<std::uint32_t> version =
        reader.getAssociatedValue<std::uint32_t, file_version_tag_t>();
    assert(version);
    return *version;
}

std::shared_ptr<World> World::read(stream::Reader &readerIn)
{
    std::uint32_t fileVersion = stream::read<std::uint32_t>(readerIn);
    if(fileVersion < 5)
    {
        throw stream::InvalidDataValueException("old file version not supported");
    }
    if(fileVersion > GameVersion::FILE_VERSION)
    {
        throw stream::InvalidDataValueException("newer file version not supported");
    }
    stream::ExpandReader reader(readerIn);
    setStreamFileVersion(reader, fileVersion);
    SeedType worldGeneratorSeed = stream::read<SeedType>(reader);
    std::shared_ptr<World> retval = std::make_shared<World>(
        worldGeneratorSeed, MyWorldGenerator::getInstance(), internal_construct_flag());
    World &world = *retval;
    world.chunkUnloaderThread = thread([&world]()
                                       {
                                           setThreadName(L"chunk unloader");
                                           TLS tls;
                                           world.chunkUnloaderThreadFn(tls);
                                       });
    WorldLockManager lock_manager(TLS::getSlow());
    StreamWorldGuard streamWorldGuard(reader, world, lock_manager);
    world.randomGenerator = stream::read<rc4_random_engine>(reader);
    std::uint64_t playerCount = stream::read<std::uint64_t>(reader);
    for(std::uint64_t i = 0; i < playerCount; i++)
    {
        std::shared_ptr<Player> player = Player::read(reader);
        // read function already adds to world
        ignore_unused_variable_warning(player);
    }
    std::uint64_t chunkCount = stream::read<std::uint64_t>(reader);
    struct BlocksTLSTag
    {
    };
    thread_local_variable<BlocksGenerateArray, BlocksTLSTag> blocksTLS(lock_manager.tls);
    auto &blocks = blocksTLS.get();
    struct BiomesTLSTag
    {
    };
    thread_local_variable<checked_array<checked_array<BiomeProperties, BlockChunk::chunkSizeZ>,
                                        BlockChunk::chunkSizeX>,
                          BiomesTLSTag> biomesTLS(lock_manager.tls);
    auto &biomes = biomesTLS.get();
    auto blockUpdates =
        checked_array<checked_array<checked_array<std::vector<std::tuple<PositionI,
                                                                         float,
                                                                         BlockUpdateKind>>,
                                                  BlockChunk::subchunkCountZ>,
                                    BlockChunk::subchunkCountY>,
                      BlockChunk::subchunkCountX>();
    for(std::uint64_t chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++)
    {
        PositionI chunkBasePosition = stream::read<PositionI>(reader);
        if(chunkBasePosition != BlockChunk::getChunkBasePosition(chunkBasePosition))
            throw stream::InvalidDataValueException("block chunk base is not a valid position");
        BlockIterator cbi = world.getBlockIterator(chunkBasePosition, lock_manager.tls);
        cbi.chunk->getChunkVariables().generated = true;
        cbi.chunk->getChunkVariables().generateStarted = true;
        std::uint64_t entityCount = stream::read<std::uint64_t>(reader);
        for(std::uint64_t entityIndex = 0; entityIndex < entityCount; entityIndex++)
        {
            Entity().read(reader,
                          [&world, &lock_manager](Entity &e, PositionF position, VectorF velocity)
                          {
                              world.addEntity(
                                  e.descriptor, position, velocity, lock_manager, e.data);
                          });
        }
        for(std::size_t x = 0; x < BlockChunk::chunkSizeX; x++)
        {
            for(std::size_t z = 0; z < BlockChunk::chunkSizeZ; z++)
            {
                biomes[x][z] = stream::read<BiomeProperties>(reader);
                for(std::size_t y = 0; y < BlockChunk::chunkSizeY; y++)
                {
                    blocks[x][y][z] = stream::read<Block>(reader);
                }
                std::uint32_t blockUpdateCount = stream::read<std::uint32_t>(reader);
                for(; blockUpdateCount > 0; blockUpdateCount--)
                {
                    PositionI position = stream::read<PositionI>(reader);
                    if(chunkBasePosition != BlockChunk::getChunkBasePosition(position))
                        throw stream::InvalidDataValueException("block update is outside of chunk");
                    float timeLeft = stream::read_limited<float32_t>(reader, 0, 1e6);
                    BlockUpdateKind blockUpdateKind = stream::read<BlockUpdateKind>(reader);
                    VectorI subchunkIndex = BlockChunk::getSubchunkIndexFromPosition(position);
                    blockUpdates[subchunkIndex.x][subchunkIndex.y][subchunkIndex.z].emplace_back(
                        position, timeLeft, blockUpdateKind);
                }
            }
        }
        for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
        {
            for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
            {
                BlockIterator bi = cbi;
                bi.moveBy(VectorI(dx, 0, dz), lock_manager.tls);
                bi.updateLock(lock_manager);
                biomes[dx][dz].swap(bi.getBiome().biomeProperties);
            }
        }
        for(VectorI subchunkPos = VectorI(0); subchunkPos.x < BlockChunk::chunkSizeX;
            subchunkPos.x += BlockChunk::subchunkSizeXYZ)
        {
            for(subchunkPos.y = 0; subchunkPos.y < BlockChunk::chunkSizeY;
                subchunkPos.y += BlockChunk::subchunkSizeXYZ)
            {
                for(subchunkPos.z = 0; subchunkPos.z < BlockChunk::chunkSizeZ;
                    subchunkPos.z += BlockChunk::subchunkSizeXYZ)
                {
                    BlockIterator sbi = cbi;
                    sbi.moveBy(subchunkPos, lock_manager.tls);
                    BlockChunkSubchunk &subchunk = sbi.getSubchunk();
                    VectorI subchunkIndex =
                        BlockChunk::getSubchunkIndexFromChunkRelativePosition(subchunkPos);
                    std::vector<std::tuple<PositionI, float, BlockUpdateKind>> &
                        currentBlockUpdates =
                            blockUpdates[subchunkIndex.x][subchunkIndex.y][subchunkIndex.z];
                    for(auto update : currentBlockUpdates)
                    {
                        BlockIterator bi = sbi;
                        bi.moveTo(std::get<0>(update), lock_manager.tls);
                        world.addBlockUpdate(
                            bi, lock_manager, std::get<2>(update), std::get<1>(update));
                    }
                    currentBlockUpdates.clear();
                    for(VectorI subchunkRelativePos = VectorI(0);
                        subchunkRelativePos.x < BlockChunk::subchunkSizeXYZ;
                        subchunkRelativePos.x++)
                    {
                        for(subchunkRelativePos.y = 0;
                            subchunkRelativePos.y < BlockChunk::subchunkSizeXYZ;
                            subchunkRelativePos.y++)
                        {
                            for(subchunkRelativePos.z = 0;
                                subchunkRelativePos.z < BlockChunk::subchunkSizeXYZ;
                                subchunkRelativePos.z++)
                            {
                                BlockIterator bi = sbi;
                                bi.moveBy(subchunkRelativePos, lock_manager.tls);
                                VectorI newBlocksPosition = subchunkRelativePos + subchunkPos;
                                Block newBlock =
                                    blocks[newBlocksPosition.x][newBlocksPosition
                                                                    .y][newBlocksPosition.z];
                                BlockChunkBlock &b = bi.getBlock(lock_manager);
                                BlockDescriptorPointer bd = subchunk.getBlockKind(b);
                                if(bd != nullptr && bd->generatesParticles())
                                {
                                    subchunk.removeParticleGeneratingBlock(bi.position());
                                }
                                bd = newBlock.descriptor;
                                BlockChunk::putBlockIntoArray(
                                    BlockChunk::getSubchunkRelativePosition(
                                        bi.currentRelativePosition),
                                    b,
                                    subchunk,
                                    std::move(newBlock),
                                    lock_manager.tls);
                                if(bd != nullptr && bd->generatesParticles())
                                {
                                    subchunk.addParticleGeneratingBlock(bi.position());
                                }
                            }
                        }
                    }
                    world.lightingStable = false;
                }
            }
        }
        world.lightingStable = false;
    }
    for(std::size_t x = 0; x < BlockChunk::chunkSizeX; x++)
    {
        for(std::size_t z = 0; z < BlockChunk::chunkSizeZ; z++)
        {
            for(std::size_t y = 0; y < BlockChunk::chunkSizeY; y++)
            {
                blocks[x][y][z] = Block();
            }
        }
    }
    float timeOfDayInSeconds = stream::read_limited<float32_t>(reader, 0, dayDurationInSeconds);
    std::uint8_t moonPhase = stream::read_limited<std::uint8_t>(reader, 0, moonPhaseCount);
    world.setTimeOfDayInSeconds(timeOfDayInSeconds, moonPhase);
    bool hasAnyPlayers = false;
    for(std::shared_ptr<Player> p : world.players().lock())
    {
        p->checkAfterRead();
        hasAnyPlayers = true;
    }
    if(!hasAnyPlayers)
        throw stream::InvalidDataValueException("world has no players");

    for(std::size_t i = 0; i < ThreadCounts::get().lightingThreadCount; i++)
    {
        world.lightingThreads.emplace_back([&world]()
                                           {
                                               setThreadName(L"lighting");
                                               TLS tls;
                                               world.lightingThreadFn(tls);
                                           });
    }
    for(std::size_t i = 0; i < ThreadCounts::get().generateThreadCount; i++)
    {
        world.chunkGeneratingThreads.emplace_back([&world]()
                                                  {
                                                      setThreadName(L"chunk generate");
                                                      setThreadPriority(ThreadPriority::Low);
                                                      TLS tls;
                                                      world.chunkGeneratingThreadFn(nullptr, tls);
                                                  });
    }
    for(std::size_t i = 0; i < ThreadCounts::get().blockUpdateThreadCount; i++)
    {
        bool isPhaseManager = i == 0;
        world.blockUpdateThreads.emplace_back([&world, isPhaseManager]()
                                              {
                                                  setThreadName(L"block update");
                                                  TLS tls;
                                                  world.blockUpdateThreadFn(tls, isPhaseManager);
                                              });
    }
    world.particleGeneratingThread = thread([&world]()
                                            {
                                                setThreadName(L"particle generate");
                                                TLS tls;
                                                world.particleGeneratingThreadFn(tls);
                                            });
    world.moveEntitiesThread = thread([&world]()
                                      {
                                          setThreadName(L"move entities");
                                          TLS tls;
                                          world.moveEntitiesThreadFn(tls);
                                      });
    return retval;
}

bool World::isChunkCloseEnoughToPlayerToGetRandomUpdates(PositionI chunkBasePosition)
{
    LockedPlayers lockedPlayers = players().lock();
    for(std::shared_ptr<Player> player : lockedPlayers)
    {
        PositionI playerChunkBasePosition =
            BlockChunk::getChunkBasePosition((PositionI)player->getPosition());
        if(playerChunkBasePosition.d != chunkBasePosition.d)
            continue;
        VectorI displacement = playerChunkBasePosition - chunkBasePosition;
        VectorI v(std::abs(displacement.x), std::abs(displacement.y), std::abs(displacement.z));
        const int maxDistance = 16 * 7;
        if(v.x > maxDistance || v.y > maxDistance || v.z > maxDistance)
            continue;
        return true;
    }
    return false;
}

void World::chunkUnloaderThreadFn(TLS &tls) // this thread doesn't need to be paused
{
    FIXME_MESSAGE(finish fixing chunk unloader thread)
#if 0 // disable for now
#error add read/write block updates
    std::shared_ptr<ChunkCache> chunkCache = std::make_shared<ChunkCache>();
    auto reloadFn = [chunkCache, this](std::shared_ptr<BlockChunk> chunk)
    {
        try
        {
            TLS &tls = TLS::getSlow();
            getLoadIntoChunk() = chunk;
            std::vector<std::uint8_t> buffer;
            chunkCache->getChunk(chunk->basePosition, buffer);
            stream::MemoryReader readerIn(std::move(buffer));
            stream::ExpandReader reader(readerIn);
            setStreamFileVersion(reader, GameVersion::FILE_VERSION);
            WorldLockManager lock_manager(false, tls);
            StreamWorldGuard streamWorldGuard(reader, *this, lock_manager);
            BlockChunkMap *chunksMap = &physicsWorld->chunks;
            BlockIterator cbi(chunk, chunksMap, chunk->basePosition, VectorI(0, 0, 0));
            std::uint64_t entityCount = stream::read<std::uint64_t>(reader);
            for(std::uint64_t entityIndex = 0; entityIndex < entityCount; entityIndex++)
            {
                Entity *entity = stream::read<Entity>(reader);
                // read function already adds to world
#error fix
                ignore_unused_variable_warning(entity);
            }
            BlocksGenerateArray blocks;
            checked_array<checked_array<BiomeProperties, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> biomes;
            for(std::size_t x = 0; x < BlockChunk::chunkSizeX; x++)
            {
                for(std::size_t z = 0; z < BlockChunk::chunkSizeZ; z++)
                {
                    biomes[x][z] = stream::read<BiomeProperties>(reader);
                    for(std::size_t y = 0; y < BlockChunk::chunkSizeY; y++)
                    {
                        blocks[x][y][z] = stream::read<Block>(reader);
                    }
                }
            }
            for(int dx = 0; dx < BlockChunk::chunkSizeX; dx++)
            {
                for(int dz = 0; dz < BlockChunk::chunkSizeZ; dz++)
                {
                    BlockIterator bi = cbi;
                    bi.moveBy(VectorI(dx, 0, dz), lock_manager.tls);
                    bi.updateLock(lock_manager);
                    biomes[dx][dz].swap(bi.getBiome().biomeProperties);
                }
            }
            setBlockRange(cbi, chunk->basePosition, chunk->basePosition + VectorI(BlockChunk::chunkSizeX - 1, BlockChunk::chunkSizeY - 1, BlockChunk::chunkSizeZ - 1), lock_manager, blocks, VectorI(0));
            for(std::size_t x = 0; x < BlockChunk::chunkSizeX; x++)
            {
                for(std::size_t z = 0; z < BlockChunk::chunkSizeZ; z++)
                {
                    for(std::size_t y = 0; y < BlockChunk::chunkSizeY; y++)
                    {
                        blocks[x][y][z] = Block();
                    }
                }
            }
        }
        catch(...)
        {
            getLoadIntoChunk() = nullptr;
            throw;
        }
        getLoadIntoChunk() = nullptr;
    };
    IndirectBlockChunk::getIgnoreReferencesFromThreadFlag() = true;
    for(bool didAnything = false; !destructing; didAnything ? static_cast<void>(didAnything = false) : std::this_thread::sleep_for(std::chrono::milliseconds(10)))
    {
        BlockChunkMap *chunksMap = &physicsWorld->chunks;
        for(auto chunkIter = chunksMap->begin(); chunkIter != chunksMap->end(); chunkIter++)
        {
            IndirectBlockChunk &ibc = *chunkIter;
            if(chunkIter.is_locked())
                chunkIter.unlock();
            std::unique_lock<std::mutex> indirectBlockChunkLock(ibc.chunkLock);
            if(!ibc.chunk)
                continue;
            if(!ibc.chunk.unique()) // more than one reference : in use
                continue;
            if(ibc.accessedFlag.exchange(false, std::memory_order_relaxed)) // was accessed
                continue;
            if(!ibc.chunkVariables.generated)
                continue;
            std::atomic_bool unloadAbortFlag(false);
            std::shared_ptr<BlockChunk> chunk = ibc.chunk;
            indirectBlockChunkLock.unlock();
            BlockChunkFullLock blockChunkFullLock(*chunk);
            indirectBlockChunkLock.lock();
            ibc.unloadAbortFlag = &unloadAbortFlag;
            indirectBlockChunkLock.unlock();
            getDebugLog() << "unload " << ibc.basePosition << postnl;
            struct UnloadAbortedException final : public std::runtime_error
            {
                using runtime_error::runtime_error;
            };
            auto checkUnloadAborted = [&unloadAbortFlag]()
            {
                if(unloadAbortFlag.load(std::memory_order_relaxed))
                    throw UnloadAbortedException("unload aborted");
            };
            bool succeded = true;
            try
            {
                stream::MemoryWriter memWriter;
                {
                    WorldLockManager lock_manager(false, tls);
                    stream::CompressWriter writer(memWriter);
                    StreamWorldGuard streamWorldGuard(writer, *this, lock_manager);
                    std::vector<WrappedEntity *> entities;
                    {
                        std::unique_lock<std::recursive_mutex> lockChunk(chunk->getChunkVariables().entityListLock);
                        WrappedEntity::ChunkListType &chunkEntityList = chunk->getChunkVariables().entityList;
                        for(auto i = chunkEntityList.begin(); i != chunkEntityList.end(); ++i)
                        {
                            checkUnloadAborted();
                            WrappedEntity &entity = *i;
                            if(!entity.entity.good())
                            {
                                continue;
                            }
                            entities.push_back(&entity);
                        }
                    }
                    stream::write<std::uint64_t>(writer, entities.size());
                    for(WrappedEntity *entity : entities)
                    {
                        checkUnloadAborted();
                        entity->entity.write(writer);
                    }
                    entities.clear();
                    BlockIterator cbi(chunk, chunksMap, chunk->basePosition, VectorI(0, 0, 0));
                    for(std::size_t x = 0; x < BlockChunk::chunkSizeX; x++)
                    {
                        for(std::size_t z = 0; z < BlockChunk::chunkSizeZ; z++)
                        {
                            checkUnloadAborted();
                            BlockIterator columnBlockIterator = cbi;
                            columnBlockIterator.moveBy(VectorI(x, 0, z), lock_manager.tls);
                            stream::write<BiomeProperties>(writer, columnBlockIterator.getBiomeProperties(lock_manager));
                            BlockIterator bi = columnBlockIterator;
                            for(std::size_t y = 0; y < BlockChunk::chunkSizeY; y++, bi.moveTowardPY(lock_manager.tls))
                            {
                                stream::write<Block>(writer, bi.get(lock_manager));
                            }
                        }
                    }
                    writer.flush();
                }
                checkUnloadAborted();
                chunkCache->setChunk(chunk->basePosition, memWriter.getBuffer());
                chunk = nullptr;
                checkUnloadAborted();
            }
            catch(stream::IOException &e)
            {
                getDebugLog() << "unload of " << ibc.basePosition << " failed : " << e.what() << postnl;
                succeded = false;
            }
            catch(UnloadAbortedException &)
            {
                getDebugLog() << "unload of " << ibc.basePosition << " aborted" << postnl;
                succeded = false;
            }
            if(succeded)
            {
                succeded = ibc.setUnloaded(reloadFn);
                if(!succeded)
                {
                    getDebugLog() << "unload of " << ibc.basePosition << " aborted" << postnl;
                }
            }
            indirectBlockChunkLock.lock();
            ibc.unloadAbortFlag = nullptr;
            ibc.chunkCond.notify_all();
            indirectBlockChunkLock.unlock();
            if(succeded)
            {
                getDebugLog() << "unload of " << ibc.basePosition << " succeded" << postnl;
                didAnything = true;
            }
        }
    }
#endif
}
}
}
