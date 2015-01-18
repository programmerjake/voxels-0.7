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
#ifndef BASIC_BLOCK_CHUNK_H_INCLUDED
#define BASIC_BLOCK_CHUNK_H_INCLUDED

#include "util/position.h"
#include <array>

namespace programmerjake
{
namespace voxels
{
template <typename BT, typename SCT, typename CVT, std::size_t ChunkShiftXV = 4, std::size_t ChunkShiftYV = 8, std::size_t ChunkShiftZV = 4, std::size_t SubchunkShiftXYZV = 2>
struct BasicBlockChunk
{
    typedef BT BlockType;
    typedef SCT SubchunkType;
    typedef CVT ChunkVariablesType;
    const PositionI basePosition;
    static constexpr std::size_t chunkShiftX = ChunkShiftXV;
    static constexpr std::size_t chunkShiftY = ChunkShiftYV;
    static constexpr std::size_t chunkShiftZ = ChunkShiftZV;
    static constexpr std::size_t subchunkShiftXYZ = SubchunkShiftXYZV;
    static constexpr std::int32_t chunkSizeX = (std::int32_t)1 << chunkShiftX;
    static constexpr std::int32_t chunkSizeY = (std::int32_t)1 << chunkShiftY;
    static constexpr std::int32_t chunkSizeZ = (std::int32_t)1 << chunkShiftZ;
    static constexpr std::int32_t subchunkSizeXYZ = (std::int32_t)1 << subchunkShiftXYZ;
    static_assert(chunkSizeX > 0, "chunkSizeX must be positive");
    static_assert(chunkSizeY > 0, "chunkSizeY must be positive");
    static_assert(chunkSizeZ > 0, "chunkSizeZ must be positive");
    static_assert(subchunkSizeXYZ > 0, "subchunkSizeXYZ must be positive");
    static_assert((chunkSizeX & (chunkSizeX - 1)) == 0, "chunkSizeX must be a power of 2");
    static_assert((chunkSizeY & (chunkSizeY - 1)) == 0, "chunkSizeY must be a power of 2");
    static_assert((chunkSizeZ & (chunkSizeZ - 1)) == 0, "chunkSizeZ must be a power of 2");
    static_assert((subchunkSizeXYZ & (subchunkSizeXYZ - 1)) == 0, "subchunkSizeXYZ must be a power of 2");
    static_assert(subchunkSizeXYZ <= chunkSizeX && subchunkSizeXYZ <= chunkSizeY && subchunkSizeXYZ <= chunkSizeZ, "subchunkSizeXYZ must not be bigger than the chunk size");
    constexpr BasicBlockChunk(PositionI basePosition)
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
    static constexpr std::int32_t getSubchunkBaseAbsolutePosition(std::int32_t v)
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
    static constexpr std::int32_t getSubchunkRelativePosition(std::int32_t v)
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
    std::array<std::array<std::array<BlockType, chunkSizeZ>, chunkSizeY>, chunkSizeX> blocks;
    std::array<std::array<std::array<SubchunkType, subchunkSizeXYZ>, subchunkSizeXYZ>, subchunkSizeXYZ> subchunks;
    ChunkVariablesType chunkVariables;
};
}
}

#endif // BASIC_BLOCK_CHUNK_H_INCLUDED
