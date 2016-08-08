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
#include "util/block_update.h"
#include "entity/entity_struct.h"
#include <iterator>
#include "util/enum_traits.h"
#include "block/block_struct.h"
#include "lighting/lighting.h"
#include <memory>
#include "generate/biome/biome.h"
#include <cstdint>
#include <limits>
#include "util/lock.h"
#include "util/checked_array.h"
#include <chrono>
#include <list>
#include <functional>
#include <utility>
#include "util/tls.h"
#include <map>

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

class BlockUpdateIterator;
class BlockUpdateInChunkIterator;
struct BlockChunkBlockExtraData;
class BlockUpdate final
{
    friend struct BlockChunkBlockExtraData;
    friend class BlockUpdateIterator;
    friend class BlockUpdateInChunkIterator;

private:
    BlockUpdate *chunkPrev;
    BlockUpdate *chunkNext;
    PositionI position;
    float timeLeft;
    BlockUpdateKind kind;
    constexpr bool isEmpty(BlockUpdate *blockUpdatesInChunkListHead) const noexcept
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
        : chunkPrev(nullptr), chunkNext(nullptr), position(position), timeLeft(timeLeft), kind(kind)
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
    typedef enum_array<BlockUpdate, BlockUpdateKind> BlockUpdates;
    BlockUpdates blockUpdates;
    BlockDataPointer<BlockData> data;
    bool isEmpty(BlockUpdate *blockUpdatesInChunkListHead) const noexcept
    {
        if(data)
            return false;
        for(const auto &blockUpdate : blockUpdates)
        {
            if(!blockUpdate.isEmpty(blockUpdatesInChunkListHead))
                return false;
        }
        return true;
    }
};

class BlockIterator;

class BlockUpdateIterator final
{
    friend class BlockIterator;

public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef BlockUpdate value_type;
    typedef std::ptrdiff_t difference_type;
    typedef BlockUpdate *pointer;
    typedef BlockUpdate &reference;

private:
    BlockChunkBlockExtraData::BlockUpdates::iterator iter;
    BlockChunkBlockExtraData *blockExtraData;
    BlockUpdate *blockUpdatesInChunkListHead;

public:
    constexpr BlockUpdate &operator*() const noexcept
    {
        return *iter;
    }
    constexpr BlockUpdate *operator->() const noexcept
    {
        return &operator*();
    }
    constexpr BlockChunkBlockExtraData::BlockUpdates::iterator get() const noexcept
    {
        return iter;
    }

private:
    BlockUpdateIterator(BlockChunkBlockExtraData::BlockUpdates::iterator iter,
                        BlockChunkBlockExtraData *blockExtraData,
                        BlockUpdate *blockUpdatesInChunkListHead)
        : iter(iter),
          blockExtraData(blockExtraData),
          blockUpdatesInChunkListHead(blockUpdatesInChunkListHead)
    {
        while(this->iter != blockExtraData->blockUpdates.end()
              && this->iter->isEmpty(blockUpdatesInChunkListHead))
        {
            ++this->iter;
        }
    }

public:
    constexpr BlockUpdateIterator()
        : iter(), blockExtraData(nullptr), blockUpdatesInChunkListHead(nullptr)
    {
    }
    BlockUpdateIterator operator++(int)
    {
        auto retval = *this;
        operator++();
        return retval;
    }
    BlockUpdateIterator &operator++()
    {
        assert(iter < blockExtraData->blockUpdates.end());
        ++iter;
        while(iter != blockExtraData->blockUpdates.end()
              && iter->isEmpty(blockUpdatesInChunkListHead))
        {
            ++iter;
        }
        return *this;
    }
    BlockUpdateIterator operator--(int)
    {
        auto retval = *this;
        operator--();
        return retval;
    }
    BlockUpdateIterator &operator--()
    {
        assert(iter > blockExtraData->blockUpdates.begin());
        --iter;
        while(iter->isEmpty(blockUpdatesInChunkListHead))
        {
            assert(iter > blockExtraData->blockUpdates.begin());
            --iter;
        }
        return *this;
    }
    constexpr bool operator==(const BlockUpdateIterator &r) const noexcept
    {
        return iter == r.iter;
    }
    constexpr bool operator!=(const BlockUpdateIterator &r) const noexcept
    {
        return iter != r.iter;
    }
};

