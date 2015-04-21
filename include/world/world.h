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
#include "entity/entity.h"
#ifndef WORLD_H_INCLUDED
#define WORLD_H_INCLUDED

#include "physics/physics.h"
#include "util/position.h"
#include "render/mesh.h"
#include "render/render_layer.h"
#include "util/enum_traits.h"
#include <memory>
#include <atomic>
#include <array>
#include <mutex>
#include <cstdint>
#include <cwchar>
#include <string>
#include <thread>
#include <list>
#include "render/renderer.h"
#include "util/cached_variable.h"
#include "platform/platform.h"
#include "util/unlock_guard.h"
#include "util/linked_map.h"
#include "ray_casting/ray_casting.h"
#include "util/flag.h"
#include "util/spin_lock.h"
#include "util/parallel_map.h"
#include "generate/biome/biome_descriptor.h"
#include <algorithm>
#include <random>
#include "util/math_constants.h"

namespace programmerjake
{
namespace voxels
{
class WorldGenerator;
class ViewPoint;

class World final
{
    friend class ViewPoint;
    World(const World &) = delete;
    const World &operator =(const World &) = delete;
public:
    typedef std::uint64_t SeedType;
    static SeedType makeSeed(std::wstring seed)
    {
        SeedType retval = ((SeedType)0xF9CCC7E0 << 32) + 0x987EEEE5; // large prime number
        for(wchar_t ch : seed)
        {
            retval = 23 * retval + (SeedType)ch;
        }
        return retval;
    }
    SeedType getWorldGeneratorSeed() const
    {
        return worldGeneratorSeed;
    }
    static constexpr std::int32_t SeaLevel = 64;
private:
    std::mt19937 randomGenerator;
    std::mutex randomGeneratorLock;
public:
    struct WorldRandomNumberGenerator final
    {
        friend class World;
    private:
        World &world;
        WorldRandomNumberGenerator(World &world)
            : world(world)
        {
        }
    public:
        typedef std::mt19937::result_type result_type;
        static result_type min()
        {
            return std::mt19937::min();
        }
        static result_type max()
        {
            return std::mt19937::max();
        }
        result_type operator ()() const
        {
            std::lock_guard<std::mutex> lockIt(world.randomGeneratorLock);
            return world.randomGenerator();
        }
        void discard(unsigned long long count) const
        {
            std::lock_guard<std::mutex> lockIt(world.randomGeneratorLock);
            world.randomGenerator.discard(count);
        }
    };
private:
    WorldRandomNumberGenerator wrng;
public:
    WorldRandomNumberGenerator &getRandomGenerator()
    {
        return wrng;
    }
private:
    struct internal_construct_flag
    {
    };
    World(SeedType seed, const WorldGenerator *worldGenerator, internal_construct_flag);
public:
    /** @brief construct a world
     *
     * @param seed the world seed to use
     * @param worldGenerator the world generator to use
     *
     */
    World(SeedType seed, const WorldGenerator *worldGenerator);
    /** @brief construct a world
     *
     * @param seed the world seed to use
     * @param worldGenerator the world generator to use
     *
     */
    World(std::wstring seed, const WorldGenerator *worldGenerator)
        : World(makeSeed(seed), worldGenerator)
    {
    }
    /** @brief construct a world
     *
     * @param seed the world seed to use
     *
     */
    explicit World(SeedType seed);

