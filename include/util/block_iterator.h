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
#include "util/cached_variable.h"
#include <unordered_map>
#include <cassert>
#include <tuple>

namespace programmerjake
{
namespace voxels
{
class BlockIterator final
{
    friend class World;
public:
    typedef Block BlockType;
    typedef BlockChunk ChunkType;
private:
    ChunkType *chunk;
    std::unordered_map<PositionI, ChunkType> *chunks;
    PositionI currentBasePosition;
    VectorI currentRelativePosition;
    BlockChunkReaderLock rlock;
    void getChunk()
    {
        auto iter = chunks->find(currentBasePosition);
        if(iter == chunks->end())
        {
            iter = std::get<0>(chunks->insert(std::pair<PositionI, ChunkType>(currentBasePosition, ChunkType(currentBasePosition))));
        }
        chunk = &std::get<1>(*iter);
    }
    void setLock(BlockChunkLockType &new_lock)
    {
        if(rlock.mutex() != &new_lock)
            rlock = BlockChunkReaderLock(new_lock);
    }
    void lockSubchunk()
    {
        VectorI currentSubchunkIndex = ChunkType::getSubchunkIndexFromChunkRelativePosition(currentRelativePosition);
        setLock(chunk->subchunks[currentSubchunkIndex.x][currentSubchunkIndex.y][currentSubchunkIndex.z].lock);
    }
    void unlock()
    {
        rlock.unlock();
    }
    BlockIterator(ChunkType *chunk, std::unordered_map<PositionI, ChunkType> *chunks, PositionI currentBasePosition, VectorI currentRelativePosition)
        : chunk(chunk), chunks(chunks), currentBasePosition(currentBasePosition), currentRelativePosition(currentRelativePosition)
    {
        lockSubchunk();
    }
public:
    BlockIterator(std::unordered_map<PositionI, ChunkType> *chunks, PositionI position)
        : chunks(chunks), currentBasePosition(ChunkType::getChunkBasePosition(position)), currentRelativePosition(ChunkType::getChunkRelativePosition(position))
    {
        assert(chunks != nullptr);
        getChunk();
        lockSubchunk();
    }
    BlockIterator(const BlockIterator &rt)
        : chunk(rt.chunk), chunks(rt.chunks), currentBasePosition(rt.currentBasePosition), currentRelativePosition(rt.currentRelativePosition)
    {
        lockSubchunk();
    }
    PositionI position() const
    {
        return currentBasePosition + currentRelativePosition;
    }
    void moveTowardNX()
    {
        if(currentRelativePosition.x <= 0)
        {
            currentRelativePosition.x = ChunkType::chunkSizeX - 1;
            currentBasePosition.x -= ChunkType::chunkSizeX;
            getChunk();
        }
        else
            currentRelativePosition.x--;
        lockSubchunk();
    }
    void moveTowardNY()
    {
        if(currentRelativePosition.y <= 0)
        {
            currentRelativePosition.y = ChunkType::chunkSizeY - 1;
            currentBasePosition.y -= ChunkType::chunkSizeY;
            getChunk();
        }
        else
            currentRelativePosition.y--;
        lockSubchunk();
    }
    void moveTowardNZ()
    {
        if(currentRelativePosition.z <= 0)
        {
            currentRelativePosition.z = ChunkType::chunkSizeZ - 1;
            currentBasePosition.z -= ChunkType::chunkSizeZ;
            getChunk();
        }
        else
            currentRelativePosition.z--;
        lockSubchunk();
    }
    void moveTowardPX()
    {
        if(currentRelativePosition.x >= ChunkType::chunkSizeX - 1)
        {
            currentRelativePosition.x = 0;
            currentBasePosition.x += ChunkType::chunkSizeX;
            getChunk();
        }
        else
            currentRelativePosition.x++;
        lockSubchunk();
    }
    void moveTowardPY()
    {
        if(currentRelativePosition.y >= ChunkType::chunkSizeY - 1)
        {
            currentRelativePosition.y = 0;
            currentBasePosition.y += ChunkType::chunkSizeY;
            getChunk();
        }
        else
            currentRelativePosition.y++;
        lockSubchunk();
    }
    void moveTowardPZ()
    {
        if(currentRelativePosition.z >= ChunkType::chunkSizeZ - 1)
        {
            currentRelativePosition.z = 0;
            currentBasePosition.z += ChunkType::chunkSizeZ;
            getChunk();
        }
        else
            currentRelativePosition.z++;
        lockSubchunk();
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
        VectorI deltaBasePosition = ChunkType::getChunkBasePosition(currentRelativePosition);
        if(deltaBasePosition != VectorI(0))
        {
            currentBasePosition += deltaBasePosition;
            currentRelativePosition -= deltaBasePosition;
            getChunk();
        }
        lockSubchunk();
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
            currentBasePosition = ChunkType::getChunkBasePosition(pos);
            currentRelativePosition = ChunkType::getChunkRelativePosition(pos);
            getChunk();
            lockSubchunk();
        }
    }
    const BlockType &operator *() const
    {
        return chunk->blocks[currentRelativePosition.x][currentRelativePosition.y][currentRelativePosition.z].block;
    }
    const BlockType *operator ->() const
    {
        return &chunk->blocks[currentRelativePosition.x][currentRelativePosition.y][currentRelativePosition.z].block;
    }
};
}
}

#endif // BLOCK_ITERATOR_H_INCLUDED
