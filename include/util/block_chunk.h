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
#ifndef BLOCK_CHUNK_H_INCLUDED
#define BLOCK_CHUNK_H_INCLUDED

#include "util/position.h"
#include "stream/stream.h"
#include "stream/compressed_stream.h"
#include "render/mesh.h"
#include "render/render_layer.h"
#include "util/enum_traits.h"
#include <mutex>
#include "lighting/lighting.h"
#include <array>
#include <memory>
#include <atomic>
#include <iterator>
#include "util/atomic_shared_ptr.h"
#include "util/lock.h"
#include <cstdint>
#include "generate/biome/biome.h"
#include "block/block_struct.h"
#include <condition_variable>
#include "util/linked_map.h"
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include "util/tls.h"
#include "util/util.h"
#include "util/semaphore.h"

//#define USE_SEMAPHORE_FOR_BLOCK_CHUNK

namespace programmerjake
{
namespace voxels
{
struct BlockChunkSubchunk;
struct BlockChunk;
}
}

#include "util/wrapped_entity.h"

namespace programmerjake
{
namespace voxels
{
class BlockUpdate final
{
    friend class World;
    friend class BlockUpdateIterator;
    friend struct BlockChunkChunkVariables;

private:
    BlockUpdate *chunk_prev = nullptr;
    BlockUpdate *chunk_next = nullptr;
    BlockUpdate *block_next = nullptr;
    PositionI position = PositionI();
    float time_left = 0;
    BlockUpdateKind kind = BlockUpdateKind();
    BlockUpdate()
    {
    }
    void init(BlockUpdateKind kind, PositionI position, float time_left, BlockUpdate *block_next)
    {
        this->chunk_prev = nullptr;
        this->chunk_next = nullptr;
        this->block_next = block_next;
        this->position = position;
        this->time_left = time_left;
        this->kind = kind;
    }
    BlockUpdate(const BlockUpdate &) = delete;
    const BlockUpdate &operator=(const BlockUpdate &) = delete;
    ~BlockUpdate()
    {
    }
    struct FreeList final
    {
        BlockUpdate *head = nullptr;
        std::size_t size = 0;
        ~FreeList()
        {
            while(head != nullptr)
            {
                BlockUpdate *deleteMe = head;
                head = head->block_next;
                delete deleteMe;
            }
        }
    };
    struct freeList_tls_tag
    {
    };
    static void allocateAndFree(BlockUpdate *blockUpdateToFree,
                                BlockUpdate **pBlockUpdateToAllocate,
                                TLS &tls)
    {
        thread_local_variable<FreeList, freeList_tls_tag> freeList(tls);
        if(pBlockUpdateToAllocate)
        {
            if(blockUpdateToFree)
            {
                *pBlockUpdateToAllocate = blockUpdateToFree;
            }
            else if(freeList.get().head == nullptr)
            {
                *pBlockUpdateToAllocate = new BlockUpdate;
            }
            else
            {
                *pBlockUpdateToAllocate = freeList.get().head;
                freeList.get().head = freeList.get().head->block_next;
                freeList.get().size--;
            }
        }
        else if(blockUpdateToFree)
        {
            if(freeList.get().size > static_cast<std::size_t>(1 << 20) / sizeof(BlockUpdate))
            {
                delete blockUpdateToFree;
            }
            else
            {
                blockUpdateToFree->block_next = freeList.get().head;
                freeList.get().head = blockUpdateToFree;
                freeList.get().size++;
            }
        }
    }
    static BlockUpdate *allocate(TLS &tls,
                                 BlockUpdateKind kind,
                                 PositionI position,
                                 float time_left,
                                 BlockUpdate *block_next = nullptr)
    {
        BlockUpdate *retval;
        allocateAndFree(nullptr, &retval, tls);
        retval->init(kind, position, time_left, block_next);
        return retval;
    }
    static void free(BlockUpdate *v, TLS &tls)
    {
        allocateAndFree(v, nullptr, tls);
    }

public:
    PositionI getPosition() const
    {
        return position;
    }
    float getTimeLeft() const
    {
        return time_left;
    }
    BlockUpdateKind getKind() const
    {
        return kind;
    }
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
class BlockUpdateIterator final : public std::iterator<std::forward_iterator_tag, const BlockUpdate>
{
    GCC_PRAGMA(diagnostic pop)
private:
    friend class BlockIterator;
    friend class World;
    BlockUpdate *node;

public:
    BlockUpdate &operator*()
    {
        return *node;
    }
    BlockUpdate *operator->()
    {
        return node;
    }

private:
    explicit BlockUpdateIterator(BlockUpdate *node) : node(node)
    {
    }

public:
    BlockUpdateIterator() : node(nullptr)
    {
    }
    BlockUpdateIterator operator++(int)
    {
        BlockUpdateIterator retval = *this;
        node = node->block_next;
        return retval;
    }
    const BlockUpdateIterator &operator++()
    {
        node = node->block_next;
        return *this;
    }
    const BlockUpdate &operator*() const
    {
        return *node;
    }
    const BlockUpdate *operator->() const
    {
        return node;
    }
    bool operator==(const BlockUpdateIterator &r) const
    {
        return node == r.node;
    }
    bool operator!=(const BlockUpdateIterator &r) const
    {
        return node != r.node;
    }
};

struct BlockChunkBlock final
{
    typedef Lighting::LightValueType LightValueType;
    static constexpr int lightBitWidth = Lighting::lightBitWidth;
    LightValueType directSkylight : lightBitWidth;
    LightValueType indirectSkylight : lightBitWidth;
    LightValueType indirectArtificalLight : lightBitWidth;
    bool hasOptionalData : 1;
    static constexpr int blockKindBitWidth = 32 - 1 - 3 * lightBitWidth;
    std::uint32_t blockKind : blockKindBitWidth;
    BlockChunkBlock()
        : directSkylight(0),
          indirectSkylight(0),
          indirectArtificalLight(0),
          hasOptionalData(false),
          blockKind((static_cast<std::uint32_t>(1) << blockKindBitWidth) - 1)
    {
    }
    void invalidate()
    {
    }
    const BlockChunkBlock &operator=(const BlockChunkBlock &) = delete;
    Lighting getLighting() const
    {
        return Lighting(
            directSkylight, indirectSkylight, indirectArtificalLight, Lighting::MakeDirectOnly);
    }
    void setLighting(Lighting l)
    {
        directSkylight = l.directSkylight;
        indirectSkylight = l.indirectSkylight;
        indirectArtificalLight = l.indirectArtificalLight;
    }
};

class BlockOptionalDataHashTable;

constexpr int BlockChunkShiftX = 4;
constexpr int BlockChunkShiftY = 8;
constexpr int BlockChunkShiftZ = 4;
constexpr std::int32_t BlockChunkSizeX = 1 << BlockChunkShiftX;
constexpr std::int32_t BlockChunkSizeY = 1 << BlockChunkShiftY;
constexpr std::int32_t BlockChunkSizeZ = 1 << BlockChunkShiftZ;

struct BlockChunkBiome final
{
    BiomeProperties biomeProperties;
    BlockChunkBiome() : biomeProperties()
    {
    }
};

typedef std::uint64_t BlockChunkInvalidateCountType;
struct BlockChunk final
{
    static constexpr int chunkShiftX = 4;
    static constexpr int chunkShiftY = 8;
    static constexpr int chunkShiftZ = 4;
    static constexpr std::int32_t chunkSizeX = 1 << chunkShiftX;
    static constexpr std::int32_t chunkSizeY = 1 << chunkShiftY;
    static constexpr std::int32_t chunkSizeZ = 1 << chunkShiftZ;
    static constexpr PositionI getChunkBasePosition(PositionI pos)
    {
        return PositionI(
            pos.x & ~(chunkSizeX - 1), pos.y & ~(chunkSizeY - 1), pos.z & ~(chunkSizeZ - 1), pos.d);
    }
    static constexpr VectorI getChunkBasePosition(VectorI pos)
    {
        return VectorI(
            pos.x & ~(chunkSizeX - 1), pos.y & ~(chunkSizeY - 1), pos.z & ~(chunkSizeZ - 1));
    }
    static constexpr PositionI getChunkRelativePosition(PositionI pos)
    {
        return PositionI(
            pos.x & (chunkSizeX - 1), pos.y & (chunkSizeY - 1), pos.z & (chunkSizeZ - 1), pos.d);
    }
    static constexpr VectorI getChunkRelativePosition(VectorI pos)
    {
        return VectorI(
            pos.x & (chunkSizeX - 1), pos.y & (chunkSizeY - 1), pos.z & (chunkSizeZ - 1));
    }
    class BlocksArray final
    {
    private:
        BlockChunkBlock blocks[chunkSizeX * chunkSizeY * chunkSizeZ];
        static constexpr std::size_t getIndex(std::size_t indexX,
                                              std::size_t indexY,
                                              std::size_t indexZ) noexcept
        {
            assert(indexX < static_cast<std::size_t>(chunkSizeX)
                   && indexY < static_cast<std::size_t>(chunkSizeY)
                   && indexZ < static_cast<std::size_t>(chunkSizeZ));
            return indexZ + (indexX << chunkShiftZ) + (indexY << (chunkShiftZ + chunkShiftX));
        }