    /** @brief construct a world
     *
     * @param seed the world seed to use
     *
     */
    explicit World(std::wstring seed)
        : World(makeSeed(seed))
    {
    }
    /** @brief construct a world
     *
     */
    explicit World();
    ~World();
    /** @brief get the current world time
     *
     * @return the current world time
     *
     */
    inline double getCurrentTime() const
    {
        return physicsWorld->getCurrentTime();
    }
    /** @brief reschedule a block update
     *
     * @param bi a <code>BlockIterator</code> to the block to reschedule the block update for
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param kind the kind of block update
     * @param updateTimeFromNow how far in the future to schedule the block update or (for values < 0) a flag to delete the block update
     * @return the time left for the previous block update or -1 if there is no previous block update
     *
     */
    float rescheduleBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind, float updateTimeFromNow)
    {
        BlockChunkBlock &b = bi.getBlock(lock_manager);
        BlockUpdate **ppnode = &b.updateListHead;
        BlockUpdate *pnode = b.updateListHead;
        std::unique_lock<decltype(bi.chunk->chunkVariables.blockUpdateListLock)> lockIt(bi.chunk->chunkVariables.blockUpdateListLock);
        float retval = -1;
        while(pnode != nullptr)
        {
            if(pnode->kind == kind)
            {
                retval = pnode->time_left;
                if(updateTimeFromNow < 0)
                {
                    *ppnode = pnode->block_next;
                    if(pnode->chunk_prev == nullptr)
                        bi.chunk->chunkVariables.blockUpdateListHead = pnode->chunk_next;
                    else
                        pnode->chunk_prev->chunk_next = pnode->chunk_next;
                    if(pnode->chunk_next == nullptr)
                        bi.chunk->chunkVariables.blockUpdateListTail = pnode->chunk_prev;
                    else
                        pnode->chunk_next->chunk_prev = pnode->chunk_prev;
                    delete pnode;
                }
                else
                {
                    pnode->time_left = updateTimeFromNow;
                }
                return retval;
            }
            ppnode = &pnode->block_next;
            pnode = *ppnode;
        }
        if(updateTimeFromNow >= 0)
        {
            pnode = new BlockUpdate(kind, bi.position(), updateTimeFromNow, b.updateListHead);
            b.updateListHead = pnode;
            pnode->chunk_next = bi.chunk->chunkVariables.blockUpdateListHead;
            pnode->chunk_prev = nullptr;
            if(bi.chunk->chunkVariables.blockUpdateListHead != nullptr)
                bi.chunk->chunkVariables.blockUpdateListHead->chunk_prev = pnode;
            else
                bi.chunk->chunkVariables.blockUpdateListTail = pnode;
            bi.chunk->chunkVariables.blockUpdateListHead = pnode;
        }
        return retval;
    }
    /** @brief add a block update
     *
     * @param bi a <code>BlockIterator</code> to the block to add the block update for
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param kind the kind of block update
     * @param updateTimeFromNow how far in the future to schedule the block update
     * @return if there is a previous block update
     *
     */
    bool addBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind, float updateTimeFromNow)
    {
        assert(updateTimeFromNow >= 0);
        BlockChunkBlock &b = bi.getBlock(lock_manager);
        BlockUpdate **ppnode = &b.updateListHead;
        BlockUpdate *pnode = b.updateListHead;
        std::unique_lock<decltype(bi.chunk->chunkVariables.blockUpdateListLock)> lockIt(bi.chunk->chunkVariables.blockUpdateListLock);
        while(pnode != nullptr)
        {
            if(pnode->kind == kind)
            {
                if(pnode->time_left > updateTimeFromNow)
                    pnode->time_left = updateTimeFromNow;
                return true;
            }
            ppnode = &pnode->block_next;
            pnode = *ppnode;
        }
        pnode = new BlockUpdate(kind, bi.position(), updateTimeFromNow, b.updateListHead);
        b.updateListHead = pnode;
        pnode->chunk_next = bi.chunk->chunkVariables.blockUpdateListHead;
        pnode->chunk_prev = nullptr;
        if(bi.chunk->chunkVariables.blockUpdateListHead != nullptr)
            bi.chunk->chunkVariables.blockUpdateListHead->chunk_prev = pnode;
        else
            bi.chunk->chunkVariables.blockUpdateListTail = pnode;
        bi.chunk->chunkVariables.blockUpdateListHead = pnode;
        return false;
    }
    /** @brief delete a block update
     *
     * @param bi a <code>BlockIterator</code> to the block to delete the block update for
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param kind the kind of block update to delete
     * @return the time left for the previous block update or -1 if there is no previous block update
     *
     */
    float deleteBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind)
    {
        return rescheduleBlockUpdate(bi, lock_manager, kind, -1);
    }
