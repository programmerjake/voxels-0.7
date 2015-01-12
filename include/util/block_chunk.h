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

namespace programmerjake
{
namespace voxels
{
struct BlockChunkBlock final
{
    Block block;
    BlockLighting lighting;
    bool lightingValid = false;
};

typedef std::mutex BlockChunkLockType;

struct BlockChunkSubchunk final
{
    BlockChunkLockType lock;
    std::shared_ptr<Mesh> cachedMesh;
    BlockChunkSubchunk(const BlockChunkSubchunk &rt)
        : cachedMesh(nullptr)
    {
    }
    BlockChunkSubchunk() = default;
};

struct BlockChunkChunkVariables final
{

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
