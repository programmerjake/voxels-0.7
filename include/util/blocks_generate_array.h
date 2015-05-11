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
#ifndef BLOCKS_GENERATE_ARRAY_H_INCLUDED
#define BLOCKS_GENERATE_ARRAY_H_INCLUDED

#include "block/block_struct.h"
#include "util/block_chunk.h"
#include "util/checked_array.h"
#include <cassert>

namespace programmerjake
{
namespace voxels
{
class BlocksGenerateArray final
{
private:
    checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> blocks;
public:
    class IndexHelper1;
    class IndexHelper2 final
    {
        friend class IndexHelper1;
    private:
        checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks;
        std::size_t indexX;
        std::size_t indexY;
        IndexHelper2(checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks, std::size_t indexX, std::size_t indexY)
            : blocks(blocks), indexX(indexX), indexY(indexY)
        {
        }
    public:
        Block &operator[](std::size_t indexZ)
        {
            return blocks[indexX][indexZ][indexY];
        }
        Block &at(std::size_t index)
        {
            assert(index < size());
            return operator [](index);
        }
        static std::size_t size()
        {
            return BlockChunk::chunkSizeZ;
        }
    };
    class IndexHelper1 final
    {
        friend class BlocksGenerateArray;
    private:
        checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks;
        std::size_t indexX;
        IndexHelper1(checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks, std::size_t indexX)
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
            return operator [](index);
        }
        static std::size_t size()
        {
            return BlockChunk::chunkSizeY;
        }
    };
    class ConstIndexHelper1;
    class ConstIndexHelper2 final
    {
        friend class ConstIndexHelper1;
    private:
        const checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks;
        std::size_t indexX;
        std::size_t indexY;
        ConstIndexHelper2(const checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks, std::size_t indexX, std::size_t indexY)
            : blocks(blocks), indexX(indexX), indexY(indexY)
        {
        }
    public:
        const Block &operator[](std::size_t indexZ)
        {
            return blocks[indexX][indexZ][indexY];
        }
        const Block &at(std::size_t index)
        {
            assert(index < size());
            return operator [](index);
        }
        static std::size_t size()
        {
            return BlockChunk::chunkSizeZ;
        }
    };
    class ConstIndexHelper1 final
    {
        friend class BlocksGenerateArray;
    private:
        const checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks;
        std::size_t indexX;
        ConstIndexHelper1(const checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeX> &blocks, std::size_t indexX)
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
            return operator [](index);
        }
        static std::size_t size()
        {
            return BlockChunk::chunkSizeY;
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
        return operator [](index);
    }
    IndexHelper1 at(std::size_t index)
    {
        assert(index < size());
        return operator [](index);
    }
    static std::size_t size()
    {
        return BlockChunk::chunkSizeX;
    }
};
}
}

#endif // BLOCKS_GENERATE_ARRAY_H_INCLUDED
