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
#include "block/block.h"
#ifndef BLOCK_ITERATOR_H_INCLUDED
#define BLOCK_ITERATOR_H_INCLUDED

#include "util/position.h"
#include "util/block_chunk.h"
#include "util/block_face.h"
#include "util/cached_variable.h"
#include <cassert>
#include <tuple>
#include <iostream>
#include <thread>
#include "util/logging.h"
#include "util/world_lock_manager.h"

namespace programmerjake
{
namespace voxels
{
class BlockIterator final
{
    friend class World;
    friend class ViewPoint;
    BlockChunk *chunk;
    BlockChunkMap *chunks;
    PositionI currentBasePosition;
    VectorI currentRelativePosition;
    void getChunk()
    {
        chunk = &(*chunks)[currentBasePosition];
    }
    BlockChunkSubchunk &getSubchunk() const
    {
        VectorI currentSubchunkIndex = BlockChunk::getSubchunkIndexFromChunkRelativePosition(currentRelativePosition);
        return chunk->subchunks[currentSubchunkIndex.x][currentSubchunkIndex.y][currentSubchunkIndex.z];
    }
    BlockChunkSubchunk &getBiomeSubchunk() const
    {
        VectorI currentSubchunkIndex = BlockChunk::getSubchunkIndexFromChunkRelativePosition(currentRelativePosition);
        return chunk->subchunks[currentSubchunkIndex.x][0][currentSubchunkIndex.z];
    }
    void updateLock(WorldLockManager &lock_manager) const
    {
        lock_manager.block_biome_lock.set(getSubchunk().lock);
    }
    void updateBiomeLock(WorldLockManager &lock_manager) const
    {
        lock_manager.block_biome_lock.set(getBiomeSubchunk().biome_lock);
    }
    BlockIterator(BlockChunk *chunk, BlockChunkMap *chunks, PositionI currentBasePosition, VectorI currentRelativePosition)
        : chunk(chunk), chunks(chunks), currentBasePosition(currentBasePosition), currentRelativePosition(currentRelativePosition)
    {
    }
public:
    BlockIterator(BlockChunkMap *chunks, PositionI position)
        : chunks(chunks), currentBasePosition(BlockChunk::getChunkBasePosition(position)), currentRelativePosition(BlockChunk::getChunkRelativePosition(position))
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
            currentRelativePosition.x = BlockChunk::chunkSizeX - 1;
            currentBasePosition.x -= BlockChunk::chunkSizeX;
            getChunk();
        }
        else
            currentRelativePosition.x--;
    }
    void moveTowardNY()
    {
        if(currentRelativePosition.y <= 0)
        {
            currentRelativePosition.y = BlockChunk::chunkSizeY - 1;
            currentBasePosition.y -= BlockChunk::chunkSizeY;
            getChunk();
        }
        else
            currentRelativePosition.y--;
    }
    void moveTowardNZ()
    {
        if(currentRelativePosition.z <= 0)
        {
            currentRelativePosition.z = BlockChunk::chunkSizeZ - 1;
            currentBasePosition.z -= BlockChunk::chunkSizeZ;
            getChunk();
        }
        else
            currentRelativePosition.z--;
    }
    void moveTowardPX()
    {
        if(currentRelativePosition.x >= BlockChunk::chunkSizeX - 1)
        {
            currentRelativePosition.x = 0;
            currentBasePosition.x += BlockChunk::chunkSizeX;
            getChunk();
        }
        else
            currentRelativePosition.x++;
    }
    void moveTowardPY()
    {
        if(currentRelativePosition.y >= BlockChunk::chunkSizeY - 1)
        {
            currentRelativePosition.y = 0;
            currentBasePosition.y += BlockChunk::chunkSizeY;
            getChunk();
        }
        else
            currentRelativePosition.y++;
    }
    void moveTowardPZ()
    {
        if(currentRelativePosition.z >= BlockChunk::chunkSizeZ - 1)
        {
            currentRelativePosition.z = 0;
            currentBasePosition.z += BlockChunk::chunkSizeZ;
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
        VectorI deltaBasePosition = BlockChunk::getChunkBasePosition(currentRelativePosition);
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
            currentBasePosition = BlockChunk::getChunkBasePosition(pos);
            currentRelativePosition = BlockChunk::getChunkRelativePosition(pos);
            getChunk();
        }
    }
private:
    BlockChunkBlock &getBlock() const
    {
        return chunk->blocks[currentRelativePosition.x][currentRelativePosition.y][currentRelativePosition.z];
    }
    BlockChunkBlock &getBlock(WorldLockManager &lock_manager) const
    {
        updateLock(lock_manager);
        return chunk->blocks[currentRelativePosition.x][currentRelativePosition.y][currentRelativePosition.z];
    }
    BlockChunkBiome &getBiome() const
    {
        return chunk->biomes[currentRelativePosition.x][currentRelativePosition.z];
    }
public:
    Block get(WorldLockManager &lock_manager) const
    {
        return (Block)getBlock(lock_manager).block;
    }
    const BiomeProperties &getBiomeProperties(WorldLockManager &lock_manager) const
    {
        updateBiomeLock(lock_manager);
        return getBiome().biomeProperties;
    }
    BlockUpdateIterator updatesBegin(WorldLockManager &lock_manager) const
    {
        return BlockUpdateIterator(getBlock(lock_manager).updateListHead);
    }
    BlockUpdateIterator updatesEnd(WorldLockManager &) const
    {
        return BlockUpdateIterator();
    }
    BlockChunkInvalidateCountType getInvalidateCount(WorldLockManager &lock_manager) const
    {
        updateLock(lock_manager);
        return getSubchunk().invalidateCount;
    }
};
}
}

#endif // BLOCK_ITERATOR_H_INCLUDED