    public:
        class IndexHelper1;
        class IndexHelper2 final
        {
            friend class IndexHelper1;

        private:
            BlockChunkBlock *blocks;
            std::size_t indexX;
            std::size_t indexY;
            IndexHelper2(BlockChunkBlock *blocks, std::size_t indexX, std::size_t indexY)
                : blocks(blocks), indexX(indexX), indexY(indexY)
            {
            }

        public:
            BlockChunkBlock &operator[](std::size_t indexZ)
            {
                return blocks[getIndex(indexX, indexY, indexZ)];
            }
            BlockChunkBlock &at(std::size_t index)
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size()
            {
                return chunkSizeZ;
            }
        };
        class IndexHelper1 final
        {
            friend class BlocksArray;

        private:
            BlockChunkBlock *blocks;
            std::size_t indexX;
            IndexHelper1(BlockChunkBlock *blocks, std::size_t indexX)
                : blocks(blocks), indexX(indexX)
            {
            }

        public:
            IndexHelper2 operator[](std::size_t indexY)
            {
                return IndexHelper2(blocks, indexX, indexY);
            }
            IndexHelper2 at(std::size_t index)
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size()
            {
                return chunkSizeY;
            }
        };
        class ConstIndexHelper1;
        class ConstIndexHelper2 final
        {
            friend class ConstIndexHelper1;

