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
#include "block/block.h"
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

namespace programmerjake
{
namespace voxels
{
struct WrappedEntity;

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
    BlockUpdate(BlockUpdateKind kind, PositionI position, float time_left, BlockUpdate *block_next = nullptr)
        : chunk_prev(nullptr), chunk_next(nullptr), block_next(block_next), position(position), time_left(time_left), kind(kind)
    {
    }
    BlockUpdate(const BlockUpdate &) = delete;
    const BlockUpdate &operator =(const BlockUpdate &) = delete;
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
    BlockUpdate &operator *()
    {
        return *node;
    }
    BlockUpdate *operator ->()
    {
        return node;
    }
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
    PackedBlock block;
    BlockUpdate *updateListHead = nullptr; // BlockChunkChunkVariables is responsible for deleting
    BlockChunkBlock() = default;
    BlockChunkBlock(const BlockChunkBlock &rt)
        : block(rt.block), updateListHead(nullptr)
    {
    }
    void invalidate()
    {
    }
    const BlockChunkBlock &operator =(const BlockChunkBlock &) = delete;
};

typedef std::uint64_t BlockChunkInvalidateCountType;

struct BlockChunkSubchunk final
{
    std::mutex lock;
    atomic_shared_ptr<enum_array<Mesh, RenderLayer>> cachedMeshes;
    WrappedEntity *entityListHead = nullptr;
    WrappedEntity *entityListTail = nullptr; // entities deleted by chunk
    BlockChunkInvalidateCountType invalidateCount = 0;
    BlockChunkSubchunk(const BlockChunkSubchunk &rt)
        : cachedMeshes(nullptr)
    {
    }
    BlockChunkSubchunk() = default;
    void invalidate()
    {
        cachedMeshes = nullptr;
        invalidateCount++;
    }
};

struct BlockChunkChunkVariables final
{
    atomic_shared_ptr<enum_array<Mesh, RenderLayer>> cachedMeshes;
    BlockChunkChunkVariables(const BlockChunkChunkVariables &rt)
        : cachedMeshes(nullptr), generated(false), generateStarted(false)
    {
    }
    BlockChunkChunkVariables()
        : generated(false), generateStarted(false)
    {
    }
    std::mutex blockUpdateListLock;
    BlockUpdate *blockUpdateListHead = nullptr;
    BlockUpdate *blockUpdateListTail = nullptr;
    std::recursive_mutex entityListLock;
    WrappedEntity *entityListHead = nullptr;
    WrappedEntity *entityListTail = nullptr;
    std::atomic_bool generated, generateStarted;
    ~BlockChunkChunkVariables();
    void invalidate()
    {
        cachedMeshes = nullptr;
    }
};

typedef BasicBlockChunk<BlockChunkBlock, BlockChunkSubchunk, BlockChunkChunkVariables> BlockChunk;
typedef ChunkMap<BlockChunk> BlockChunkMap;
typedef BasicBlockChunkRelativePositionIterator<BlockChunk> BlockChunkRelativePositionIterator;

class BlockChunkFullLock final
{
    BlockChunk &chunk;
    struct SubchunkLockIterator
    {
        VectorI pos;
        BlockChunk *chunk;
        std::mutex &operator *() const
        {
            return chunk->subchunks[pos.x][pos.y][pos.z].lock;
        }
        std::mutex *operator ->() const
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