private:
    void invalidateBlock(BlockIterator bi, WorldLockManager &lock_manager)
    {
        BlockChunkBlock &b = bi.getBlock(lock_manager);
        b.invalidate();
        bi.getSubchunk().invalidate();
        bi.chunk->chunkVariables.invalidate();
        for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
        {
            float defaultPeriod = BlockUpdateKindDefaultPeriod(kind);
            if(defaultPeriod == 0)
                addBlockUpdate(bi, lock_manager, kind, defaultPeriod);
        }
    }
public:
    /** @brief set a block
     *
     * @param bi a <code>BlockIterator</code> to the block to set
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param newBlock the new block
     * @see setBlockRange
     * @see setBiomeProperties
     */
    void setBlock(BlockIterator bi, WorldLockManager &lock_manager, Block newBlock)
    {
        BlockChunkBlock &b = bi.getBlock(lock_manager);
        Block block = (Block)b.block;
        if(block.good() && block.descriptor->generatesParticles())
        {
            bi.getSubchunk().removeParticleGeneratingBlock(bi.position());
        }
        b.block = (PackedBlock)newBlock;
        block = newBlock;
        if(block.good() && block.descriptor->generatesParticles())
        {
            bi.getSubchunk().addParticleGeneratingBlock(bi.position());
        }
        lightingStable = false;
        for(int dx = -1; dx <= 1; dx++)
        {
            for(int dy = -1; dy <= 1; dy++)
            {
                for(int dz = -1; dz <= 1; dz++)
                {
                    BlockIterator bi2 = bi;
                    bi2.moveBy(VectorI(dx, dy, dz));
                    invalidateBlock(std::move(bi2), lock_manager);
                }
            }
        }
        for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
        {
            float defaultPeriod = BlockUpdateKindDefaultPeriod(kind);
            if(defaultPeriod > 0)
                addBlockUpdate(bi, lock_manager, kind, defaultPeriod);
        }
    }
    /** @brief set a block column's biome properties
     *
     * @param bi a <code>BlockIterator</code> to the block column to set
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param newBiomeProperties the new biome properties
     * @see setBlock
     */
    void setBiomeProperties(BlockIterator bi, WorldLockManager &lock_manager, BiomeProperties newBiomeProperties)
    {
        bi.updateLock(lock_manager);
        BlockChunkBiome &b = bi.getBiome();
        b.biomeProperties = std::move(newBiomeProperties);
        lightingStable = false;
        PositionI pos = bi.position();
        pos.y = 0;
        for(std::int32_t y = 0; y < BlockChunk::chunkSizeY; y++)
        {
            for(int dx = -1; dx <= 1; dx++)
            {
                for(int dz = -1; dz <= 1; dz++)
                {
                    BlockIterator bi2 = bi;
                    bi2.moveTo(pos + VectorI(dx, y, dz));
                    invalidateBlock(std::move(bi2), lock_manager);
                }
            }
        }
    }
    /** @brief set a block range
     *
     * except for the order of setting blocks, this is equivalent to:
     * @code
     * template <typename T>
     * void setBlockRange(PositionI minCorner, VectorI maxCorner, WorldLockManager &lock_manager, const T &newBlocks, VectorI newBlocksOrigin)
     * {
     *     for(PositionI pos = minCorner; pos.x <= maxCorner.x; pos.x++)
     *     {
     *         for(pos.y = minCorner.y; pos.y <= maxCorner.y; pos.y++)
     *         {
     *             for(pos.z = minCorner.z; pos.z <= maxCorner.z; pos.z++)
     *             {
     *                 BlockIterator bi = getBlockIterator(pos);
     *                 VectorI newBlocksPosition = pos - minCorner + newBlocksOrigin;
     *                 Block newBlock = newBlocks[newBlocksPosition.x][newBlocksPosition.y][newBlocksPosition.z];
     *                 setBlock(bi, lock_manager, newBlock);
     *             }
     *         }
     *     }
     * }
     * @endcode
     *
     * @param minCorner the minimum corner of the block range to set. sets blocks in the range minCorner.x <= x <= maxCorner.x, minCorner.y <= y <= maxCorner.y, minCorner.z <= z <= maxCorner.z
     * @param maxCorner the maximum corner of the block range to set. sets blocks in the range minCorner.x <= x <= maxCorner.x, minCorner.y <= y <= maxCorner.y, minCorner.z <= z <= maxCorner.z
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param newBlocks the array of new blocks
     * @param newBlocksOrigin the minimum corner in the array of new blocks
     *
     *
     */
    template <typename T>
    void setBlockRange(PositionI minCorner, VectorI maxCorner, WorldLockManager &lock_manager, const T &newBlocks, VectorI newBlocksOrigin)
    {
        PositionI adjustedMinCorner = minCorner - VectorI(1);
        VectorI adjustedMaxCorner = maxCorner + VectorI(1);
        PositionI minCornerSubchunk = BlockChunk::getSubchunkBaseAbsolutePosition(minCorner);
        VectorI maxCornerSubchunk = BlockChunk::getSubchunkBaseAbsolutePosition(maxCorner);
        for(PositionI subchunkPos = minCornerSubchunk; subchunkPos.x <= maxCornerSubchunk.x; subchunkPos.x += BlockChunk::subchunkSizeXYZ)
        {
            for(subchunkPos.y = minCornerSubchunk.y; subchunkPos.y <= maxCornerSubchunk.y; subchunkPos.y += BlockChunk::subchunkSizeXYZ)
            {
                for(subchunkPos.z = minCornerSubchunk.z; subchunkPos.z <= maxCornerSubchunk.z; subchunkPos.z += BlockChunk::subchunkSizeXYZ)
                {
                    VectorI minSubchunkRelativePos = VectorI(0);
                    VectorI maxSubchunkRelativePos = VectorI(BlockChunk::subchunkSizeXYZ - 1);
                    minSubchunkRelativePos.x = std::max(minSubchunkRelativePos.x, minCorner.x - subchunkPos.x);
                    minSubchunkRelativePos.y = std::max(minSubchunkRelativePos.y, minCorner.y - subchunkPos.y);
                    minSubchunkRelativePos.z = std::max(minSubchunkRelativePos.z, minCorner.z - subchunkPos.z);
                    maxSubchunkRelativePos.x = std::min(maxSubchunkRelativePos.x, maxCorner.x - subchunkPos.x);
                    maxSubchunkRelativePos.y = std::min(maxSubchunkRelativePos.y, maxCorner.y - subchunkPos.y);
                    maxSubchunkRelativePos.z = std::min(maxSubchunkRelativePos.z, maxCorner.z - subchunkPos.z);
                    BlockIterator sbi = getBlockIterator(subchunkPos);
                    for(VectorI subchunkRelativePos = minSubchunkRelativePos; subchunkRelativePos.x <= maxSubchunkRelativePos.x; subchunkRelativePos.x++)
                    {
                        for(subchunkRelativePos.y = minSubchunkRelativePos.y; subchunkRelativePos.y <= maxSubchunkRelativePos.y; subchunkRelativePos.y++)
                        {
                            for(subchunkRelativePos.z = minSubchunkRelativePos.z; subchunkRelativePos.z <= maxSubchunkRelativePos.z; subchunkRelativePos.z++)
                            {
                                BlockIterator bi = sbi;
                                bi.moveBy(subchunkRelativePos);
                                VectorI newBlocksPosition = subchunkRelativePos + subchunkPos - minCorner + newBlocksOrigin;
                                Block newBlock = newBlocks[newBlocksPosition.x][newBlocksPosition.y][newBlocksPosition.z];
                                BlockChunkBlock &b = bi.getBlock(lock_manager);
                                Block block = (Block)b.block;
                                if(block.good() && block.descriptor->generatesParticles())
                                {
                                    bi.getSubchunk().removeParticleGeneratingBlock(bi.position());
                                }
                                b.block = (PackedBlock)newBlock;
                                block = newBlock;
                                if(block.good() && block.descriptor->generatesParticles())
                                {
                                    bi.getSubchunk().addParticleGeneratingBlock(bi.position());
                                }
                                invalidateBlock(bi, lock_manager);
                                for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
                                {
                                    float defaultPeriod = BlockUpdateKindDefaultPeriod(kind);
                                    if(defaultPeriod > 0)
                                        rescheduleBlockUpdate(bi, lock_manager, kind, defaultPeriod);
                                }
                            }
                        }
                    }
                    lightingStable = false;
                }
            }
        }
        minCornerSubchunk = BlockChunk::getSubchunkBaseAbsolutePosition(adjustedMinCorner);
        maxCornerSubchunk = BlockChunk::getSubchunkBaseAbsolutePosition(adjustedMaxCorner);
        for(PositionI subchunkPos = minCornerSubchunk; subchunkPos.x <= maxCornerSubchunk.x; subchunkPos.x += BlockChunk::subchunkSizeXYZ)
        {
            for(subchunkPos.y = minCornerSubchunk.y; subchunkPos.y <= maxCornerSubchunk.y; subchunkPos.y += BlockChunk::subchunkSizeXYZ)
            {
                for(subchunkPos.z = minCornerSubchunk.z; subchunkPos.z <= maxCornerSubchunk.z; subchunkPos.z += BlockChunk::subchunkSizeXYZ)
                {
                    VectorI minSubchunkRelativePos = VectorI(0);
                    VectorI maxSubchunkRelativePos = VectorI(BlockChunk::subchunkSizeXYZ - 1);
                    minSubchunkRelativePos.x = std::max(minSubchunkRelativePos.x, adjustedMinCorner.x - subchunkPos.x);
                    minSubchunkRelativePos.y = std::max(minSubchunkRelativePos.y, adjustedMinCorner.y - subchunkPos.y);
                    minSubchunkRelativePos.z = std::max(minSubchunkRelativePos.z, adjustedMinCorner.z - subchunkPos.z);
                    maxSubchunkRelativePos.x = std::min(maxSubchunkRelativePos.x, adjustedMaxCorner.x - subchunkPos.x);
                    maxSubchunkRelativePos.y = std::min(maxSubchunkRelativePos.y, adjustedMaxCorner.y - subchunkPos.y);
                    maxSubchunkRelativePos.z = std::min(maxSubchunkRelativePos.z, adjustedMaxCorner.z - subchunkPos.z);
                    if(minSubchunkRelativePos.x > adjustedMinCorner.x - subchunkPos.x && maxSubchunkRelativePos.x < adjustedMaxCorner.x - subchunkPos.x &&
                       minSubchunkRelativePos.y > adjustedMinCorner.y - subchunkPos.y && maxSubchunkRelativePos.y < adjustedMaxCorner.y - subchunkPos.y &&
                       minSubchunkRelativePos.z > adjustedMinCorner.z - subchunkPos.z && maxSubchunkRelativePos.z < adjustedMaxCorner.z - subchunkPos.z)
                        continue;
                    BlockIterator sbi = getBlockIterator(subchunkPos);
                    for(VectorI subchunkRelativePos = minSubchunkRelativePos; subchunkRelativePos.x <= maxSubchunkRelativePos.x; subchunkRelativePos.x++)
                    {
                        for(subchunkRelativePos.y = minSubchunkRelativePos.y; subchunkRelativePos.y <= maxSubchunkRelativePos.y; subchunkRelativePos.y++)
                        {
                            for(subchunkRelativePos.z = minSubchunkRelativePos.z; subchunkRelativePos.z <= maxSubchunkRelativePos.z; subchunkRelativePos.z++)
                            {
                                PositionI pos = subchunkRelativePos + subchunkPos;
                                if(pos.x > adjustedMinCorner.x && pos.x < adjustedMaxCorner.x &&
                                   pos.x > adjustedMinCorner.y && pos.y < adjustedMaxCorner.y &&
                                   pos.x > adjustedMinCorner.z && pos.z < adjustedMaxCorner.z)
                                    continue;
                                BlockIterator bi = sbi;
                                bi.moveBy(subchunkRelativePos);
                                invalidateBlock(bi, lock_manager);
                            }
                        }
                    }
                    lightingStable = false;
                }
            }
        }
    }
    /** @brief create a <code>BlockIterator</code>
     *
     * @param position the position to create a <code>BlockIterator</code> for
     * @return the new <code>BlockIterator</code>
     *
     */
    BlockIterator getBlockIterator(PositionI position) const
    {
        return physicsWorld->getBlockIterator(position);
    }
    /** @brief add a new entity to this world
     *
     * @param descriptor the <code>EntityDescriptor</code> for the new entity
     * @param position the position to create the new entity at
     * @param position the velocity for the new entity
     *
     */
    Entity *addEntity(EntityDescriptorPointer descriptor, PositionF position, VectorF velocity, WorldLockManager &lock_manager, std::shared_ptr<void> entityData = nullptr);
    RayCasting::Collision castRay(RayCasting::Ray ray, WorldLockManager &lock_manager, float maxSearchDistance, RayCasting::BlockCollisionMask blockRayCollisionMask = RayCasting::BlockCollisionMaskDefault, const Entity *ignoreEntity = nullptr);
    void move(double deltaTime, WorldLockManager &lock_manager);
