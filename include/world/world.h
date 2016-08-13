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
#include "util/lock.h"
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
#include <stdexcept>
#include "util/math_constants.h"
#include "stream/stream.h"
#include "util/rc4_random_engine.h"
#include "util/util.h"
#include "util/tls.h"
#include "util/world_lock_manager.h"

namespace programmerjake
{
namespace voxels
{
class WorldGenerator;
class ViewPoint;
class PlayerList;

struct WorldConstructionAborted final : public std::runtime_error
{
    WorldConstructionAborted() : std::runtime_error("World construction aborted")
    {
    }
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
class World final : public std::enable_shared_from_this<World>
{
    GCC_PRAGMA(diagnostic pop)
    friend class ViewPoint;
    friend class WorldLock;
    World(const World &) = delete;
    const World &operator=(const World &) = delete;

private:
    // private types
    struct internal_construct_flag
    {
    };
    struct stream_world_tag_t final
    {
    };
    class StreamWorldGuard final
    {
        StreamWorldGuard(const StreamWorldGuard &) = delete;
        StreamWorldGuard &operator=(const StreamWorldGuard &) = delete;

    private:
        stream::Stream &stream;

    public:
        StreamWorldGuard(stream::Stream &stream, World &world, WorldLockManager &lock_manager)
            : stream(stream)
        {
            setStreamWorld(stream, StreamWorld(world, lock_manager));
        }
        ~StreamWorldGuard()
        {
            setStreamWorld(stream, StreamWorld());
        }
    };
    struct InitialChunkGenerateStruct final
    {
        InitialChunkGenerateStruct(const InitialChunkGenerateStruct &) = delete;
        InitialChunkGenerateStruct &operator=(const InitialChunkGenerateStruct &) = delete;
        Mutex lock;
        ConditionVariable initialGenerationDoneCond;
        ConditionVariable generatorWaitDoneCond;
        std::size_t count;
        bool generatorWait = true;
        std::atomic_bool *abortFlag;
        explicit InitialChunkGenerateStruct(std::size_t count, std::atomic_bool *abortFlag)
            : lock(),
              initialGenerationDoneCond(),
              generatorWaitDoneCond(),
              count(count),
              abortFlag(abortFlag)
        {
        }
    };
    struct ThreadPauseGuard final
    {
        ThreadPauseGuard(const ThreadPauseGuard &) = delete;
        ThreadPauseGuard &operator=(const ThreadPauseGuard &) = delete;

    private:
        World &world;

    public:
        explicit ThreadPauseGuard(World &world) : world(world)
        {
            std::unique_lock<Mutex> lockIt(world.stateLock);
            while(world.isPaused && !world.destructing)
            {
                world.stateCond.wait(lockIt);
            }
            world.unpausedThreadCount++;
        }
        ~ThreadPauseGuard()
        {
            std::unique_lock<Mutex> lockIt(world.stateLock);
            world.unpausedThreadCount--;
            world.stateCond.notify_all();
        }
        void checkForPause(std::unique_lock<Mutex> &lockedStateLock)
        {
            if(world.isPaused && !world.destructing)
            {
                world.unpausedThreadCount--;
                world.stateCond.notify_all();
                while(world.isPaused && !world.destructing)
                {
                    world.stateCond.wait(lockedStateLock);
                }
                world.unpausedThreadCount++;
            }
        }
        void checkForPause()
        {
            std::unique_lock<Mutex> lockedStateLock(world.stateLock);
            checkForPause(lockedStateLock);
        }
    };

public:
    // public types
    typedef std::uint64_t SeedType;
    struct WorldRandomNumberGenerator final
    {
        friend class World;

    private:
        World &world;
        WorldRandomNumberGenerator(World &world) : world(world)
        {
        }

    public:
        typedef rc4_random_engine::result_type result_type;
        static result_type min()
        {
            return rc4_random_engine::min();
        }
        static result_type max()
        {
            return rc4_random_engine::max();
        }
        result_type operator()() const
        {
            std::lock_guard<Mutex> lockIt(world.randomGeneratorLock);
            return world.randomGenerator();
        }
        void discard(unsigned long long count) const
        {
            std::lock_guard<Mutex> lockIt(world.randomGeneratorLock);
            world.randomGenerator.discard(count);
        }
    };
    class StreamWorld final
    {
    private:
        World *pworld = nullptr;
        WorldLockManager *plock_manager = nullptr;