struct BlockChunkBlock final
{
    typedef Lighting::LightValueType LightValueType;
    static constexpr int lightBitWidth = Lighting::lightBitWidth;
    LightValueType directSkylight : lightBitWidth;
    LightValueType indirectSkylight : lightBitWidth;
    LightValueType indirectArtificalLight : lightBitWidth;
    BlockDescriptorIndex blockKind;
    std::unique_ptr<BlockChunkBlockExtraData> extraData;
    static_assert(sizeof(std::unique_ptr<BlockChunkBlockExtraData>)
                      == sizeof(BlockChunkBlockExtraData *),
                  "");
    BlockChunkBlock() noexcept : directSkylight(0),
                                 indirectSkylight(0),
                                 indirectArtificalLight(0),
                                 blockKind(),
                                 extraData()
    {
    }
    Lighting getLighting() const noexcept
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
    Block getBlock() const noexcept
    {
        return Block(blockKind.get(), getLighting(), extraData ? extraData->data : nullptr);
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
            IndexHelper2(BlockChunkBlock *blocks, std::size_t indexX, std::size_t indexY) noexcept
                : blocks(blocks),
                  indexX(indexX),
                  indexY(indexY)
            {
            }

        public:
            BlockChunkBlock &operator[](std::size_t indexZ) noexcept
            {
                return blocks[getIndex(indexX, indexY, indexZ)];
            }
            BlockChunkBlock &at(std::size_t index) noexcept
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size() noexcept
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
            IndexHelper1(BlockChunkBlock *blocks, std::size_t indexX) noexcept : blocks(blocks),
                                                                                 indexX(indexX)
            {
            }

        public:
            IndexHelper2 operator[](std::size_t indexY) noexcept
            {
                return IndexHelper2(blocks, indexX, indexY);
            }
            IndexHelper2 at(std::size_t index) noexcept
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size() noexcept
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
            ConstIndexHelper2(const BlockChunkBlock *blocks,
                              std::size_t indexX,
                              std::size_t indexY) noexcept : blocks(blocks),
                                                             indexX(indexX),
                                                             indexY(indexY)
            {
            }

        public:
            const BlockChunkBlock &operator[](std::size_t indexZ) noexcept
            {
                return blocks[getIndex(indexX, indexY, indexZ)];
            }
            const BlockChunkBlock &at(std::size_t index) noexcept
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size() noexcept
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
            ConstIndexHelper1(const BlockChunkBlock *blocks, std::size_t indexX) noexcept
                : blocks(blocks),
                  indexX(indexX)
            {
            }

        public:
            ConstIndexHelper2 operator[](std::size_t indexY) noexcept
            {
                return ConstIndexHelper2(blocks, indexX, indexY);
            }
            ConstIndexHelper2 at(std::size_t index) noexcept
            {
                assert(index < size());
                return operator[](index);
            }
            static std::size_t size() noexcept
            {
                return chunkSizeY;
            }
        };
        ConstIndexHelper1 operator[](std::size_t indexX) const noexcept
        {
            return ConstIndexHelper1(blocks, indexX);
        }
        IndexHelper1 operator[](std::size_t indexX) noexcept
        {
            return IndexHelper1(blocks, indexX);
        }
        ConstIndexHelper1 at(std::size_t index) const noexcept
        {
            assert(index < size());
            return operator[](index);
        }
        IndexHelper1 at(std::size_t index) noexcept
        {
            assert(index < size());
            return operator[](index);
        }
        static std::size_t size() noexcept
        {
            return chunkSizeX;
        }
    };
    Mutex &getLock() noexcept;
    ConditionVariable &getCond() noexcept;
    PositionI getBasePosition() const noexcept;
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
    explicit BlockChunk(IndirectBlockChunk *indirectBlockChunk)
        : nextLockedChunk(nullptr),
          columns(),
          blocks(),
          blockUpdateListHead(nullptr),
          blockUpdateListTail(nullptr),
          blockUpdatesPerPhase(),
          lastBlockUpdateTime(invalidLastBlockUpdateTime),
          entities(),
          indirectBlockChunk(indirectBlockChunk)
    {
    }
};