private:
    std::uint64_t entityRunCount = 0; // number of times all entities have moved
    std::shared_ptr<PhysicsWorld> physicsWorld;
    const WorldGenerator *worldGenerator;
    SeedType worldGeneratorSeed;
    std::list<std::thread> lightingThreads;
    std::list<std::thread> blockUpdateThreads;
    std::thread particleGeneratingThread;
    std::thread moveEntitiesThread;
    std::list<std::thread> chunkGeneratingThreads;
    std::atomic_bool destructing, lightingStable;
    std::mutex viewPointsLock;
    std::list<ViewPoint *> viewPoints;
    std::condition_variable moveEntitiesThreadCond;
    std::mutex moveEntitiesThreadLock;
    bool waitingForMoveEntities = false;
    double moveEntitiesDeltaTime = 1 / 60.0;
    void lightingThreadFn();
    void blockUpdateThreadFn();
    void generateChunk(BlockChunk *chunk, WorldLockManager &lock_manager);
    struct InitialChunkGenerateStruct final
    {
        std::mutex lock;
        std::condition_variable initialGenerationDoneCond;
        std::condition_variable generatorWaitDoneCond;
        std::size_t count;
        bool generatorWait = true;
        explicit InitialChunkGenerateStruct(std::size_t count)
            : count(count)
        {
        }
    };
    void chunkGeneratingThreadFn(std::shared_ptr<InitialChunkGenerateStruct> initialChunkGenerateStruct);
    void particleGeneratingThreadFn();
    void moveEntitiesThreadFn();
    static BlockUpdate *removeAllBlockUpdatesInChunk(BlockUpdateKind kind, BlockIterator bi, WorldLockManager &lock_manager);
    static BlockUpdate *removeAllReadyBlockUpdatesInChunk(BlockIterator bi, WorldLockManager &lock_manager);
    static Lighting getBlockLighting(BlockIterator bi, WorldLockManager &lock_manager, bool isTopFace);
    float getChunkGeneratePriority(BlockIterator bi, WorldLockManager &lock_manager); /// low values mean high priority, NAN means don't generate
    bool isInitialGenerateChunk(PositionI position);
    RayCasting::Collision castRayCheckForEntitiesInSubchunk(BlockIterator sbi, RayCasting::Ray ray, WorldLockManager &lock_manager, float maxSearchDistance, const Entity *ignoreEntity);
    void generateParticlesInSubchunk(BlockIterator bi, WorldLockManager &lock_manager, double currentTime, double deltaTime, std::vector<PositionI> &positions);
    std::recursive_mutex timeOfDayLock;
    float timeOfDayInSeconds = 0.0;
    int moonPhase = 0;