    public:
        StreamWorld()
        {
        }
        StreamWorld(World &world_, WorldLockManager &lock_manager_)
            : pworld(&world_), plock_manager(&lock_manager_)
        {
        }
        StreamWorld(const StreamWorld &) = default;
        StreamWorld &operator=(const StreamWorld &) = default;
        StreamWorld(StreamWorld &&) = default;
        StreamWorld &operator=(StreamWorld &&) = default;
        ~StreamWorld() = default;
        bool good() const
        {
            return pworld != nullptr && plock_manager != nullptr;
        }
        explicit operator bool() const
        {
            return good();
        }
        bool operator!() const
        {
            return !good();
        }
        World &world() const
        {
            assert(good());
            return *pworld;
        }
        std::shared_ptr<World> shared_world() const
        {
            assert(good());
            return pworld->shared_from_this();
        }
        WorldLockManager &lock_manager() const
        {
            assert(good());
            return *plock_manager;
        }
    };

public:
    // public variables
    static constexpr std::int32_t SeaLevel = 64;
    static constexpr float dayDurationInSeconds = 1200.0f;
    static constexpr int moonPhaseCount = 8;
    static constexpr float timeOfDayDayStart = 0;
    static constexpr float timeOfDayDuskStart = 600.0f;
    static constexpr float timeOfDayNightStart = 690.0f;
    static constexpr float timeOfDayDawnStart = 1110.0f;

public:
    // public functions
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
    static StreamWorld getStreamWorld(stream::Stream &stream)
    {
        std::shared_ptr<StreamWorld> retval =
            stream.getAssociatedValue<StreamWorld, stream_world_tag_t>();
        if(retval == nullptr)
            return StreamWorld();
        return *retval;
    }
    WorldRandomNumberGenerator &getRandomGenerator()
    {
        return wrng;
    }
    World(SeedType seed, const WorldGenerator *worldGenerator, internal_construct_flag);
    /** @brief construct a world
     *
     * @param seed the world seed to use
     * @param worldGenerator the world generator to use
     *
     */
    World(SeedType seed,
          const WorldGenerator *worldGenerator,
          std::atomic_bool *abortFlag = nullptr);
    /** @brief construct a world
     *
     * @param seed the world seed to use
     * @param worldGenerator the world generator to use
     *
     */
    World(std::wstring seed,
          const WorldGenerator *worldGenerator,
          std::atomic_bool *abortFlag = nullptr)
        : World(makeSeed(seed), worldGenerator, abortFlag)
    {
    }
    /** @brief construct a world
     *
     * @param seed the world seed to use
     *
     */
    explicit World(SeedType seed, std::atomic_bool *abortFlag = nullptr);

