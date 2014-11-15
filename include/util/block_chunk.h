#ifndef BLOCK_CHUNK_H_INCLUDED
#define BLOCK_CHUNK_H_INCLUDED

#include "util/position.h"
#include "stream/stream.h"
#include "util/variable_set.h"
#include "stream/compressed_stream.h"
#include <array>

using namespace std;

template <typename BT, typename SCT, typename CVT, size_t ChunkShiftXV = 4, size_t ChunkShiftYV = 8, size_t ChunkShiftZV = 4, size_t SubchunkShiftXYZV = 2, bool TransmitCompressedV = true>
struct BlockChunk
{
    typedef BT BlockType;
    typedef SCT SubchunkType;
    typedef CVT ChunkVariablesType;
    const PositionI basePosition;
    static constexpr size_t chunkShiftX = ChunkShiftXV;
    static constexpr size_t chunkShiftY = ChunkShiftYV;
    static constexpr size_t chunkShiftZ = ChunkShiftZV;
    static constexpr size_t subchunkShiftXYZ = SubchunkShiftXYZV;
    static constexpr int32_t chunkSizeX = (int32_t)1 << chunkShiftX;
    static constexpr int32_t chunkSizeY = (int32_t)1 << chunkShiftY;
    static constexpr int32_t chunkSizeZ = (int32_t)1 << chunkShiftZ;
    static constexpr int32_t subchunkSizeXYZ = (int32_t)1 << subchunkShiftXYZ;
    static_assert(chunkSizeX > 0, "chunkSizeX must be positive");
    static_assert(chunkSizeY > 0, "chunkSizeY must be positive");
    static_assert(chunkSizeZ > 0, "chunkSizeZ must be positive");
    static_assert(subchunkSizeXYZ > 0, "subchunkSizeXYZ must be positive");
    static_assert((chunkSizeX & (chunkSizeX - 1)) == 0, "chunkSizeX must be a power of 2");
    static_assert((chunkSizeY & (chunkSizeY - 1)) == 0, "chunkSizeY must be a power of 2");
    static_assert((chunkSizeZ & (chunkSizeZ - 1)) == 0, "chunkSizeZ must be a power of 2");
    static_assert((subchunkSizeXYZ & (subchunkSizeXYZ - 1)) == 0, "subchunkSizeXYZ must be a power of 2");
    static_assert(subchunkSizeXYZ <= chunkSizeX && subchunkSizeXYZ <= chunkSizeY && subchunkSizeXYZ <= chunkSizeZ, "subchunkSizeXYZ must not be bigger than the chunk size")
    static constexpr bool transmitCompressed = TransmitCompressedV;
    mutable ChangeTracker changeTracker;
    constexpr BlockChunk(PositionI basePosition)
        : basePosition(basePosition)
    {
    }
    static constexpr PositionI getChunkBasePosition(PositionI pos)
    {
        return PositionI(pos.x & ~(chunkSizeX - 1), pos.y & ~(chunkSizeY - 1), pos.z & ~(chunkSizeZ - 1), pos.d);
    }
    static constexpr VectorI getChunkBasePosition(VectorI pos)
    {
        return VectorI(pos.x & ~(chunkSizeX - 1), pos.y & ~(chunkSizeY - 1), pos.z & ~(chunkSizeZ - 1));
    }
    static constexpr PositionI getChunkRelativePosition(PositionI pos)
    {
        return PositionI(pos.x & (chunkSizeX - 1), pos.y & (chunkSizeY - 1), pos.z & (chunkSizeZ - 1), pos.d);
    }
    static constexpr VectorI getChunkRelativePosition(VectorI pos)
    {
        return VectorI(pos.x & (chunkSizeX - 1), pos.y & (chunkSizeY - 1), pos.z & (chunkSizeZ - 1));
    }
    static constexpr int32_t getSubchunkBaseAbsolutePosition(int32_t v)
    {
        return v & ~(subchunkSizeXYZ - 1);
    }
    static constexpr VectorI getSubchunkBaseAbsolutePosition(VectorI pos)
    {
        return VectorI(getSubchunkBaseAbsolutePosition(pos.x), getSubchunkBaseAbsolutePosition(pos.y), getSubchunkBaseAbsolutePosition(pos.z));
    }
    static constexpr PositionI getSubchunkBaseAbsolutePosition(PositionI pos)
    {
        return PositionI(getSubchunkBaseAbsolutePosition(pos.x), getSubchunkBaseAbsolutePosition(pos.y), getSubchunkBaseAbsolutePosition(pos.z), pos.d);
    }
    static constexpr VectorI getSubchunkBaseRelativePosition(VectorI pos)
    {
        return VectorI(pos.x & ((chunkSizeX - 1) & ~(subchunkSizeXYZ - 1)), pos.y & ((chunkSizeY - 1) & ~(subchunkSizeXYZ - 1)), pos.z & ((chunkSizeZ - 1) & ~(subchunkSizeXYZ - 1)));
    }
    static constexpr VectorI getSubchunkIndexFromChunkRelativePosition(VectorI pos)
    {
        return VectorI(pos.x >> subchunkShiftXYZ, pos.y >> subchunkShiftXYZ, pos.z >> subchunkShiftXYZ);
    }
    static constexpr VectorI getSubchunkIndexFromPosition(VectorI pos)
    {
        return getSubchunkIndexFromChunkRelativePosition(getChunkRelativePosition(pos));
    }
    static constexpr PositionI getSubchunkBaseRelativePosition(PositionI pos)
    {
        return PositionI(pos.x & ((chunkSizeX - 1) & ~(subchunkSizeXYZ - 1)), pos.y & ((chunkSizeY - 1) & ~(subchunkSizeXYZ - 1)), pos.z & ((chunkSizeZ - 1) & ~(subchunkSizeXYZ - 1)), pos.d);
    }
    static constexpr int32_t getSubchunkRelativePosition(int32_t v)
    {
        return v & (subchunkSizeXYZ - 1);
    }
    static constexpr VectorI getSubchunkRelativePosition(VectorI pos)
    {
        return VectorI(getSubchunkRelativePosition(pos.x), getSubchunkRelativePosition(pos.y), getSubchunkRelativePosition(pos.z));
    }
    static constexpr PositionI getSubchunkRelativePosition(PositionI pos)
    {
        return PositionI(getSubchunkRelativePosition(pos.x), getSubchunkRelativePosition(pos.y), getSubchunkRelativePosition(pos.z), pos.d);
    }
    array<array<array<BlockType, chunkSizeZ>, chunkSizeY>, chunkSizeX> blocks;
    array<array<array<SubchunkType, subchunkSizeXYZ>, subchunkSizeXYZ>, subchunkSizeXYZ> subchunks;
    ChunkVariablesType chunkVariables;
    BlockChunk(const BlockChunk & rt)
        : basePosition(rt.basePosition), blocks(rt.blocks), subchunks(rt.subchunks), chunkVariables(chunkVariables)
    {
        changeTracker.onChange();
    }
private:
    static void readInternal(shared_ptr<BlockChunk> chunk, stream::Reader &reader, VariableSet &variableSet)
    {
        for(int32_t x = 0; x < chunkSizeX; x++)
        {
            for(int32_t y = 0; y < chunkSizeY; y++)
            {
                for(int32_t z = 0; z < chunkSizeZ; z++)
                {
                    chunk->blocks[x][y][z] = (T)stream::read<T>(reader, variableSet);
                }
            }
        }
    }
    void writeInternal(stream::Writer &writer, VariableSet &variableSet) const
    {
        for(int32_t x = 0; x < chunkSizeX; x++)
        {
            for(int32_t y = 0; y < chunkSizeY; y++)
            {
                for(int32_t z = 0; z < chunkSizeZ; z++)
                {
                    stream::write<T>(writer, variableSet, blocks[x][y][z]);
                }
            }
        }
        changeTracker.onWrite(variableSet);
    }
    static bool checkPosition(PositionI p)
    {
        return p.x % chunkSizeX == 0 && p.y % chunkSizeY == 0 && p.z % chunkSizeZ == 0;
    }
    static void readTemplateParameters(stream::Reader &reader)
    {
        stream::read_limited<int32_t>(reader, chunkSizeX, chunkSizeX);
        stream::read_limited<int32_t>(reader, chunkSizeY, chunkSizeY);
        stream::read_limited<int32_t>(reader, chunkSizeZ, chunkSizeZ);
        stream::read_checked<bool>(reader, [](bool v){return v == transmitCompressed;});
    }
    static void writeTemplateParameters(stream::Writer &writer)
    {
        stream::write<int32_t>(writer, chunkSizeX);
        stream::write<int32_t>(writer, chunkSizeY);
        stream::write<int32_t>(writer, chunkSizeZ);
        stream::write<bool>(writer, transmitCompressed);
    }
public:
    static shared_ptr<BlockChunk> read(stream::Reader &reader, VariableSet &variableSet)
    {
        PositionI basePosition = stream::read_checked<PositionI>(reader, checkPosition);
        readTemplateParameters(reader);
        shared_ptr<BlockChunk> retval = shared_ptr<BlockChunk>(new BlockChunk(basePosition));
        if(transmitCompressed)
        {
            stream::ExpandReader expandReader(reader);
            readInternal(retval, expandReader, variableSet);
        }
        else
            readInternal(retval, reader, variableSet);
        return retval;
    }
    void write(stream::Writer &writer, VariableSet &variableSet) const
    {
        stream::write<PositionI>(writer, basePosition);
        writeTemplateParameters(writer);
        if(transmitCompressed)
        {
            stream::CompressWriter compressWriter(writer);
            writeInternal(compressWriter, variableSet);
            compressWriter.finish();
        }
        else
            writeInternal(writer, variableSet);
    }
    void onChange()
    {
        changeTracker.onChange();
    }
};

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

#endif // BLOCK_CHUNK_H_INCLUDED