public:
    static constexpr float dayDurationInSeconds = 1200.0f;
    static constexpr int moonPhaseCount = 8;
    bool isLightingStable()
    {
        return lightingStable;
    }
    float getTimeOfDayInSeconds()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds;
    }
    float getTimeOfDayInDays()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds / dayDurationInSeconds;
    }
    static constexpr float timeOfDayDayStart = 0;
    static constexpr float timeOfDayDuskStart = 600.0f;
    static constexpr float timeOfDayNightStart = 690.0f;
    static constexpr float timeOfDayDawnStart = 1110.0f;
    bool isDaytime()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayDayStart && timeOfDayInSeconds < timeOfDayDuskStart;
    }
    bool isDusk()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayDuskStart && timeOfDayInSeconds < timeOfDayNightStart;
    }
    bool isNight()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayNightStart && timeOfDayInSeconds < timeOfDayDawnStart;
    }
    bool isDawn()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayDawnStart && timeOfDayInSeconds < dayDurationInSeconds;
    }
    float getLightingParameter()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        float t = 1;
        if(timeOfDayInSeconds < timeOfDayDuskStart)
        {
            t = 1;
        }
        else if(timeOfDayInSeconds < timeOfDayNightStart)
        {
            t = 1 - (timeOfDayInSeconds - timeOfDayDuskStart) / (timeOfDayNightStart - timeOfDayDuskStart);
        }
        else if(timeOfDayInSeconds < timeOfDayDawnStart)
        {
            t = 0;
        }
        else
        {
            t = (timeOfDayInSeconds - timeOfDayDawnStart) / (dayDurationInSeconds - timeOfDayDawnStart);
        }
        t = limit(t, 0.0f, 1.0f);
        return t;
    }
    WorldLightingProperties getLighting(Dimension d)
    {
        float t = interpolate<float>(getLightingParameter(), getNightSkyBrightnessLevel(d), getDaySkyBrightnessLevel(d));
        t = (std::floor(16 * t) + 0.01f) / 15;
        t = limit(t, 0.0f, 1.0f);
        return WorldLightingProperties(t, d);
    }
    void setTimeOfDayInSeconds(float v)
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = v - dayDurationInSeconds * std::floor(v / dayDurationInSeconds);
    }
    void setTimeOfDayInDays(float v)
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = dayDurationInSeconds * (v - std::floor(v));
    }
    void setTimeOfDayInSeconds(float v, int newMoonPhase)
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = v - dayDurationInSeconds * std::floor(v / dayDurationInSeconds);
        moonPhase = newMoonPhase % moonPhaseCount;
        if(moonPhase < 0)
            moonPhase += moonPhaseCount;
    }
    void setTimeOfDayInDays(float v, int newMoonPhase)
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = dayDurationInSeconds * (v - std::floor(v));
        moonPhase = newMoonPhase % moonPhaseCount;
        if(moonPhase < 0)
            moonPhase += moonPhaseCount;
    }
    void advanceTimeOfDay(double deltaTime) /// @param deltaTime the time to advance in seconds
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        double newTimeOfDayInSeconds = timeOfDayInSeconds + deltaTime;
        double newTimeInDays = newTimeOfDayInSeconds / dayDurationInSeconds;
        timeOfDayInSeconds = dayDurationInSeconds * (newTimeInDays - std::floor(newTimeInDays));
        moonPhase = (moonPhase + (int)std::floor(newTimeInDays)) % moonPhaseCount;
    }
    int getMoonPhase()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return moonPhase;
    }
    void getTimeOfDayInSeconds(float &timeOfDayInSeconds, int &moonPhase)
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = this->timeOfDayInSeconds;
        moonPhase = this->moonPhase;
    }
    int getVisibleMoonPhase()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        int retval = moonPhase;
        if(timeOfDayInSeconds < 300.0f) // switch phases at noon
            retval = (retval + moonPhaseCount - 1) % moonPhaseCount;
        return retval;
    }
    bool canSleepInBed()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= 627.05f && timeOfDayInSeconds <= 1172.9f;
    }
    bool handleSleepInBed()
    {
        std::unique_lock<std::recursive_mutex> lockIt(timeOfDayLock);
        if(!canSleepInBed())
            return false;
        advanceTimeOfDay(dayDurationInSeconds - timeOfDayInSeconds);
    }
    VectorF getSunPosition()
    {
        float angle = M_PI * 2 * getTimeOfDayInDays();
        return VectorF(std::cos(angle), std::sin(angle), 0);
    }
    VectorF getMoonPosition()
    {
        return -getSunPosition();
    }
    ColorF getSkyColor(PositionF pos)
    {
        return interpolate(getLightingParameter(), getNightSkyColor(pos.d), getDaySkyColor(pos.d));
    }
    float getMoonFullness()
    {
        int phase = getVisibleMoonPhase();
        return 1.0f - 2.0f / (float)moonPhaseCount * std::abs(phase - moonPhaseCount / 2);
    }
};
}
}

#include "world/world_generator.h"

#endif // WORLD_H_INCLUDED