        private:
            const BlockChunkBlock *blocks;
            std::size_t indexX;
            std::size_t indexY;
            ConstIndexHelper2(const BlockChunkBlock *blocks, std::size_t indexX, std::size_t indexY)
                : blocks(blocks), indexX(indexX), indexY(indexY)
            {
            }

        public:
            const BlockChunkBlock &operator[](std::size_t indexZ)
            {
                return blocks[getIndex(indexX, indexY, indexZ)];
            }
            const BlockChunkBlock &at(std::size_t index)
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size()
            {
                return chunkSizeZ;
            }
        };
        class ConstIndexHelper1 final
        {
            friend class BlocksArray;

        private:
            const BlockChunkBlock *blocks;
            std::size_t indexX;
            ConstIndexHelper1(const BlockChunkBlock *blocks, std::size_t indexX)
                : blocks(blocks), indexX(indexX)
            {
            }

        public:
            ConstIndexHelper2 operator[](std::size_t indexY)
            {
                return ConstIndexHelper2(blocks, indexX, indexY);
            }
            ConstIndexHelper2 at(std::size_t index)
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size()
            {
                return chunkSizeY;
            }
        };
        ConstIndexHelper1 operator[](std::size_t indexX) const
        {
            return ConstIndexHelper1(blocks, indexX);
        }
        IndexHelper1 operator[](std::size_t indexX)
        {
            return IndexHelper1(blocks, indexX);
        }
        ConstIndexHelper1 at(std::size_t index) const
        {
            assert(index < size());
            return operator[](index);
        }
        IndexHelper1 at(std::size_t index)
        {
            assert(index < size());
            return operator[](index);
        }
        static std::size_t size()
        {
            return chunkSizeX;
        }
    };
    typedef std::mutex LockType;
    LockType lock;
    checked_array<checked_array<BlockChunkBiome, chunkSizeZ>, chunkSizeX> biomes;
    BlocksArray blocks;
    BlockOptionalDataHashTable blockOptionalData;
};

struct BlockChunkSubchunk final
{
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
    class LockImp final
    {
        std::mutex lockImp;
        std::shared_ptr<Semaphore> chunkSemaphore;
        friend struct BlockChunk;
        friend struct BlockChunkSubchunk;