    /** @brief construct a world
     *
     * @param seed the world seed to use
     *
     */
    explicit World(std::wstring seed, std::atomic_bool *abortFlag = nullptr)
        : World(makeSeed(seed), abortFlag)
    {
    }
    /** @brief construct a world
     *
     */
    explicit World(std::atomic_bool *abortFlag = nullptr);
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
     * @param updateTimeFromNow how far in the future to schedule the block update or (for values <
     *0) a flag to delete the block update
     * @return the time left for the previous block update or -1 if there is no previous block
     *update
     *
     */
    float rescheduleBlockUpdate(BlockIterator bi,
                                WorldLockManager &lock_manager,
                                BlockUpdateKind kind,
                                float updateTimeFromNow)
    {
        BlockChunkBlock &block = bi.getBlock();
        if(updateTimeFromNow >= 0)
        {
            if(block.extraData == nullptr)
            {
                block.extraData.reset(new BlockChunkBlockExtraData(bi.position()));
            }
        }
        else if(block.extraData == nullptr)
        {
            return -1;
        }
        BlockUpdate &blockUpdate = block.extraData->blockUpdates[kind];
        if(blockUpdate.isEmpty(*bi.chunk))
        {
            if(updateTimeFromNow >= 0)
            {
                blockUpdate.kind = kind;
                blockUpdate.position = bi.position();
                blockUpdate.timeLeft = updateTimeFromNow;
                bi.chunk->blockUpdates.push_back(&blockUpdate);
            }
            return -1;
        }
        else
        {
            float retval = blockUpdate.timeLeft;
            if(updateTimeFromNow < 0)
            {
                bi.chunk->blockUpdates.detach(bi.chunk->blockUpdates.to_iterator(&blockUpdate));
                if(block.extraData->isEmpty(*bi.chunk))
                    block.extraData.reset();
            }
            else
            {
                blockUpdate.timeLeft = updateTimeFromNow;
            }
            return retval;
        }
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
    bool addBlockUpdate(BlockIterator bi,
                        WorldLockManager &lock_manager,
                        BlockUpdateKind kind,
                        float updateTimeFromNow)
    {
        assert(updateTimeFromNow >= 0);
        BlockChunkBlock &block = bi.getBlock();
        if(block.extraData == nullptr)
            block.extraData.reset(new BlockChunkBlockExtraData(bi.position()));
        BlockUpdate &blockUpdate = block.extraData->blockUpdates[kind];
        if(blockUpdate.isEmpty(*bi.chunk))
        {
            blockUpdate.kind = kind;
            blockUpdate.position = bi.position();
            blockUpdate.timeLeft = updateTimeFromNow;
            bi.chunk->blockUpdates.push_back(&blockUpdate);
            return false;
        }
        else
        {
            blockUpdate.timeLeft = updateTimeFromNow;
            return true;
        }
    }
    /** @brief delete a block update
     *
     * @param bi a <code>BlockIterator</code> to the block to delete the block update for
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param kind the kind of block update to delete
     * @return the time left for the previous block update or -1 if there is no previous block
     *update
     *
     */
    float deleteBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind)
    {
        return rescheduleBlockUpdate(bi, lock_manager, kind, -1);
    }

private:
    template <bool removeEmptyExtraData>
    void setBlockNoUpdate(BlockIterator bi, WorldLockManager &lock_manager, Block newBlock)
    {
        BlockChunkBlock &b = bi.getBlock();
        if(b.blockKind != nullptr && b.blockKind->generatesParticles())
        {
            bi.chunk->particleGeneratingBlockList.erase(
                bi.chunk->particleGeneratingBlockList.to_iterator(b.extraData.get()));
        }
        b.blockKind = BlockDescriptorIndex(newBlock.descriptor);
        b.setLighting(newBlock.lighting);
        if(newBlock.data || b.extraData)
        {
            if(!b.extraData)
                b.extraData.reset(new BlockChunkBlockExtraData(bi.position()));
            b.extraData->data = std::move(newBlock.data);
        }
        if(newBlock.descriptor != nullptr && newBlock.descriptor->generatesParticles())
        {
            if(!b.extraData)
                b.extraData.reset(new BlockChunkBlockExtraData(bi.position()));
            bi.chunk->particleGeneratingBlockList.push_back(b.extraData.get());
        }
        if(removeEmptyExtraData && b.extraData->isEmpty(*bi.chunk))
            b.extraData.reset();
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
        setBlockNoUpdate<false>(bi, lock_manager, std::move(newBlock));
        lightingStable = false;
        for(int dx = -1; dx <= 1; dx++)
        {
            for(int dy = -1; dy <= 1; dy++)
            {
                for(int dz = -1; dz <= 1; dz++)
                {
                    BlockIterator bi2 = bi;
                    bi2.moveBy(VectorI(dx, dy, dz), lock_manager);
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
    void setBiomeProperties(BlockIterator bi,
                            WorldLockManager &lock_manager,
                            BiomeProperties newBiomeProperties)
    {
        bi.getColumn().biomeProperties = std::move(newBiomeProperties);
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
                    bi2.moveTo(pos + VectorI(dx, y, dz), lock_manager);
                    invalidateBlock(std::move(bi2), lock_manager);
                }
            }
        }
    }
    template <bool doInvalidate = true, typename T>
    void setBiomePropertiesRange(PositionI minCorner,
                                 VectorI maxCorner,
                                 WorldLockManager &lock_manager,
                                 const T &newBiomes,
                                 VectorI newBiomesOrigin)
    {
        minCorner.y = 0;
        maxCorner.y = BlockChunk::chunkSizeY - 1;
        VectorI adjustedMinCorner = minCorner - VectorI(1, 0, 1);
        VectorI adjustedMaxCorner = maxCorner + VectorI(1, 0, 1);
        BlockIterator biX(minCorner, lock_manager);
        for(VectorI p = minCorner; p.x <= maxCorner.x; p.x++, biX.moveTowardPX(lock_manager))
        {
            BlockIterator biXZ = biX;
            for(p.z = minCorner.z; p.z <= maxCorner.z; p.z++, biXZ.moveTowardPZ(lock_manager))
            {
                BlockChunkColumn &column = biXZ.getColumn();
                VectorI inputP = p - minCorner + newBiomesOrigin;
                column.biomeProperties = newBiomes[inputP.x][inputP.z];
            }
        }
        lightingStable = false;
        if(doInvalidate)
            invalidateBlockRange(biX, adjustedMinCorner, adjustedMaxCorner, lock_manager);
    }
    /** @brief set a block range
     *
     * except for the order of setting blocks, this is equivalent to:
     * @code
     * template <typename T>
     * void setBlockRange(PositionI minCorner, VectorI maxCorner, WorldLockManager &lock_manager,
     *const T &newBlocks, VectorI newBlocksOrigin)
     * {
     *     for(PositionI pos = minCorner; pos.x <= maxCorner.x; pos.x++)
     *     {
     *         for(pos.y = minCorner.y; pos.y <= maxCorner.y; pos.y++)
     *         {
     *             for(pos.z = minCorner.z; pos.z <= maxCorner.z; pos.z++)
     *             {
     *                 BlockIterator bi = getBlockIterator(pos);
     *                 VectorI newBlocksPosition = pos - minCorner + newBlocksOrigin;
     *                 Block newBlock =
     *newBlocks[newBlocksPosition.x][newBlocksPosition.y][newBlocksPosition.z];
     *                 setBlock(bi, lock_manager, newBlock);
     *             }
     *         }
     *     }
     * }
     * @endcode
     *
     * @param blockIterator the BlockIterator to use
     * @param minCorner the minimum corner of the block range to set. sets blocks in the range
     *minCorner.x <= x <= maxCorner.x, minCorner.y <= y <= maxCorner.y, minCorner.z <= z <=
     *maxCorner.z
     * @param maxCorner the maximum corner of the block range to set. sets blocks in the range
     *minCorner.x <= x <= maxCorner.x, minCorner.y <= y <= maxCorner.y, minCorner.z <= z <=
     *maxCorner.z
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param newBlocks the array of new blocks
     * @param newBlocksOrigin the minimum corner in the array of new blocks
     *
     *
     */
    template <typename T>
    void setBlockRange(BlockIterator blockIterator,
                       PositionI minCorner,
                       VectorI maxCorner,
                       WorldLockManager &lock_manager,
                       const T &newBlocks,
                       VectorI newBlocksOrigin)
    {
        PositionI adjustedMinCorner = minCorner - VectorI(1);
        VectorI adjustedMaxCorner = maxCorner + VectorI(1);
        PositionI minCornerChunk = BlockChunk::getChunkBasePosition(minCorner);
        VectorI maxCornerChunk = BlockChunk::getChunkBasePosition(maxCorner);
        for(PositionI chunkPos = minCornerChunk; chunkPos.y <= maxCornerChunk.y;
            chunkPos.y += BlockChunk::chunkSizeY)
        {
            for(chunkPos.x = minCornerChunk.x; chunkPos.x <= maxCornerChunk.x;
                chunkPos.x += BlockChunk::chunkSizeX)
            {
                for(chunkPos.z = minCornerChunk.z; chunkPos.z <= maxCornerChunk.z;
                    chunkPos.z += BlockChunk::chunkSizeZ)
                {
                    VectorI minChunkRelativePos = VectorI(0);
                    VectorI maxChunkRelativePos = VectorI(BlockChunk::chunkSizeX - 1,
                                                          BlockChunk::chunkSizeY - 1,
                                                          BlockChunk::chunkSizeZ - 1);
                    minChunkRelativePos.x =
                        std::max(minChunkRelativePos.x, minCorner.x - chunkPos.x);
                    minChunkRelativePos.y =
                        std::max(minChunkRelativePos.y, minCorner.y - chunkPos.y);
                    minChunkRelativePos.z =
                        std::max(minChunkRelativePos.z, minCorner.z - chunkPos.z);
                    maxChunkRelativePos.x =
                        std::min(maxChunkRelativePos.x, maxCorner.x - chunkPos.x);
                    maxChunkRelativePos.y =
                        std::min(maxChunkRelativePos.y, maxCorner.y - chunkPos.y);
                    maxChunkRelativePos.z =
                        std::min(maxChunkRelativePos.z, maxCorner.z - chunkPos.z);
                    BlockIterator cbi = blockIterator;
                    cbi.moveTo(chunkPos, lock_manager);
                    for(VectorI chunkRelativePos = minChunkRelativePos;
                        chunkRelativePos.y <= maxChunkRelativePos.y;
                        chunkRelativePos.y++)
                    {
                        for(chunkRelativePos.x = minChunkRelativePos.x;
                            chunkRelativePos.x <= maxChunkRelativePos.x;
                            chunkRelativePos.x++)
                        {
                            for(chunkRelativePos.z = minChunkRelativePos.z;
                                chunkRelativePos.z <= maxChunkRelativePos.z;
                                chunkRelativePos.z++)
                            {
                                BlockIterator bi = cbi;
                                bi.moveBy(chunkRelativePos, lock_manager);
                                VectorI newBlocksPosition =
                                    chunkRelativePos + chunkPos - minCorner + newBlocksOrigin;
                                Block newBlock =
                                    newBlocks[newBlocksPosition.x][newBlocksPosition
                                                                       .y][newBlocksPosition.z];
                                setBlockNoUpdate<false>(bi, lock_manager, std::move(newBlock));
                                for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
                                {
                                    float defaultPeriod = BlockUpdateKindDefaultPeriod(kind);
                                    if(defaultPeriod > 0)
                                        rescheduleBlockUpdate(
                                            bi, lock_manager, kind, defaultPeriod);
                                }
                            }
                        }
                    }
                    lightingStable = false;
                }
            }
        }
        invalidateBlockRange(blockIterator, adjustedMinCorner, adjustedMaxCorner, lock_manager);
        lightingStable = false;
    }
    /** @brief add a new entity to this world
     *
     * @param descriptor the <code>EntityDescriptor</code> for the new entity
     * @param position the position to create the new entity at
     * @param position the velocity for the new entity
     *
     */
    Entity *addEntity(EntityDescriptorPointer descriptor,
                      PositionF position,
                      VectorF velocity,
                      WorldLockManager &lock_manager,
                      std::shared_ptr<void> entityData = nullptr);
    RayCasting::Collision castRay(RayCasting::Ray ray,
                                  WorldLockManager &lock_manager,
                                  float maxSearchDistance,
                                  RayCasting::BlockCollisionMask blockRayCollisionMask =
                                      RayCasting::BlockCollisionMaskDefault,
                                  const Entity *ignoreEntity = nullptr);
    void move(double deltaTime, WorldLockManager &lock_manager);
    bool isLightingStable()
    {
        return lightingStable;
    }
    float getTimeOfDayInSeconds()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds;
    }
    float getTimeOfDayInDays()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds / dayDurationInSeconds;
    }
    bool isDaytime()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayDayStart && timeOfDayInSeconds < timeOfDayDuskStart;
    }
    bool isDusk()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayDuskStart && timeOfDayInSeconds < timeOfDayNightStart;
    }
    bool isNight()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayNightStart && timeOfDayInSeconds < timeOfDayDawnStart;
    }
    bool isDawn()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= timeOfDayDawnStart
               && timeOfDayInSeconds < dayDurationInSeconds;
    }
    float getLightingParameter()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        float t = 1;
        if(timeOfDayInSeconds < timeOfDayDuskStart)
        {
            t = 1;
        }
        else if(timeOfDayInSeconds < timeOfDayNightStart)
        {
            t = 1
                - (timeOfDayInSeconds - timeOfDayDuskStart)
                      / (timeOfDayNightStart - timeOfDayDuskStart);
        }
        else if(timeOfDayInSeconds < timeOfDayDawnStart)
        {
            t = 0;
        }
        else
        {
            t = (timeOfDayInSeconds - timeOfDayDawnStart)
                / (dayDurationInSeconds - timeOfDayDawnStart);
        }
        t = limit(t, 0.0f, 1.0f);
        return t;
    }
    WorldLightingProperties getLighting(Dimension d)
    {
        float t = interpolate<float>(
            getLightingParameter(), getNightSkyBrightnessLevel(d), getDaySkyBrightnessLevel(d));
        t = (std::floor(16 * t) + 0.01f) / 15;
        t = limit(t, 0.0f, 1.0f);
        return WorldLightingProperties(t, d);
    }
    void setTimeOfDayInSeconds(float v)
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = v - dayDurationInSeconds * std::floor(v / dayDurationInSeconds);
    }
    void setTimeOfDayInDays(float v)
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = dayDurationInSeconds * (v - std::floor(v));
    }
    void setTimeOfDayInSeconds(float v, int newMoonPhase)
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = v - dayDurationInSeconds * std::floor(v / dayDurationInSeconds);
        moonPhase = newMoonPhase % moonPhaseCount;
        if(moonPhase < 0)
            moonPhase += moonPhaseCount;
    }
    void setTimeOfDayInDays(float v, int newMoonPhase)
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = dayDurationInSeconds * (v - std::floor(v));
        moonPhase = newMoonPhase % moonPhaseCount;
        if(moonPhase < 0)
            moonPhase += moonPhaseCount;
    }
    void advanceTimeOfDay(double deltaTime) /// @param deltaTime the time to advance in seconds
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        double newTimeOfDayInSeconds = timeOfDayInSeconds + deltaTime;
        double newTimeInDays = newTimeOfDayInSeconds / dayDurationInSeconds;
        timeOfDayInSeconds =
            dayDurationInSeconds * static_cast<float>(newTimeInDays - std::floor(newTimeInDays));
        moonPhase = (moonPhase + (int)std::floor(newTimeInDays)) % moonPhaseCount;
    }
    int getMoonPhase()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return moonPhase;
    }
    void getTimeOfDayInSeconds(float &timeOfDayInSeconds, int &moonPhase)
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        timeOfDayInSeconds = this->timeOfDayInSeconds;
        moonPhase = this->moonPhase;
    }
    int getVisibleMoonPhase()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        int retval = moonPhase;
        if(timeOfDayInSeconds < 300.0f) // switch phases at noon
            retval = (retval + moonPhaseCount - 1) % moonPhaseCount;
        return retval;
    }
    bool canSleepInBed()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        return timeOfDayInSeconds >= 627.05f && timeOfDayInSeconds <= 1172.9f;
    }
    bool handleSleepInBed()
    {
        std::unique_lock<RecursiveMutex> lockIt(timeOfDayLock);
        if(!canSleepInBed())
            return false;
        advanceTimeOfDay(dayDurationInSeconds - timeOfDayInSeconds);
        return true;
    }
    VectorF getSunPosition()
    {
        float angle = static_cast<float>(M_PI) * 2 * getTimeOfDayInDays();
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
    PlayerList &players()
    {
        return *playerList;
    }
    bool paused()
    {
        std::unique_lock<Mutex> lockIt(stateLock);
        return isPaused;
    }
    void paused(bool newPaused, WorldLockManager &lockManager)
    {
        std::unique_lock<Mutex> lockIt(stateLock);
        if(newPaused == isPaused)
        {
            return;
        }
        lockManager.clearAndCheckLocks();
        isPaused = newPaused;
        stateCond.notify_all();
        if(isPaused)
        {
            while(unpausedThreadCount > 0)
            {
                stateCond.wait(lockIt);
            }
        }
    }
    void write(stream::Writer &writer, WorldLockManager &lock_manager);
    static std::shared_ptr<World> read(stream::Reader &reader);
    static std::uint32_t getStreamFileVersion(stream::Reader &reader);

private:
    // private variables
    rc4_random_engine randomGenerator;
    Mutex randomGeneratorLock;
    WorldRandomNumberGenerator wrng;
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
    Mutex viewPointsLock;
    std::list<ViewPoint *> viewPoints;
    bool waitingForMoveEntities = false;
    double moveEntitiesDeltaTime = 1 / 60.0;
    RecursiveMutex timeOfDayLock;
    float timeOfDayInSeconds = 0.0;
    int moonPhase = 0;
    std::shared_ptr<PlayerList> playerList;
    Mutex stateLock;
    ConditionVariable stateCond;
    bool isPaused = false;
    std::size_t unpausedThreadCount = 0;
    std::thread chunkUnloaderThread;
    BlockUpdatePhase blockUpdateCurrentPhase;
    std::size_t blockUpdateCurrentPhaseCount;
    std::size_t blockUpdateNextPhaseCount;
    bool blockUpdateDidAnything;
    // private functions
    void lightingThreadFn(TLS &tls);
    void blockUpdateThreadFn(TLS &tls, bool isPhaseManager);
    void generateChunk(std::shared_ptr<BlockChunk> chunk,
                       WorldLockManager &lock_manager,
                       const std::atomic_bool *abortFlag,
                       std::unique_ptr<ThreadPauseGuard> &pauseGuard);
    void chunkGeneratingThreadFn(
        std::shared_ptr<InitialChunkGenerateStruct> initialChunkGenerateStruct, TLS &tls);
    void particleGeneratingThreadFn(TLS &tls);
    void moveEntitiesThreadFn(TLS &tls);
    static BlockUpdate *removeAllBlockUpdatesInChunk(BlockUpdateKind kind,
                                                     BlockIterator bi,
                                                     WorldLockManager &lock_manager);
    static BlockUpdate *removeAllReadyBlockUpdatesInChunk(BlockUpdatePhase phase,
                                                          BlockIterator bi,
                                                          WorldLockManager &lock_manager);
    static Lighting getBlockLighting(BlockIterator bi,
                                     WorldLockManager &lock_manager,
                                     bool isTopFace);
    float getChunkGeneratePriority(
        BlockIterator bi,
        WorldLockManager &lock_manager); /// low values mean high priority, NAN means don't generate
    bool isInitialGenerateChunk(PositionI position);
    RayCasting::Collision castRayCheckForEntitiesInSubchunk(BlockIterator sbi,
                                                            RayCasting::Ray ray,
                                                            WorldLockManager &lock_manager,
                                                            float maxSearchDistance,
                                                            const Entity *ignoreEntity);
    void generateParticlesInSubchunk(BlockIterator bi,
                                     WorldLockManager &lock_manager,
                                     double currentTime,
                                     double deltaTime,
                                     std::vector<PositionI> &positions);
    static void setStreamWorld(stream::Stream &stream, StreamWorld streamWorld)
    {
        std::shared_ptr<StreamWorld> p =
            stream.getAssociatedValue<StreamWorld, stream_world_tag_t>();
        if(p == nullptr && streamWorld)
        {
            stream.setAssociatedValue<StreamWorld, stream_world_tag_t>(
                std::make_shared<StreamWorld>(streamWorld));
        }
        else if(p != nullptr && !streamWorld)
        {
            stream.setAssociatedValue<StreamWorld, stream_world_tag_t>(nullptr);
        }
        else if(p != nullptr)
        {
            *p = streamWorld;
        }
    }
    void invalidateBlock(BlockIterator bi, WorldLockManager &lock_manager)
    {
        bi.chunk->invalidate();
        for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
        {
            float defaultPeriod = BlockUpdateKindDefaultPeriod(kind);
            if(defaultPeriod == 0)
                addBlockUpdate(bi, lock_manager, kind, defaultPeriod);
        }
    }
    void invalidateBlockRange(BlockIterator bi,
                              VectorI minCorner,
                              VectorI maxCorner,
                              WorldLockManager &lock_manager);
    bool isChunkCloseEnoughToPlayerToGetRandomUpdates(PositionI chunkBasePosition);
    void chunkUnloaderThreadFn(TLS &tls);
    BlockIterator getBlockIteratorForWorldAddEntity(PositionI pos, TLS &tls);
};
}
}

#include "world/world_generator.h"

#endif // WORLD_H_INCLUDED