class World;
class BlockChunks;

class IndirectBlockChunk final
{
    friend class World;
    friend class BlockChunk;
    friend class BlockChunks;
    friend class BlockIterator;
    IndirectBlockChunk(const IndirectBlockChunk &) = delete;
    IndirectBlockChunk &operator=(const IndirectBlockChunk &) = delete;

private:
    std::shared_ptr<BlockChunk> chunk;
    std::function<void(BlockChunk &chunk)> loadFn;
    bool loading = false;
    bool setUnloaded(std::function<void(BlockChunk &chunk)> newLoadFn,
                     std::unique_lock<Mutex> &lockIt) // called by World
    {
        assert(chunk);
        assert(newLoadFn);
        assert(!loading);
        if(chunk.unique())
        {
            chunk.reset();
            loadFn = newLoadFn;
            return true;
        }
        return false;
    }
    static bool &getIgnoreReferencesFromThreadFlag(TLS &tls)
    {
        struct retval_tls_tag
        {
        };
        thread_local_variable<bool, retval_tls_tag> retval(tls, false);
        return retval.get();
    }

public:
    ConditionVariable chunkCond;
    Mutex chunkLock;
    const PositionI basePosition;
    explicit IndirectBlockChunk(PositionI basePosition)
        : chunk(), loadFn(), chunkCond(), chunkLock(), basePosition(basePosition)
    {
        chunk = std::unique_ptr<BlockChunk>(new BlockChunk(this));
    }
    ~IndirectBlockChunk()
    {
        chunk.reset(); // destroy chunk before anything else
    }
    bool isLoaded() noexcept
    {
        std::unique_lock<Mutex> lockIt(chunkLock);
        return chunk != nullptr;
    }
    bool isLoaded(std::unique_lock<Mutex> &lockIt) noexcept
    {
        return chunk != nullptr;
    }
    std::shared_ptr<BlockChunk> ensureLoaded(TLS &tls, std::unique_lock<Mutex> &lockIt)
    {
        assert(lockIt.owns_lock() && lockIt.mutex() == &chunkLock);
        if(chunk != nullptr)
            return chunk;
        if(!loading)
        {
            loading = true;
            assert(loadFn);
            lockIt.unlock();
            std::shared_ptr<BlockChunk> newChunk;
            try
            {
                newChunk = std::make_shared<BlockChunk>(this);
                loadFn(*newChunk);
            }
            catch(...)
            {
                lockIt.lock();
                loading = false;
                chunkCond.notify_all();
                throw;
            }
            lockIt.lock();
            chunk = std::move(newChunk);
            loadFn = nullptr;
            loading = false;
            chunkCond.notify_all();
        }
        else
        {
            while(loading)
                chunkCond.wait(lockIt);
            if(chunk == nullptr)
                throw std::runtime_error("ensureLoaded failed while waiting");
        }
        return chunk;
    }
};

inline Mutex &BlockChunk::getLock() noexcept
{
    return indirectBlockChunk->chunkLock;
}

inline ConditionVariable &BlockChunk::getCond() noexcept
{
    return indirectBlockChunk->chunkCond;
}

inline PositionI BlockChunk::getBasePosition() const noexcept
{
    return indirectBlockChunk->basePosition;
}

struct BlockChunkLockOrderLess final
{
    constexpr bool operator()(const PositionI &a, const PositionI &b) const noexcept
    {
        return a.x < b.x ? true : a.x > b.x ? false : a.y < b.y ? true : a.y > b.y ?
                                                                  false :
                                                                  a.z < b.z ? true : a.z > b.z ?
                                                                              false :
                                                                              a.d < b.d;
    }
    bool operator()(const BlockChunk &a, const BlockChunk &b) const noexcept
    {
        return operator()(a.getBasePosition(), b.getBasePosition());
    }
    bool operator()(const IndirectBlockChunk &a, const IndirectBlockChunk &b) const noexcept
    {
        return operator()(a.basePosition, b.basePosition);
    }
    bool operator()(const BlockChunk *a, const BlockChunk *b) const noexcept
    {
        if(!a)
            return b != nullptr;
        if(!b)
            return false;
        return operator()(a->getBasePosition(), b->getBasePosition());
    }
    bool operator()(const IndirectBlockChunk *a, const IndirectBlockChunk *b) const noexcept
    {
        if(!a)
            return b != nullptr;
        if(!b)
            return false;
        return operator()(a->basePosition, b->basePosition);
    }
};