    public:
        LockImp(std::shared_ptr<Semaphore> chunkSemaphore)
            : lockImp(), chunkSemaphore(std::move(chunkSemaphore))
        {
        }
        void lock()
        {
            assert(chunkSemaphore);
            lockImp.lock();
            try
            {
                chunkSemaphore->lock();
            }
            catch(...)
            {
                lockImp.unlock();
                throw;
            }
        }
        bool try_lock()
        {
            assert(chunkSemaphore);
            if(!lockImp.try_lock())
            {
                return false;
            }
            bool retval;
            try
            {
                retval = chunkSemaphore->try_lock();
            }
            catch(...)
            {
                lockImp.unlock();
                throw;
            }
            if(!retval)
                lockImp.unlock();
            return retval;
        }
        void unlock()
        {
            assert(chunkSemaphore);
            lockImp.unlock();
            chunkSemaphore->unlock();
        }
    };
#else
    typedef std::mutex LockImp;
#endif
    LockImp lockImp;
    generic_lock_wrapper lock;
    BlockOptionalDataHashTable blockOptionalData;
    std::vector<BlockDescriptorIndex> blockKinds;
    std::unordered_map<BlockDescriptorIndex, unsigned> blockKindsMap;
    BlockDescriptorPointer getBlockKind(const BlockChunkBlock &block) const
    {
        if(blockKinds.empty() || block.blockKind >= blockKinds.size())
            return nullptr;
        return blockKinds[block.blockKind].get();
    }
    void setBlockKind(BlockChunkBlock &block, BlockDescriptorPointer bd)
    {
        BlockDescriptorIndex bdi(bd);
        if(blockKinds.size()
           >= static_cast<std::size_t>(1)
                  << BlockChunkBlock::blockKindBitWidth) // every block kind is different : so we
        // can just replace the old block kind
        // because this is the only block using it
        {
            blockKinds[block.blockKind] = bdi;
            // don't maintain blockKindsMap anymore
            return;
        }
        auto iter = blockKindsMap.find(bdi);
        if(iter != blockKindsMap.end())
        {
            block.blockKind = std::get<1>(*iter);
            return;
        }
        if(blockKinds.capacity() <= blockKinds.size())
            blockKinds.reserve(blockKinds.size()
                               + 8); // don't use vector's default step to save space

        block.blockKind = blockKinds.size();
        blockKinds.push_back(bdi);
        if(blockKinds.size() >= static_cast<std::size_t>(1) << BlockChunkBlock::blockKindBitWidth)
        {
            blockKindsMap.clear(); // don't maintain blockKindsMap anymore
        }
        else
        {
            blockKindsMap.emplace(bdi, static_cast<unsigned>(block.blockKind));
        }
    }
    atomic_shared_ptr<enum_array<Mesh, RenderLayer>> cachedMeshes;
    std::atomic_bool generatingCachedMeshes;
    std::atomic_bool cachedMeshesInvalidated;
    WrappedEntity::SubchunkListType entityList;
    BlockChunkInvalidateCountType invalidateCount = 0;
    BlockChunkInvalidateCountType cachedMeshesInvalidateCount = 0;
    linked_map<PositionI, char> particleGeneratingSet; /// holds positions of blocks in this
    /// subchunk that generate particles
    void addParticleGeneratingBlock(PositionI position) /// must be locked first
    {
        particleGeneratingSet[position] = '\0';
    }
    void removeParticleGeneratingBlock(PositionI position) /// must be locked first
    {
        particleGeneratingSet.erase(position);
        if(particleGeneratingSet.empty())
            particleGeneratingSet.clear(); // frees more memory
    }
    void addToParticleGeneratingBlockList(
        std::vector<PositionI> &dest) const /// must be locked first
    {
        dest.reserve(dest.size() + particleGeneratingSet.size());
        for(auto v : particleGeneratingSet)
        {
            dest.push_back(std::get<0>(v));
        }
    }
    BlockChunkSubchunk(const BlockChunkSubchunk &rt)
        : lockImp(
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
              rt.lockImp.chunkSemaphore
#endif
              ),
          lock(lockImp),
          blockOptionalData(rt.blockOptionalData),
          blockKinds(rt.blockKinds),
          blockKindsMap(rt.blockKindsMap),
          cachedMeshes(nullptr),
          generatingCachedMeshes(false),
          cachedMeshesInvalidated(true),
          entityList([](WrappedEntity *)
                     {
                     }),
          particleGeneratingSet()
    {
    }
    explicit BlockChunkSubchunk(
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
        std::shared_ptr<Semaphore> chunkSemaphore = nullptr
#endif
        )
        : lockImp(
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
              std::move(chunkSemaphore)
#endif
                  ),
          lock(lockImp),
          blockOptionalData(),
          blockKinds(),
          blockKindsMap(),
          cachedMeshes(nullptr),
          generatingCachedMeshes(false),
          cachedMeshesInvalidated(true),
          entityList([](WrappedEntity *)
                     {
                     }),
          particleGeneratingSet()
    {
    }
    void invalidate()
    {
        cachedMeshesInvalidated = true;
        invalidateCount++;
    }
};

struct BlockChunkChunkVariables final
{
    BlockChunkChunkVariables &operator=(const BlockChunkChunkVariables &) = delete;
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
    const std::shared_ptr<Semaphore> semaphore;
#endif
    std::mutex cachedMeshesLock;
    std::shared_ptr<enum_array<Mesh, RenderLayer>> cachedMeshes;
    bool generatingCachedMeshes = false;
    std::condition_variable cachedMeshesCond;
    std::atomic_bool cachedMeshesUpToDate;
    WorldLightingProperties wlp;
    std::mutex blockUpdateListLock;
    BlockUpdate *blockUpdateListHead = nullptr;
    BlockUpdate *blockUpdateListTail = nullptr;
    enum_array<std::size_t, BlockUpdatePhase> blockUpdatesPerPhase;
    std::chrono::steady_clock::time_point lastBlockUpdateTime;
    bool lastBlockUpdateTimeValid = false;
    std::recursive_mutex entityListLock;
    WrappedEntity::ChunkListType entityList;
    std::atomic_bool generated, generateStarted;
    ~BlockChunkChunkVariables();
    void invalidate()
    {
        cachedMeshesUpToDate = false;
    }
    BlockChunkChunkVariables(const BlockChunkChunkVariables &rt)
        : BlockChunkChunkVariables(
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
              rt.semaphore
#endif
              )
    {
    }
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
    explicit
#endif
        BlockChunkChunkVariables(
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
            std::shared_ptr<Semaphore> semaphore
#endif
            )
        :
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
          semaphore(std::move(semaphore)),
#endif
          cachedMeshesLock(),
          cachedMeshes(nullptr),
          cachedMeshesCond(),
          cachedMeshesUpToDate(false),
          wlp(),
          blockUpdateListLock(),
          blockUpdatesPerPhase(),
          lastBlockUpdateTime(),
          entityListLock(),
          entityList([](WrappedEntity *v)
                     {
                         delete v;
                     }),
          generated(false),
          generateStarted(false)
    {
    }
};

class IndirectBlockChunk;

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
struct BlockChunk final
    : public BasicBlockChunk<BlockChunkBlock, BlockChunkBiome, BlockChunkSubchunk>
{
    GCC_PRAGMA(diagnostic pop)
    BlockChunk(const BlockChunk &) = delete;
    BlockChunk &operator=(const BlockChunk &) = delete;
    static_assert(BlockChunkBlock::blockKindBitWidth >= 3 * subchunkShiftXYZ,
                  "BlockChunkBlock::blockKindBitWidth is too small");
    static_assert(BlockOptionalData::BlockChunkSubchunkShiftXYZ == subchunkShiftXYZ,
                  "BlockOptionalData::BlockChunkSubchunkShiftXYZ is wrong value");
    ObjectCounter<BlockChunk, 0> objectCounter;
    explicit BlockChunk(PositionI basePosition, IndirectBlockChunk *indirectBlockChunk);
    ~BlockChunk();
    static Block getBlockFromArray(VectorI subchunkRelativePosition,
                                   const BlockChunkBlock &blockChunkBlock,
                                   BlockChunkSubchunk &subchunk)
    {
        BlockDescriptorPointer bd = subchunk.getBlockKind(blockChunkBlock);
        Lighting lighting = blockChunkBlock.getLighting();
        return Block(bd,
                     lighting,
                     blockChunkBlock.hasOptionalData ?
                         subchunk.blockOptionalData.getData(subchunkRelativePosition) :
                         BlockDataPointer<BlockData>(nullptr));
    }
    static void putBlockIntoArray(VectorI subchunkRelativePosition,
                                  BlockChunkBlock &blockChunkBlock,
                                  BlockChunkSubchunk &subchunk,
                                  Block newBlock,
                                  TLS &tls) /// @note doesn't handle updates or particles or
    /// anything except copying the members from Block
    {
        subchunk.setBlockKind(blockChunkBlock, newBlock.descriptor);
        blockChunkBlock.setLighting(newBlock.lighting);
        blockChunkBlock.hasOptionalData = subchunk.blockOptionalData.setData(
            subchunkRelativePosition, std::move(newBlock.data), tls);
    }
    IndirectBlockChunk *const indirectBlockChunk;
    BlockChunkChunkVariables &getChunkVariables();
};

class World;

class IndirectBlockChunk final
{
    friend class World;
    IndirectBlockChunk(const IndirectBlockChunk &) = delete;
    IndirectBlockChunk &operator=(const IndirectBlockChunk &) = delete;

private:
    std::shared_ptr<BlockChunk> chunk;
    std::function<void(std::shared_ptr<BlockChunk> chunk)> loadFn;
    bool loading = false;
    std::condition_variable chunkCond;
    std::mutex chunkLock;
    bool setUnloaded(
        std::function<void(std::shared_ptr<BlockChunk> chunk)> newLoadFn) // called by World
    {
        std::unique_lock<std::mutex> lockIt(chunkLock);
        assert(chunk);
        assert(newLoadFn);
        assert(!loading);
        if(!chunk.unique())
            return false;
        chunk = nullptr;
        loadFn = newLoadFn;
        return true;
    }
    std::atomic_bool accessedFlag;
    std::atomic_bool *unloadAbortFlag = nullptr;
    static bool &getIgnoreReferencesFromThreadFlag(TLS &tls)
    {
        struct retval_tls_tag
        {
        };
        thread_local_variable<bool, retval_tls_tag> retval(tls, false);
        return retval.get();
    }

public:
    const PositionI basePosition;
    BlockChunkChunkVariables chunkVariables;
    IndirectBlockChunk(PositionI basePosition)
        : chunk(),
          loadFn(),
          chunkCond(),
          chunkLock(),
          accessedFlag(false),
          basePosition(basePosition),
          chunkVariables(
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
              std::make_shared<Semaphore>(BlockChunk::subchunkCountX * BlockChunk::subchunkCountY
                                          * BlockChunk::subchunkCountZ)
#endif
                  )
    {
        chunk = std::shared_ptr<BlockChunk>(new BlockChunk(basePosition, this));
    }
    ~IndirectBlockChunk()
    {
        std::unique_lock<std::mutex> lockIt(chunkLock);
        assert(!chunk || chunk.unique());
        chunk = nullptr; // destruct BlockChunk before chunkVariables
    }
    bool isLoaded()
    {
        std::unique_lock<std::mutex> lockIt(chunkLock);
        return chunk != nullptr;
    }
    std::shared_ptr<BlockChunk> getOrLoad(TLS &tls)
    {
        std::unique_lock<std::mutex> lockIt(chunkLock);
        if(unloadAbortFlag != nullptr)
        {
            unloadAbortFlag->store(true, std::memory_order_relaxed);
            while(unloadAbortFlag != nullptr)
            {
                chunkCond.wait(lockIt);
            }
        }
        if(chunk == nullptr)
        {
            if(loading)
            {
                while(loading || unloadAbortFlag != nullptr)
                {
                    if(unloadAbortFlag != nullptr)
                        unloadAbortFlag->store(true, std::memory_order_relaxed);
                    chunkCond.wait(lockIt);
                }
                return chunk;
            }
            loading = true;
            assert(loadFn);
            std::shared_ptr<BlockChunk> newChunk =
                std::shared_ptr<BlockChunk>(new BlockChunk(basePosition, this));
            lockIt.unlock();
            try
            {
                loadFn(newChunk);
            }
            catch(...)
            {
                lockIt.lock();
                loading = false;
                chunkCond.notify_all();
                throw;
            }
            lockIt.lock();
            chunk = newChunk;
            loadFn = nullptr;
            loading = false;
            chunkCond.notify_all();
        }
        if(!getIgnoreReferencesFromThreadFlag(tls))
            accessedFlag.store(true, std::memory_order_relaxed);
        return chunk;
    }
};

inline BlockChunkChunkVariables &BlockChunk::getChunkVariables()
{
    return indirectBlockChunk->chunkVariables;
}


typedef ChunkMap<IndirectBlockChunk> BlockChunkMap;
typedef BasicBlockChunkRelativePositionIterator<BlockChunk> BlockChunkRelativePositionIterator;

class BlockChunkFullLock final
{
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
    std::shared_ptr<Semaphore> semaphore;

public:
    explicit BlockChunkFullLock(BlockChunk &blockChunk)
        : semaphore(blockChunk.getChunkVariables().semaphore)
    {
        semaphore->lock(BlockChunk::subchunkCountX * BlockChunk::subchunkCountY
                        * BlockChunk::subchunkCountZ);
    }
    ~BlockChunkFullLock()
    {
        semaphore->unlock(BlockChunk::subchunkCountX * BlockChunk::subchunkCountY
                          * BlockChunk::subchunkCountZ);
    }
#else
    BlockChunk &chunk;
    struct SubchunkLockIterator
    {
        VectorI pos;
        BlockChunk *chunk;
        generic_lock_wrapper &operator*() const
        {
            return chunk->subchunks[pos.x][pos.y][pos.z].lock;
        }
        generic_lock_wrapper *operator->() const
        {
            return &operator*();
        }
        bool operator==(const SubchunkLockIterator &rt) const
        {
            return rt.pos == pos;
        }
        bool operator!=(const SubchunkLockIterator &rt) const
        {
            return rt.pos != pos;
        }
        const SubchunkLockIterator &operator++()
        {
            pos.x++;
            if(pos.x >= BlockChunk::subchunkCountX)
            {
                pos.x = 0;
                pos.z++;
                if(pos.z >= BlockChunk::subchunkCountZ)
                {
                    pos.z = 0;
                    pos.y++;
                }
            }
            return *this;
        }
        SubchunkLockIterator operator++(int)
        {
            SubchunkLockIterator retval = *this;
            operator++();
            return std::move(retval);
        }
        SubchunkLockIterator() : pos(), chunk()
        {
        }
        SubchunkLockIterator(VectorI pos, BlockChunk *chunk) : pos(pos), chunk(chunk)
        {
        }
    };

public:
    explicit BlockChunkFullLock(BlockChunk &chunk) : chunk(chunk)
    {
        SubchunkLockIterator begin(VectorI(0, 0, 0), &chunk),
            end(VectorI(0, BlockChunk::subchunkCountY, 0), &chunk);
        lock_all(begin, end);
    }
    ~BlockChunkFullLock()
    {
        SubchunkLockIterator begin(VectorI(0, 0, 0), &chunk),
            end(VectorI(0, BlockChunk::subchunkCountY, 0), &chunk);
        for(auto i = begin; i != end; i++)
            i->unlock();
    }
#endif
    BlockChunkFullLock(const BlockChunkFullLock &) = delete;
    const BlockChunkFullLock operator=(const BlockChunkFullLock &) = delete;
};

#if 0
namespace stream
{
template <typename T, size_t ChunkSizeXV, size_t ChunkSizeYV, size_t ChunkSizeZV, bool TransmitCompressedV>
struct is_value_changed<BlockChunk<T, ChunkSizeXV, ChunkSizeYV, ChunkSizeZV, TransmitCompressedV>>
{
    bool operator()(std::shared_ptr<const BlockChunk<T, ChunkSizeXV, ChunkSizeYV, ChunkSizeZV, TransmitCompressedV>> value, VariableSet &variableSet) const
    {
        if(value == nullptr)
        {
            return false;
        }

        return value->changeTracker.getChanged(variableSet);
    }
};
}
#endif
}
}

#endif // BLOCK_CHUNK_H_INCLUDED
