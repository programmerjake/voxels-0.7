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
#ifndef BLOCK_ITERATOR_H_INCLUDED
#define BLOCK_ITERATOR_H_INCLUDED

#include "util/position.h"
#include "util/block_chunk.h"
#include "util/block_face.h"
#include <unordered_map>
#include <cassert>
#include <tuple>

using namespace std;

class BlockIterator final
{
    BlockChunk<Block> *chunk;
    unordered_map<PositionI, BlockChunk<Block>> *chunks;
    PositionI currentBasePosition;
    VectorI currentRelativePosition;
    void getChunk()
    {
        auto iter = chunks->find(currentBasePosition);
        if(iter == chunks->end())
        {
            iter = std::get<0>(chunks->insert(pair<PositionI, BlockChunk<Block>>(currentBasePosition, BlockChunk<Block>(currentBasePosition))));
        }
        chunk = &std::get<1>(*iter);
    }
    BlockIterator(BlockChunk<Block> *chunk, unordered_map<PositionI, BlockChunk<Block>> *chunks, PositionI currentBasePosition, VectorI currentRelativePosition)
        : chunk(chunk), chunks(chunks), currentBasePosition(currentBasePosition), currentRelativePosition(currentRelativePosition)
    {
    }
public:
    BlockIterator(unordered_map<PositionI, BlockChunk<Block>> *chunks, PositionI position)
        : chunks(chunks), currentBasePosition(BlockChunk<Block>::getChunkBasePosition(position)), currentRelativePosition(BlockChunk<Block>::getChunkRelativePosition(position))
    {
        assert(chunks != nullptr);
        getChunk();
    }
    PositionI position() const
    {
        return currentBasePosition + currentRelativePosition;
    }
    void moveTowardNX()
    {
        if(currentRelativePosition.x <= 0)
        {
            currentRelativePosition.x = BlockChunk<Block>::chunkSizeX - 1;
            currentBasePosition.x -= BlockChunk<Block>::chunkSizeX;
            getChunk();
        }
        else
            currentRelativePosition.x--;
    }
    void moveTowardNY()
    {
        if(currentRelativePosition.y <= 0)
        {
            currentRelativePosition.y = BlockChunk<Block>::chunkSizeY - 1;
            currentBasePosition.y -= BlockChunk<Block>::chunkSizeY;
            getChunk();
        }
        else
            currentRelativePosition.y--;
    }
    void moveTowardNZ()
    {
        if(currentRelativePosition.z <= 0)
        {
            currentRelativePosition.z = BlockChunk<Block>::chunkSizeZ - 1;
            currentBasePosition.z -= BlockChunk<Block>::chunkSizeZ;
            getChunk();
        }
        else
            currentRelativePosition.z--;
    }
    void moveTowardPX()
    {
        if(currentRelativePosition.x >= BlockChunk<Block>::chunkSizeX - 1)
        {
            currentRelativePosition.x = 0;
            currentBasePosition.x += BlockChunk<Block>::chunkSizeX;
            getChunk();
        }
        else
            currentRelativePosition.x++;
    }
    void moveTowardPY()
    {
        if(currentRelativePosition.y >= BlockChunk<Block>::chunkSizeY - 1)
        {
            currentRelativePosition.y = 0;
            currentBasePosition.y += BlockChunk<Block>::chunkSizeY;
            getChunk();
        }
        else
            currentRelativePosition.y++;
    }
    void moveTowardPZ()
    {
        if(currentRelativePosition.z >= BlockChunk<Block>::chunkSizeZ - 1)
        {
            currentRelativePosition.z = 0;
            currentBasePosition.z += BlockChunk<Block>::chunkSizeZ;
            getChunk();
        }
        else
            currentRelativePosition.z++;
    }
    void moveToward(BlockFace bf)
    {
        switch(bf)
        {
        case BlockFace::NX:
            moveTowardNX();
            break;
        case BlockFace::PX:
            moveTowardPX();
            break;
        case BlockFace::NY:
            moveTowardNY();
            break;
        case BlockFace::PY:
            moveTowardPY();
            break;
        case BlockFace::NZ:
            moveTowardNZ();
            break;
        case BlockFace::PZ:
            moveTowardPZ();
            break;
        }
    }
    void moveFrom(BlockFace bf)
    {
        switch(bf)
        {
        case BlockFace::NX:
            moveTowardPX();
            break;
        case BlockFace::PX:
            moveTowardNX();
            break;
        case BlockFace::NY:
            moveTowardPY();
            break;
        case BlockFace::PY:
            moveTowardNY();
            break;
        case BlockFace::NZ:
            moveTowardPZ();
            break;
        case BlockFace::PZ:
            moveTowardNZ();
            break;
        }
    }
    void moveBy(VectorI delta)
    {
        currentRelativePosition += delta;
        VectorI deltaBasePosition = BlockChunk<Block>::getChunkBasePosition(currentRelativePosition);
        if(deltaBasePosition != VectorI(0))
        {
            currentBasePosition += deltaBasePosition;
            currentRelativePosition -= deltaBasePosition;
            getChunk();
        }
    }
    void moveTo(VectorI pos)
    {
        moveBy(pos - (VectorI)position());
    }
    void moveTo(PositionI pos)
    {
        if(pos.d == currentBasePosition.d)
        {
            moveTo((VectorI)pos);
        }
        else
        {
            currentBasePosition = BlockChunk<Block>::getChunkBasePosition(pos);
            currentRelativePosition = BlockChunk<Block>::getChunkRelativePosition(pos);
            getChunk();
        }
    }
    const Block &operator *() const
    {
        return chunk->blocks[currentRelativePosition.x][currentRelativePosition.y][currentRelativePosition.z];
    }
    const Block *operator ->() const
    {
        return &chunk->blocks[currentRelativePosition.x][currentRelativePosition.y][currentRelativePosition.z];
    }
};

#endif // BLOCK_ITERATOR_H_INCLUDED