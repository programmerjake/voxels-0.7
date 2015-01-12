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
    friend class BlockChunkChunkVariables;
private:
    BlockUpdate *chunk_prev;
    BlockUpdate *chunk_next;
    BlockUpdate *block_next;
    PositionI position;
    float time_left;
    BlockUpdateKind kind;
    BlockUpdate(BlockUpdateKind kind, PositionI position, float time_left, BlockUpdate *block_next = nullptr)
        : world_prev(nullptr), world_next(nullptr), block_next(block_next), position(position), time_left(time_left), kind(kind)
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

#if 0
namespace stream
{
template <typename T, size_t ChunkSizeXV, size_t ChunkSizeYV, size_t ChunkSizeZV, bool TransmitCompressedV>
struct is_value_changed<BlockChunk<T, ChunkSizeXV, ChunkSizeYV, ChunkSizeZV, TransmitCompressedV>>
{
    bool operator ()(std::shared_ptr<const BlockChunk<T, ChunkSizeXV, ChunkSizeYV, ChunkSizeZV, TransmitCompressedV>> value, VariableSet &variableSet) const
    {
        if(value == nullptr)
            return false;
        return value->changeTracker.getChanged(variableSet);
    }
};
}
#endif
}
}

#endif // BLOCK_CHUNK_H_INCLUDED
