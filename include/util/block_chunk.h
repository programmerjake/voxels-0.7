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
#ifndef BLOCK_CHUNK_H_INCLUDED
#define BLOCK_CHUNK_H_INCLUDED

#include "util/position.h"
#include "stream/stream.h"
#include "util/variable_set.h"
#include "stream/compressed_stream.h"
#include "util/basic_block_chunk.h"
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
    BlockUpdate *chunk_prev;
    BlockUpdate *chunk_next;
    BlockUpdate *block_next;
    PositionI position;
    float time_left;
    BlockUpdateKind kind;
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
    const BlockUpdate &operator =(const BlockUpdate &) = delete;
    ~BlockUpdate()
    {
    }
    static void allocateAndFree(BlockUpdate *blockUpdateToFree, BlockUpdate **pBlockUpdateToAllocate)
    {
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
        static thread_local FreeList freeList;
        if(pBlockUpdateToAllocate)
        {
            if(blockUpdateToFree)
            {
                *pBlockUpdateToAllocate = blockUpdateToFree;
            }
            else if(freeList.head == nullptr)
            {
                *pBlockUpdateToAllocate = new BlockUpdate;
            }
            else
            {
                *pBlockUpdateToAllocate = freeList.head;
                freeList.head = freeList.head->block_next;
                freeList.size--;
            }
        }
        else if(blockUpdateToFree)
        {
            if(freeList.size > static_cast<std::size_t>(1 << 20) / sizeof(BlockUpdate))
            {
                delete blockUpdateToFree;
            }
            else
            {
                blockUpdateToFree->block_next = freeList.head;
                freeList.head = blockUpdateToFree;
                freeList.size++;
            }
        }
    }
    static BlockUpdate *allocate(BlockUpdateKind kind, PositionI position, float time_left, BlockUpdate *block_next = nullptr)
    {
        BlockUpdate *retval;
        allocateAndFree(nullptr, &retval);
        retval->init(kind, position, time_left, block_next);
        return retval;
    }
    static void free(BlockUpdate *v)
    {
        allocateAndFree(v, nullptr);
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

class BlockUpdateIterator final : public std::iterator<std::forward_iterator_tag, const BlockUpdate>
{
private:
    friend class BlockIterator;
    friend class World;
    BlockUpdate *node;
public:
    BlockUpdate &operator *()
    {
        return *node;
    }
    BlockUpdate *operator ->()
    {
        return node;
    }
private:
    explicit BlockUpdateIterator(BlockUpdate *node)
        : node(node)
    {
    }
public:
    BlockUpdateIterator()
        : node(nullptr)
    {
    }
    BlockUpdateIterator operator ++(int)
    {
        BlockUpdateIterator retval = *this;
        node = node->block_next;
        return retval;
    }
    const BlockUpdateIterator &operator ++()
    {
        node = node->block_next;
        return *this;
    }
    const BlockUpdate &operator *() const
    {
        return *node;
    }
    const BlockUpdate *operator ->() const
    {
        return node;
    }
    bool operator ==(const BlockUpdateIterator &r) const
    {
        return node == r.node;
    }
    bool operator !=(const BlockUpdateIterator &r) const
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
    static constexpr int blockKindBitWidth = 32 - 3 * lightBitWidth;
    unsigned blockKind : blockKindBitWidth;
    BlockChunkBlock()
        : directSkylight(0), indirectSkylight(0), indirectArtificalLight(0), blockKind((static_cast<unsigned>(1) << blockKindBitWidth) - 1)
    {
    }
    void invalidate()
    {
    }
    const BlockChunkBlock &operator =(const BlockChunkBlock &) = delete;
    Lighting getLighting() const
    {
        return Lighting(directSkylight, indirectSkylight, indirectArtificalLight, Lighting::MakeDirectOnly);
    }
    void setLighting(Lighting l)
    {
        directSkylight = l.directSkylight;
        indirectSkylight = l.indirectSkylight;
        indirectArtificalLight = l.indirectArtificalLight;
    }
};

class BlockOptionalDataHashTable;

class BlockOptionalData final
{
    friend class BlockOptionalDataHashTable;
public:
    static constexpr int BlockChunkSubchunkShiftXYZ = 3;
    static constexpr std::int32_t BlockChunkSubchunkSizeXYZ = 1 << BlockChunkSubchunkShiftXYZ;
private:
    BlockOptionalData *hashNext;
    BlockOptionalData()
    {
    }
    ~BlockOptionalData()
    {
    }
    void init()
    {
        hashNext = nullptr;
        updateListHead = nullptr;
        data = nullptr;
    }
    struct Allocator final
    {
    private:
        BlockOptionalData *freeListHead = nullptr;
        std::size_t freeListSize = 0;
    public:
        Allocator() = default;
        ~Allocator()
        {
            while(freeListHead != nullptr)
            {
                BlockOptionalData *deleteMe = freeListHead;
                freeListHead = freeListHead->hashNext;
                delete deleteMe;
            }
        }
        void free(BlockOptionalData *p)
        {
            if(!p)
                return;
            if(freeListSize > 1 << 16)
            {
                delete p;
                return;
            }
            p->init();
            p->hashNext = freeListHead;
            freeListHead = p;
            freeListSize++;
        }
        BlockOptionalData *allocate()
        {
            if(freeListHead)
            {
                BlockOptionalData *retval = freeListHead;
                freeListHead = retval->hashNext;
                freeListSize--;
                return retval;
            }
            return new BlockOptionalData();
        }
    };
    static Allocator &getAllocator()
    {
        static thread_local Allocator a;
        return a;
    }
    static void free(BlockOptionalData *p)
    {
        getAllocator().free(p);
    }
    static BlockOptionalData *allocate()
    {
        return getAllocator().allocate();
    }
    std::uint8_t posX : BlockChunkSubchunkShiftXYZ, posY : BlockChunkSubchunkShiftXYZ, posZ : BlockChunkSubchunkShiftXYZ;
public:
    BlockUpdate *updateListHead = nullptr; // BlockChunkChunkVariables is responsible for deleting
    BlockDataPointer<BlockData> data;
    bool empty() const
    {
        return updateListHead == nullptr && data == nullptr;
    }
};

struct BlockOptionalDataHashTable final
{
public:
    static constexpr std::int32_t BlockChunkSubchunkSizeXYZ = BlockOptionalData::BlockChunkSubchunkSizeXYZ;
private:
    checked_array<BlockOptionalData *, (1 << 6)> table;
    std::size_t hashPos(VectorI pos) const
    {
        assert(pos.x >= 0 && pos.x < BlockChunkSubchunkSizeXYZ);
        assert(pos.y >= 0 && pos.y < BlockChunkSubchunkSizeXYZ);
        assert(pos.z >= 0 && pos.z < BlockChunkSubchunkSizeXYZ);
        return (pos.x + pos.y * 9 + pos.z * 49) % table.size();
    }
public:
    BlockOptionalDataHashTable()
        : table{}
    {
    }
    void clear()
    {
        for(BlockOptionalData *&i : table)
        {
            BlockOptionalData *node = i;
            i = nullptr;
            while(node != nullptr)
            {
                BlockOptionalData *freeMe = node;
                node = node->hashNext;
                BlockOptionalData::free(freeMe);
            }
        }
    }
    ~BlockOptionalDataHashTable()
    {
        clear();
    }
    BlockOptionalDataHashTable(const BlockOptionalDataHashTable &rt)
        : table{}
    {
        for(std::size_t i = 0; i < table.size(); i++)
        {
            BlockOptionalData **pNewNode = &table[i];
            for(BlockOptionalData *node = rt.table[i]; node != nullptr; node = node->hashNext)
            {
                BlockOptionalData *newNode = BlockOptionalData::allocate();
                *pNewNode = newNode;
                pNewNode = &newNode->hashNext;
                newNode->data = node->data;
            }
            *pNewNode = nullptr;
        }
    }
    BlockOptionalDataHashTable &operator =(const BlockOptionalDataHashTable &) = delete;
    void erase(VectorI pos)
    {
        BlockOptionalData **pNode = &table[hashPos(pos)];
        while(*pNode != nullptr)
        {
            BlockOptionalData *node = *pNode;
            if(node->posX == pos.x && node->posY == pos.y && node->posZ == pos.z)
            {
                *pNode = node->hashNext;
                BlockOptionalData::free(node);
                return;
            }
            pNode = &node->hashNext;
        }
    }
    BlockOptionalData *get_or_make(VectorI pos)
    {
        BlockOptionalData **pNode = &table[hashPos(pos)];
        BlockOptionalData **pTableEntry = pNode;
        while(*pNode != nullptr)
        {
            BlockOptionalData *node = *pNode;
            if(node->posX == pos.x && node->posY == pos.y && node->posZ == pos.z)
            {
                *pNode = node->hashNext;
                node->hashNext = *pTableEntry;
                *pTableEntry = node;
                return node;
            }
            pNode = &node->hashNext;
        }
        BlockOptionalData *node = BlockOptionalData::allocate();
        node->posX = pos.x;
        node->posY = pos.y;
        node->posZ = pos.z;
        node->hashNext = *pTableEntry;
        *pTableEntry = node;
        return node;
    }
    BlockOptionalData *get(VectorI pos)
    {
        BlockOptionalData **pNode = &table[hashPos(pos)];
        BlockOptionalData **pTableEntry = pNode;
        while(*pNode != nullptr)
        {
            BlockOptionalData *node = *pNode;
            if(node->posX == pos.x && node->posY == pos.y && node->posZ == pos.z)
            {
                *pNode = node->hashNext;
                node->hashNext = *pTableEntry;
                *pTableEntry = node;
                return node;
            }
            pNode = &node->hashNext;
        }
        return nullptr;
    }
    BlockDataPointer<BlockData> getData(VectorI pos)
    {
        BlockOptionalData *node = get(pos);
        if(node == nullptr)
            return nullptr;
        return node->data;
    }
    void setData(VectorI pos, BlockDataPointer<BlockData> data)
    {
        BlockOptionalData *node;
        if(data != nullptr)
            node = get_or_make(pos);
        else
            node = get(pos);
        if(node == nullptr)
            return;
        node->data = std::move(data);
        if(node->empty())
            erase(pos);
    }
    void setUpdateListHead(VectorI pos, BlockUpdate *updateListHead)
    {
        BlockOptionalData *node;
        if(updateListHead != nullptr)
            node = get_or_make(pos);
        else
            node = get(pos);
        if(node == nullptr)
            return;
        node->updateListHead = updateListHead;
        if(node->empty())
            erase(pos);
    }
    BlockUpdate *getUpdateListHead(VectorI pos)
    {
        BlockOptionalData *node = get(pos);
        if(node == nullptr)
            return nullptr;
        return node->updateListHead;
    }
};

struct BlockChunkBiome final
{
    BiomeProperties biomeProperties;
};

typedef std::uint64_t BlockChunkInvalidateCountType;

struct BlockChunkSubchunk final
{
    checked_lock<150> lockImp;
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
        if(blockKinds.size() >= static_cast<std::size_t>(1) << BlockChunkBlock::blockKindBitWidth) // every block kind is different : so we can just replace the old block kind because this is the only block using it
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
            blockKinds.reserve(blockKinds.size() + 8); // don't use vector's default step to save space

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
    std::shared_ptr<enum_array<Mesh, RenderLayer>> cachedMeshes;
    std::atomic_bool cachedMeshesUpToDate;
    WrappedEntity::SubchunkListType entityList;
    BlockChunkInvalidateCountType invalidateCount = 0;
    linked_map<PositionI, char> particleGeneratingSet; /// holds positions of blocks in this subchunk that generate particles
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
    void addToParticleGeneratingBlockList(std::vector<PositionI> &dest) const /// must be locked first
    {
        dest.reserve(dest.size() + particleGeneratingSet.size());
        for(auto v : particleGeneratingSet)
        {
            dest.push_back(std::get<0>(v));
        }
    }
    BlockChunkSubchunk(const BlockChunkSubchunk &rt)
        : lock(lockImp),
          blockOptionalData(rt.blockOptionalData),
          blockKinds(rt.blockKinds),
          blockKindsMap(rt.blockKindsMap),
          cachedMeshes(nullptr),
          cachedMeshesUpToDate(false),
          entityList([](WrappedEntity *)
          {
          })
    {
    }
    BlockChunkSubchunk()
        : lock(lockImp),
          cachedMeshes(nullptr),
          cachedMeshesUpToDate(false),
          entityList([](WrappedEntity *)
          {
          })
    {
    }
    void invalidate()
    {
        cachedMeshesUpToDate = false;
        invalidateCount++;
    }
};

struct BlockChunkChunkVariables final
{
    std::mutex cachedMeshesLock;
    std::shared_ptr<enum_array<Mesh, RenderLayer>> cachedMeshes;
    bool generatingCachedMeshes = false;
    std::condition_variable cachedMeshesCond;
    std::atomic_bool cachedMeshesUpToDate;
    WorldLightingProperties wlp;
    BlockChunkChunkVariables()
        : cachedMeshes(nullptr), cachedMeshesUpToDate(false), entityList([](WrappedEntity *v)
        {
            delete v;
        }), generated(false), generateStarted(false)
    {
    }
    BlockChunkChunkVariables(const BlockChunkChunkVariables &rt)
        : BlockChunkChunkVariables()
    {
    }
    std::mutex blockUpdateListLock;
    BlockUpdate *blockUpdateListHead = nullptr;
    BlockUpdate *blockUpdateListTail = nullptr;
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
};

struct BlockChunk final : public BasicBlockChunk<BlockChunkBlock, BlockChunkBiome, BlockChunkSubchunk, BlockChunkChunkVariables>
{
    static_assert(BlockChunkBlock::blockKindBitWidth >= 3 * subchunkShiftXYZ, "BlockChunkBlock::blockKindBitWidth is too small");
    static_assert(BlockOptionalData::BlockChunkSubchunkShiftXYZ == subchunkShiftXYZ, "BlockOptionalData::BlockChunkSubchunkShiftXYZ is wrong value");
    ObjectCounter<BlockChunk, 0> objectCounter;
    explicit BlockChunk(PositionI basePosition);
    ~BlockChunk();
    static Block getBlockFromArray(VectorI subchunkRelativePosition, const BlockChunkBlock &blockChunkBlock, BlockChunkSubchunk &subchunk)
    {
        BlockDescriptorPointer bd = subchunk.getBlockKind(blockChunkBlock);
        Lighting lighting = blockChunkBlock.getLighting();
        return Block(bd, lighting, subchunk.blockOptionalData.getData(subchunkRelativePosition));
    }
    static void putBlockIntoArray(VectorI subchunkRelativePosition, BlockChunkBlock &blockChunkBlock, BlockChunkSubchunk &subchunk, Block newBlock) /// @note doesn't handle updates or particles or anything except copying the members from Block
    {
        subchunk.setBlockKind(blockChunkBlock, newBlock.descriptor);
        blockChunkBlock.setLighting(newBlock.lighting);
        subchunk.blockOptionalData.setData(subchunkRelativePosition, std::move(newBlock.data));
    }
};

typedef ChunkMap<BlockChunk> BlockChunkMap;
typedef BasicBlockChunkRelativePositionIterator<BlockChunk> BlockChunkRelativePositionIterator;

class BlockChunkFullLock final
{
    BlockChunk &chunk;
    struct SubchunkLockIterator
    {
        VectorI pos;
        BlockChunk *chunk;
        generic_lock_wrapper &operator *() const
        {
            return chunk->subchunks[pos.x][pos.y][pos.z].lock;
        }
        generic_lock_wrapper *operator ->() const
        {
            return &operator *();
        }
        bool operator ==(const SubchunkLockIterator &rt) const
        {
            return rt.pos == pos;
        }
        bool operator !=(const SubchunkLockIterator &rt) const
        {
            return rt.pos != pos;
        }
        const SubchunkLockIterator &operator ++()
        {
            pos.x++;
            if(pos.x >= BlockChunk::subchunkCountX)
            {
                pos.x = 0;
                pos.y++;
                if(pos.y >= BlockChunk::subchunkCountY)
                {
                    pos.y = 0;
                    pos.z++;
                }
            }
            return *this;
        }
        SubchunkLockIterator operator ++(int)
        {
            SubchunkLockIterator retval = *this;
            operator ++();
            return std::move(retval);
        }
        SubchunkLockIterator()
        {
        }
        SubchunkLockIterator(VectorI pos, BlockChunk *chunk)
        {
        }
    };
public:
    explicit BlockChunkFullLock(BlockChunk &chunk)
        : chunk(chunk)
    {
        SubchunkLockIterator begin(VectorI(0, 0, 0), &chunk), end(VectorI(0, 0, BlockChunk::subchunkCountZ), &chunk);
        lock_all(begin, end);
    }
    ~BlockChunkFullLock()
    {
        SubchunkLockIterator begin(VectorI(0, 0, 0), &chunk), end(VectorI(0, 0, BlockChunk::subchunkCountZ), &chunk);
        for(auto i = begin; i != end; i++)
            i->unlock();
    }
    BlockChunkFullLock(const BlockChunkFullLock &) = delete;
    const BlockChunkFullLock operator =(const BlockChunkFullLock &) = delete;
};

class BlockChunkColumnLock final
{
    BlockChunk &chunk;
    struct SubchunkLockIterator
    {
        VectorI pos;
        BlockChunk *chunk;
        generic_lock_wrapper &operator *() const
        {
            return chunk->subchunks[pos.x][0][pos.z].lock;
        }
        generic_lock_wrapper *operator ->() const
        {
            return &operator *();
        }
        bool operator ==(const SubchunkLockIterator &rt) const
        {
            return rt.pos == pos;
        }
        bool operator !=(const SubchunkLockIterator &rt) const
        {
            return rt.pos != pos;
        }
        const SubchunkLockIterator &operator ++()
        {
            pos.x++;
            if(pos.x >= BlockChunk::subchunkCountX)
            {
                pos.x = 0;
                pos.z++;
            }
            return *this;
        }
        SubchunkLockIterator operator ++(int)
        {
            SubchunkLockIterator retval = *this;
            operator ++();
            return std::move(retval);
        }
        SubchunkLockIterator()
        {
        }
        SubchunkLockIterator(VectorI pos, BlockChunk *chunk)
        {
        }
    };
public:
    explicit BlockChunkColumnLock(BlockChunk &chunk)
        : chunk(chunk)
    {
        SubchunkLockIterator begin(VectorI(0, 0, 0), &chunk), end(VectorI(0, 0, BlockChunk::subchunkCountZ), &chunk);
        lock_all(begin, end);
    }
    ~BlockChunkColumnLock()
    {
        SubchunkLockIterator begin(VectorI(0, 0, 0), &chunk), end(VectorI(0, 0, BlockChunk::subchunkCountZ), &chunk);
        for(auto i = begin; i != end; i++)
            i->unlock();
    }
    BlockChunkColumnLock(const BlockChunkColumnLock &) = delete;
    const BlockChunkColumnLock operator =(const BlockChunkColumnLock &) = delete;
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
