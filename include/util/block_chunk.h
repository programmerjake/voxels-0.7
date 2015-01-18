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
#include <mutex>
#include "lighting/lighting.h"
#include <array>
#include <memory>
#include <atomic>
#include <iterator>

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
    Block block;
    BlockUpdate *updateListHead = nullptr; // BlockChunkChunkVariables is responsible for deleting
    BlockLighting lighting;
    bool lightingValid = false;
    BlockChunkBlock() = default;
    BlockChunkBlock(const BlockChunkBlock &rt)
        : block(rt.block), updateListHead(nullptr), lighting(rt.lighting), lightingValid(rt.lightingValid)
    {
    }
    const BlockChunkBlock &operator =(const BlockChunkBlock &) = delete;
};

typedef std::mutex BlockChunkLockType;

struct BlockChunkSubchunk final
{
    BlockChunkLockType lock;
    std::atomic<std::shared_ptr<Mesh>> cachedMesh;
    BlockChunkSubchunk(const BlockChunkSubchunk &rt)
        : cachedMesh(nullptr)
    {
    }
    BlockChunkSubchunk() = default;
};

struct BlockChunkChunkVariables final
{
    std::atomic<std::shared_ptr<Mesh>> cachedMesh;
    BlockChunkChunkVariables(const BlockChunkChunkVariables &rt)
        : cachedMesh(nullptr)
    {
    }
    BlockChunkChunkVariables() = default;
    std::mutex blockUpdateListLock;
    BlockUpdate *blockUpdateListHead = nullptr;
    BlockUpdate *blockUpdateListTail = nullptr;
    ~BlockChunkChunkVariables()
    {
        while(blockUpdateListHead != nullptr)
        {
            BlockUpdate *deleteMe = blockUpdateListHead;
            blockUpdateListHead = blockUpdateListHead->chunk_next;
            delete deleteMe;
        }
    }
};

typedef BasicBlockChunk<BlockChunkBlock, BlockChunkSubchunk, BlockChunkChunkVariables> BlockChunk;

class BlockChunkFullLock final
{
    BlockChunk &chunk;
public:
    explicit BlockChunkFullLock(BlockChunk &chunk)
        : chunk(chunk)
    {
        BlockChunkLockType *failed_lock = nullptr;

        for(;;)
        {
try_again:

            if(failed_lock != nullptr)
            {
                failed_lock->lock();
            }

            for(std::int32_t x = 0; x < BlockChunk::subchunkSizeXYZ; x++)
            {
                for(std::int32_t y = 0; y < BlockChunk::subchunkSizeXYZ; y++)
                {
                    for(std::int32_t z = 0; z < BlockChunk::subchunkSizeXYZ; z++)
                    {
                        BlockChunkLockType &lock = chunk.subchunks[x][y][z].lock;

                        if(&lock == failed_lock)
                        {
                            continue;
                        }

                        bool locked;

                        try
                        {
                            locked = lock.try_lock();
                        }
                        catch(...)
                        {
                            for(z--; z >= 0; z--)
                            {
                                if(&chunk.subchunks[x][y][z].lock != failed_lock)
                                {
                                    chunk.subchunks[x][y][z].lock.unlock();
                                }
                            }

                            for(y--; y >= 0; y--)
                            {
                                for(std::int32_t z = 0; z < BlockChunk::subchunkSizeXYZ; z++)
                                {
                                    if(&chunk.subchunks[x][y][z].lock != failed_lock)
                                    {
                                        chunk.subchunks[x][y][z].lock.unlock();
                                    }
                                }
                            }

                            for(x--; x >= 0; x--)
                            {
                                for(std::int32_t y = 0; y < BlockChunk::subchunkSizeXYZ; y++)
                                {
                                    for(std::int32_t z = 0; z < BlockChunk::subchunkSizeXYZ; z++)
                                    {
                                        if(&chunk.subchunks[x][y][z].lock != failed_lock)
                                        {
                                            chunk.subchunks[x][y][z].lock.unlock();
                                        }
                                    }
                                }
                            }

                            throw;
                        }

                        if(!locked)
                        {
                            for(z--; z >= 0; z--)
                            {
                                if(&chunk.subchunks[x][y][z].lock != failed_lock)
                                {
                                    chunk.subchunks[x][y][z].lock.unlock();
                                }
                            }

                            for(y--; y >= 0; y--)
                            {
                                for(std::int32_t z = 0; z < BlockChunk::subchunkSizeXYZ; z++)
                                {
                                    if(&chunk.subchunks[x][y][z].lock != failed_lock)
                                    {
                                        chunk.subchunks[x][y][z].lock.unlock();
                                    }
                                }
                            }

                            for(x--; x >= 0; x--)
                            {
                                for(std::int32_t y = 0; y < BlockChunk::subchunkSizeXYZ; y++)
                                {
                                    for(std::int32_t z = 0; z < BlockChunk::subchunkSizeXYZ; z++)
                                    {
                                        if(&chunk.subchunks[x][y][z].lock != failed_lock)
                                        {
                                            chunk.subchunks[x][y][z].lock.unlock();
                                        }
                                    }
                                }
                            }

                            failed_lock->unlock();
                            failed_lock = &chunk.subchunks[x][y][z].lock;
                            goto try_again;
                        }
                    }
                }
            }
        }
    }
    ~BlockChunkFullLock()
    {
        for(std::int32_t x = 0; x < BlockChunk::subchunkSizeXYZ; x++)
        {
            for(std::int32_t y = 0; y < BlockChunk::subchunkSizeXYZ; y++)
            {
                for(std::int32_t z = 0; z < BlockChunk::subchunkSizeXYZ; z++)
                {
                    chunk.subchunks[x][y][z].lock.unlock();
                }
            }
        }
    }
    BlockChunkFullLock(const BlockChunkFullLock &) = delete;

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
