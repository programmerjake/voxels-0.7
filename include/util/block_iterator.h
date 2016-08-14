/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
#include "util/tls.h"

namespace programmerjake
{
namespace voxels
{
class World;
class ViewPoint;
class BlockIterator final
{
    friend class World;
    friend class ViewPoint;
    std::shared_ptr<BlockChunk> chunk;
    BlockChunkMap *chunks;
    PositionI currentBasePosition;
    VectorI currentRelativePosition;
    void getChunk(TLS &tls, WorldLockManager *plock_manager)
    {
        if(plock_manager)
            plock_manager->clear();
        chunk = (*chunks)[currentBasePosition].getOrLoad(tls);
    }
    BlockChunkSubchunk &getSubchunk() const
    {
        VectorI currentSubchunkIndex =
            BlockChunk::getSubchunkIndexFromChunkRelativePosition(currentRelativePosition);
        return chunk
            ->subchunks[currentSubchunkIndex.x][currentSubchunkIndex.y][currentSubchunkIndex.z];
    }
    BlockChunkSubchunk &getBiomeSubchunk() const
    {
        VectorI currentSubchunkIndex =
            BlockChunk::getSubchunkIndexFromChunkRelativePosition(currentRelativePosition);
        return chunk->subchunks[currentSubchunkIndex.x][0][currentSubchunkIndex.z];
    }
    void updateLock(WorldLockManager &lock_manager) const
    {
        if(lock_manager.needLock)
            lock_manager.block_biome_lock.set(getSubchunk().lock);
    }
    template <typename T>
    void updateLock(WorldLockManager &lock_manager, T beginV, T endV) const
    {
        if(lock_manager.needLock)
            lock_manager.block_biome_lock.set(getSubchunk().lock, beginV, endV);
        else
            lock_all(beginV, endV);
    }
    bool tryUpdateLock(WorldLockManager &lock_manager) const
    {
        if(lock_manager.needLock)
            return lock_manager.block_biome_lock.try_set(getSubchunk().lock);
        return true;
    }
    BlockIterator(std::shared_ptr<BlockChunk> chunk,
                  BlockChunkMap *chunks,
                  PositionI currentBasePosition,
                  VectorI currentRelativePosition)
        : chunk(chunk),
          chunks(chunks),
          currentBasePosition(currentBasePosition),
          currentRelativePosition(currentRelativePosition)
    {
    }

public:
    BlockIterator(BlockChunkMap *chunks, PositionI position, TLS &tls)
        : chunk(),
          chunks(chunks),
          currentBasePosition(BlockChunk::getChunkBasePosition(position)),
          currentRelativePosition(BlockChunk::getChunkRelativePosition(position))
    {
        assert(chunks != nullptr);
        getChunk(tls, nullptr);
    }
    BlockIterator(const BlockIterator &) = default;
    BlockIterator(BlockIterator &&) = default;
    BlockIterator &operator=(const BlockIterator &) = default;
    BlockIterator &operator=(BlockIterator &&) = default;
    PositionI position() const
    {
        return currentBasePosition + currentRelativePosition;
    }
    void moveTowardNX(WorldLockManager &lock_manager)
    {
        if(currentRelativePosition.x <= 0)
        {
            currentRelativePosition.x = BlockChunk::chunkSizeX - 1;
            currentBasePosition.x -= BlockChunk::chunkSizeX;
            getChunk(lock_manager.tls, &lock_manager);
        }
        else
            currentRelativePosition.x--;
    }
    void moveTowardNY(WorldLockManager &lock_manager)
    {
        if(currentRelativePosition.y <= 0)
        {
            currentRelativePosition.y = BlockChunk::chunkSizeY - 1;
            currentBasePosition.y -= BlockChunk::chunkSizeY;
            getChunk(lock_manager.tls, &lock_manager);
        }
        else
            currentRelativePosition.y--;
    }
    void moveTowardNZ(WorldLockManager &lock_manager)
    {
        if(currentRelativePosition.z <= 0)
        {
            currentRelativePosition.z = BlockChunk::chunkSizeZ - 1;
            currentBasePosition.z -= BlockChunk::chunkSizeZ;
            getChunk(lock_manager.tls, &lock_manager);
        }
        else
            currentRelativePosition.z--;
    }
    void moveTowardPX(WorldLockManager &lock_manager)
    {
        if(currentRelativePosition.x >= BlockChunk::chunkSizeX - 1)
        {
            currentRelativePosition.x = 0;
            currentBasePosition.x += BlockChunk::chunkSizeX;
            getChunk(lock_manager.tls, &lock_manager);
        }
        else
            currentRelativePosition.x++;
    }
    void moveTowardPY(WorldLockManager &lock_manager)
    {
        if(currentRelativePosition.y >= BlockChunk::chunkSizeY - 1)
        {
            currentRelativePosition.y = 0;
            currentBasePosition.y += BlockChunk::chunkSizeY;
            getChunk(lock_manager.tls, &lock_manager);
        }
        else
            currentRelativePosition.y++;
    }
    void moveTowardPZ(WorldLockManager &lock_manager)
    {
        if(currentRelativePosition.z >= BlockChunk::chunkSizeZ - 1)
        {
            currentRelativePosition.z = 0;
            currentBasePosition.z += BlockChunk::chunkSizeZ;
            getChunk(lock_manager.tls, &lock_manager);
        }
        else
            currentRelativePosition.z++;
    }
    void moveToward(BlockFace bf, WorldLockManager &lock_manager)
    {
        switch(bf)
        {
        case BlockFace::NX:
            moveTowardNX(lock_manager);
            break;
        case BlockFace::PX:
            moveTowardPX(lock_manager);
            break;
        case BlockFace::NY:
            moveTowardNY(lock_manager);
            break;
        case BlockFace::PY:
            moveTowardPY(lock_manager);
            break;
        case BlockFace::NZ:
            moveTowardNZ(lock_manager);
            break;
        case BlockFace::PZ:
            moveTowardPZ(lock_manager);
            break;
        }
    }
    void moveFrom(BlockFace bf, WorldLockManager &lock_manager)
    {
        switch(bf)
        {
        case BlockFace::NX:
            moveTowardPX(lock_manager);
            break;
        case BlockFace::PX:
            moveTowardNX(lock_manager);
            break;
        case BlockFace::NY:
            moveTowardPY(lock_manager);
            break;
        case BlockFace::PY:
            moveTowardNY(lock_manager);
            break;
        case BlockFace::NZ:
            moveTowardPZ(lock_manager);
            break;
        case BlockFace::PZ:
            moveTowardNZ(lock_manager);
            break;
        }
    }
    void moveBy(VectorI delta, WorldLockManager &lock_manager)
    {
        currentRelativePosition += delta;
        VectorI deltaBasePosition = BlockChunk::getChunkBasePosition(currentRelativePosition);
        if(deltaBasePosition != VectorI(0))
        {
            currentBasePosition += deltaBasePosition;
            currentRelativePosition -= deltaBasePosition;
            getChunk(lock_manager.tls, &lock_manager);
        }
    }
    void moveTo(VectorI pos, WorldLockManager &lock_manager)
    {
        moveBy(pos - static_cast<VectorI>(position()), lock_manager);
    }
    void moveTo(PositionI pos, WorldLockManager &lock_manager)
    {
        if(pos.d == currentBasePosition.d)
        {
            moveTo(static_cast<VectorI>(pos), lock_manager);
        }
        else
        {
            currentBasePosition = BlockChunk::getChunkBasePosition(pos);
            currentRelativePosition = BlockChunk::getChunkRelativePosition(pos);
            getChunk(lock_manager.tls, &lock_manager);
        }
    }

private:
    BlockChunkBlock &getBlock() const
    {
        return chunk->blocks[currentRelativePosition.x][currentRelativePosition
                                                            .y][currentRelativePosition.z];
    }
    BlockChunkBlock &getBlock(WorldLockManager &lock_manager) const
    {
        updateLock(lock_manager);
        return chunk->blocks[currentRelativePosition.x][currentRelativePosition
                                                            .y][currentRelativePosition.z];
    }
    BlockChunkBiome &getBiome() const
    {
        return chunk->biomes[currentRelativePosition.x][currentRelativePosition.z];
    }

public:
    Block get(WorldLockManager &lock_manager) const
    {
        BlockChunkBlock &blockChunkBlock = getBlock(lock_manager);
        return BlockChunk::getBlockFromArray(
            BlockChunk::getSubchunkRelativePosition(currentRelativePosition),
            blockChunkBlock,
            getSubchunk());
    }
    const BiomeProperties &getBiomeProperties(WorldLockManager &lock_manager) const
    {
        updateLock(lock_manager);
        return getBiome().biomeProperties;
    }
    BlockUpdateIterator updatesBegin(WorldLockManager &lock_manager) const
    {
        updateLock(lock_manager);
        return BlockUpdateIterator(getSubchunk().blockOptionalData.getUpdateListHead(
            BlockChunk::getSubchunkRelativePosition(currentRelativePosition)));
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