struct BlockChunkLockOrderGreater final
{
    constexpr bool operator()(const PositionI &a, const PositionI &b) const noexcept
    {
        return BlockChunkLockOrderLess()(b, a);
    }
    bool operator()(const BlockChunk &a, const BlockChunk &b) const noexcept
    {
        return BlockChunkLockOrderLess()(b, a);
    }
    bool operator()(const BlockChunk *a, const BlockChunk *b) const noexcept
    {
        return BlockChunkLockOrderLess()(b, a);
    }
    bool operator()(const IndirectBlockChunk &a, const IndirectBlockChunk &b) const noexcept
    {
        return BlockChunkLockOrderLess()(b, a);
    }
    bool operator()(const IndirectBlockChunk *a, const IndirectBlockChunk *b) const noexcept
    {
        return BlockChunkLockOrderLess()(b, a);
    }
};

class LockedIndirectBlockChunkList final
{
    LockedIndirectBlockChunkList(const LockedIndirectBlockChunkList &) = delete;
    LockedIndirectBlockChunkList &operator=(const LockedIndirectBlockChunkList &) = delete;

public:
    typedef std::map<PositionI, IndirectBlockChunk *, BlockChunkLockOrderLess> ListType;

public:
    static ListType makeList(std::initializer_list<IndirectBlockChunk *> listIn,
                             ListType retval = ListType())
    {
        for(auto blockChunk : listIn)
        {
            assert(blockChunk);
            bool insertSucceeded =
                std::get<1>(retval.emplace(blockChunk->basePosition, blockChunk));
            static_cast<void>(insertSucceeded);
            assert(insertSucceeded);
        }
        return retval;
    }
    static ListType makeList(const std::vector<IndirectBlockChunk *> &listIn,
                             ListType retval = ListType())
    {
        for(auto blockChunk : listIn)
        {
            assert(blockChunk);
            bool insertSucceeded =
                std::get<1>(retval.emplace(blockChunk->basePosition, blockChunk));
            static_cast<void>(insertSucceeded);
            assert(insertSucceeded);
        }
        return retval;
    }

private:
    ListType chunkList;
    bool locked;

private:
    static void unlockList(const ListType &chunkList) noexcept
    {
        for(auto &chunk : chunkList)
        {
            std::get<1>(chunk)->chunkLock.unlock();
        }
    }
    static void lockList(const ListType &chunkList) noexcept
    {
        for(auto &chunk : chunkList)
        {
            std::get<1>(chunk)->chunkLock.lock();
        }
    }
    template <std::size_t retryCount = 5>
    static bool tryLockList(const ListType &chunkList) noexcept
    {
        for(ListType::const_iterator i = chunkList.begin(); i != chunkList.end(); ++i)
        {
            bool locked = false;
            for(std::size_t j = 0; j < retryCount; j++)
            {
                if(!std::get<1>(*i)->chunkLock.try_lock())
                    continue;
                locked = true;
                break;
            }
            if(!locked)
            {
                for(ListType::const_iterator j = chunkList.begin(); j != i; ++j)
                {
                    std::get<1>(*j)->chunkLock.unlock();
                }
                return false;
            }
        }
        return true;
    }

public:
    LockedIndirectBlockChunkList() : chunkList(), locked(false)
    {
    }
    LockedIndirectBlockChunkList(LockedIndirectBlockChunkList &&rt) : chunkList(), locked(rt.locked)
    {
        chunkList.swap(rt.chunkList);
        rt.locked = false;
    }
    LockedIndirectBlockChunkList(ListType chunkList, std::defer_lock_t)
        : chunkList(std::move(chunkList)), locked(false)
    {
        assert(!this->chunkList.empty());
    }
    explicit LockedIndirectBlockChunkList(ListType chunkList)
        : LockedIndirectBlockChunkList(std::move(chunkList), std::defer_lock)
    {
        lock();
    }
    LockedIndirectBlockChunkList(ListType chunkList, std::try_to_lock_t)
        : LockedIndirectBlockChunkList(std::move(chunkList), std::defer_lock)
    {
        try_lock();
    }
    LockedIndirectBlockChunkList(ListType chunkList, std::adopt_lock_t)
        : LockedIndirectBlockChunkList(std::move(chunkList), std::defer_lock)
    {
        locked = true;
    }
    ~LockedIndirectBlockChunkList()
    {
        if(locked)
        {
            unlockList(chunkList);
        }
    }
    LockedIndirectBlockChunkList &operator=(LockedIndirectBlockChunkList &&rt) noexcept
    {
        assert(this != &rt);
        if(locked)
        {
            unlockList(chunkList);
            locked = false;
        }
        swap(rt);
        return *this;
    }
    void swap(LockedIndirectBlockChunkList &other) noexcept
    {
        using std::swap;
        swap(locked, other.locked);
        chunkList.swap(other.chunkList);
    }
    void lock() noexcept
    {
        assert(!locked);
        assert(!chunkList.empty());
        lockList(chunkList);
        locked = true;
    }
    bool try_lock() noexcept
    {
        assert(!locked);
        assert(!chunkList.empty());
        locked = tryLockList(chunkList);
        return locked;
    }
    void unlock() noexcept
    {
        assert(locked);
        assert(!chunkList.empty());
        unlockList(chunkList);
        locked = false;
    }
    ListType release() noexcept
    {
        ListType retval;
        locked = false;
        retval.swap(chunkList);
        return retval;
    }
    const ListType &getChunkList() const noexcept
    {
        return chunkList;
    }
    ListType &getChunkList() noexcept
    {
        return chunkList;
    }
    bool owns_lock() const noexcept
    {
        return locked;
    }
    explicit operator bool() const noexcept
    {
        return owns_lock();
    }
};

