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
#include "entity/entity_struct.h"

namespace programmerjake
{
namespace voxels
{
struct BlockChunk;

struct BlockChunkEntity final
{
    Entity entity;
    BlockChunk *containingChunk;
};

class BlockUpdateInChunkIterator;
struct BlockChunkBlockExtraData;
class BlockUpdate final
{
    friend struct BlockChunkBlockExtraData;
    friend class BlockUpdateInChunkIterator;

private:
    BlockUpdate *chunkPrev;
    BlockUpdate *chunkNext;
    PositionI position;
    float timeLeft;
    BlockUpdateKind kind;
    constexpr bool isEmpty(BlockUpdate *blockUpdatesInChunkListHead,
                           BlockUpdate *blockUpdatesInChunkListTail) const noexcept
    {
        return chunkPrev == nullptr && chunkNext == nullptr && this != blockUpdatesInChunkListHead;
    }
    void insert(BlockUpdate *&blockUpdatesInChunkListHead,
                BlockUpdate *&blockUpdatesInChunkListTail) noexcept
    {
        assert(chunkPrev == nullptr);
        assert(chunkNext == nullptr);
        assert(blockUpdatesInChunkListHead != this);
        chunkNext = blockUpdatesInChunkListHead;
        if(chunkNext)
        {
            assert(chunkNext->chunkPrev == nullptr);
            chunkNext->chunkPrev = this;
        }
        else
        {
            assert(blockUpdatesInChunkListTail == nullptr);
            blockUpdatesInChunkListTail = this;
        }
        blockUpdatesInChunkListHead = this;
    }
    void remove(BlockUpdate *&blockUpdatesInChunkListHead,
                BlockUpdate *&blockUpdatesInChunkListTail) noexcept
    {
        assert(chunkPrev != nullptr || blockUpdatesInChunkListHead == this);
        assert(chunkNext != nullptr || blockUpdatesInChunkListTail == this);
        assert(chunkPrev != chunkNext);
        if(chunkPrev)
        {
            assert(chunkPrev->chunkNext == this);
            chunkPrev->chunkNext = chunkNext;
        }
        else
        {
            blockUpdatesInChunkListHead = chunkNext;
        }
        if(chunkNext)
        {
            assert(chunkNext->chunkPrev == this);
            chunkNext->chunkPrev = chunkPrev;
        }
        else
        {
            blockUpdatesInChunkListTail = chunkPrev;
        }
    }
    constexpr BlockUpdate() : chunkPrev(nullptr), chunkNext(nullptr), position(), timeLeft(), kind()
    {
    }
    constexpr BlockUpdate(BlockUpdateKind kind, PositionI position, float timeLeft)
        : chunkPrev(chunkPrev),
          chunkNext(chunkNext),
          position(position),
          timeLeft(timeLeft),
          kind(kind)
    {
    }

public:
    constexpr PositionI getPosition() const
    {
        return position;
    }
    constexpr float getTimeLeft() const
    {
        return timeLeft;
    }
    constexpr BlockUpdateKind getKind() const
    {
        return kind;
    }
};

class BlockUpdateInChunkIterator final
{
    friend class BlockChunk;

public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef BlockUpdate value_type;
    typedef std::ptrdiff_t difference_type;
    typedef BlockUpdate *pointer;
    typedef BlockUpdate &reference;

private:
    BlockUpdate *node;

public:
    constexpr BlockUpdate &operator*() const noexcept
    {
        return *node;
    }
    constexpr BlockUpdate *operator->() const noexcept
    {
        return &operator*();
    }
    constexpr BlockUpdate *get() const noexcept
    {
        return node;
    }

private:
    explicit constexpr BlockUpdateInChunkIterator(BlockUpdate *node) : node(node)
    {
    }

public:
    constexpr BlockUpdateInChunkIterator() : node(nullptr)
    {
    }
    BlockUpdateInChunkIterator operator++(int)
    {
        BlockUpdateInChunkIterator retval = *this;
        node = node->chunkNext;
        return retval;
    }
    BlockUpdateInChunkIterator &operator++()
    {
        node = node->chunkNext;
        return *this;
    }
    BlockUpdateInChunkIterator operator--(int)
    {
        BlockUpdateInChunkIterator retval = *this;
        node = node->chunkPrev;
        return retval;
    }
    BlockUpdateInChunkIterator &operator--()
    {
        node = node->chunkPrev;
        return *this;
    }
    constexpr bool operator==(const BlockUpdateInChunkIterator &r) const noexcept
    {
        return node == r.node;
    }
    constexpr bool operator!=(const BlockUpdateInChunkIterator &r) const noexcept
    {
        return node != r.node;
    }
};

struct BlockChunkBlockExtraData final
{
    enum_array<BlockUpdate, BlockUpdateKind> blockUpdates;
    BlockDataPointer<BlockData> data;
    bool isEmpty(BlockUpdate *blockUpdatesInChunkListHead,
                 BlockUpdate *blockUpdatesInChunkListTail) const noexcept
    {
        if(data)
            return false;
        for(const auto &blockUpdate : blockUpdates)
        {
            if(!blockUpdate.isEmpty(blockUpdatesInChunkListHead, blockUpdatesInChunkListTail))
                return false;
        }
        return true;
    }
};

struct BlockChunkBlock final
{
    typedef Lighting::LightValueType LightValueType;
    static constexpr int lightBitWidth = Lighting::lightBitWidth;
    LightValueType directSkylight : lightBitWidth;
    LightValueType indirectSkylight : lightBitWidth;
    LightValueType indirectArtificalLight : lightBitWidth;
    static constexpr int blockKindBitWidth = 32 - 1 - 3 * lightBitWidth;
    static_assert(std::numeric_limits<BlockDescriptorIndex::IndexIntType>::radix == 2, "");
    static_assert(std::numeric_limits<BlockDescriptorIndex::IndexIntType>::digits
                      <= blockKindBitWidth,
                  "");
    static_assert(!std::numeric_limits<BlockDescriptorIndex::IndexIntType>::is_signed, "");
    BlockDescriptorIndex::IndexIntType blockKind : blockKindBitWidth;
    std::unique_ptr<BlockChunkBlockExtraData> extraData;
    static_assert(sizeof(std::unique_ptr<BlockChunkBlockExtraData>)
                      == sizeof(BlockChunkBlockExtraData *),
                  "");
    constexpr BlockChunkBlock()
        : directSkylight(0),
          indirectSkylight(0),
          indirectArtificalLight(0),
          blockKind(BlockDescriptorIndex::NullIndex),
          extraData()
    {
    }
    constexpr Lighting getLighting() const noexcept
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

struct BlockChunkColumn final
{
    BiomeProperties biomeProperties;
    std::int32_t highestNontransparentBlock;
    static constexpr std::int32_t allBlocksAreTransparentValue =
        std::numeric_limits<std::int32_t>::min();
    BlockChunkColumn() : biomeProperties(), highestNontransparentBlock(allBlocksAreTransparentValue)
    {
    }
};

typedef std::uint64_t BlockChunkInvalidateCountType;
class IndirectBlockChunk;

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
        static std::size_t getIndex(std::size_t indexX,
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
    BlockChunk *nextLockedChunk;
    checked_array<checked_array<BlockChunkColumn, chunkSizeZ>, chunkSizeX> columns;
    BlocksArray blocks;
    BlockUpdate *blockUpdateListHead;
    BlockUpdate *blockUpdateListTail;
    enum_array<std::size_t, BlockUpdatePhase> blockUpdatesPerPhase;
    std::chrono::steady_clock::time_point lastBlockUpdateTime;
    static constexpr std::chrono::steady_clock::time_point invalidLastBlockUpdateTime =
        std::chrono::steady_clock::time_point::min();
    std::list<BlockChunkEntity> entities;
    IndirectBlockChunk *const indirectBlockChunk;
    const PositionI chunkBasePosition;
};

class World;

class IndirectBlockChunk final
{
    friend class World;
    IndirectBlockChunk(const IndirectBlockChunk &) = delete;
    IndirectBlockChunk &operator=(const IndirectBlockChunk &) = delete;

private:
    std::unique_ptr<BlockChunk> chunk;
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