class BlockChunks final
{
private:
    struct Slice final
    {
        Mutex lock;
        std::unordered_map<PositionI, IndirectBlockChunk> map;
    };
    checked_array<Slice, 17> slices;
    std::size_t getSliceIndex(const PositionI &position) noexcept
    {
        return std::hash<PositionI>()(position) % slices.size();
    }

public:
    IndirectBlockChunk &getOrMake(const PositionI &position)
    {
        Slice &slice = slices[getSliceIndex(position)];
        std::unique_lock<Mutex> lockIt(slice.lock);
        return slice.map[position];
    }
    template <bool loadChunks>
    LockedIndirectBlockChunkList lockChunks(const PositionI *positions,
                                            std::size_t positionCount,
                                            TLS &tls)
    {
        LockedIndirectBlockChunkList::ListType chunkList;
        for(std::size_t i = 0; i < positionCount; i++)
        {
            chunkList = LockedIndirectBlockChunkList::makeList({&getOrMake(positions[i])},
                                                               std::move(chunkList));
        }
        if(loadChunks)
        {
            std::vector<std::shared_ptr<BlockChunk>> blockChunks;
            blockChunks.reserve(positionCount);
            for(auto iter = chunkList.begin(); iter != chunkList.end(); ++iter)
            {
                std::unique_lock<Mutex> lockIt(std::get<1>(*iter)->chunkLock);
                blockChunks.push_back(std::get<1>(*iter)->ensureLoaded(tls, lockIt));
            }
            return LockedIndirectBlockChunkList(std::move(chunkList));
        }
        else
            return LockedIndirectBlockChunkList(std::move(chunkList));
    }
    std::vector<IndirectBlockChunk *> getAll(
        std::vector<IndirectBlockChunk *> retval = std::vector<IndirectBlockChunk *>())
    {
        retval.clear();
        for(Slice &slice : slices)
        {
            std::unique_lock<Mutex> lockIt(slice.lock);
            retval.reserve(retval.size() + slice.map.size());
            retval.insert(retval.end(), slice.map.begin(), slice.map.end());
        }
        return retval;
    }
};
}
}

namespace std
{
inline void swap(programmerjake::voxels::LockedIndirectBlockChunkList &a,
                 programmerjake::voxels::LockedIndirectBlockChunkList &b) noexcept
{
    a.swap(b);
}
}

#endif // BLOCK_CHUNK_H_INCLUDED
