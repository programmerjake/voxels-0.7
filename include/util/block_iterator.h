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
    BlockChunk *chunk;
    PositionI currentBasePosition;
    VectorI currentRelativePosition;
    void getChunk(WorldLockManager &lock_manager) noexcept
    {
        assert(lock_manager.lockedBlockChunkList);
        chunk = lock_manager.lockedBlockChunkList->getBlockChunks().at(currentBasePosition);
    }

public:
    BlockIterator(PositionI position, WorldLockManager &lock_manager) noexcept
        : chunk(),
          currentBasePosition(BlockChunk::getChunkBasePosition(position)),
          currentRelativePosition(BlockChunk::getChunkRelativePosition(position))
    {
        getChunk(lock_manager);
    }
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
            getChunk(lock_manager);
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
            getChunk(lock_manager);
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
            getChunk(lock_manager);
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
            getChunk(lock_manager);
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
            getChunk(lock_manager);
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
            getChunk(lock_manager);
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
            getChunk(lock_manager);
        }
    }

private:
    BlockChunkBlock &getBlock() const noexcept
    {
        return chunk->blocks[currentRelativePosition.x][currentRelativePosition
                                                            .y][currentRelativePosition.z];
    }
    BlockChunkColumn &getColumn() const noexcept
    {
        return chunk->columns[currentRelativePosition.x][currentRelativePosition.z];
    }

public:
    Block get(WorldLockManager &lock_manager) const noexcept
    {
        return getBlock().getBlock();
    }
    const BiomeProperties &getBiomeProperties(WorldLockManager &lock_manager) const noexcept
    {
        return getColumn().biomeProperties;
    }
    BlockUpdateIterator updatesBegin(WorldLockManager &lock_manager) const noexcept
    {
        auto &block = getBlock();
        if(!block.extraData)
            return BlockUpdateIterator();
        return BlockUpdateIterator(block.extraData->blockUpdates.begin(),
                                   block.extraData.get(),
                                   chunk->blockUpdateListHead);
    }
    BlockUpdateIterator updatesEnd(WorldLockManager &lock_manager) const noexcept
    {
        auto &block = getBlock();
        if(!block.extraData)
            return BlockUpdateIterator();
        return BlockUpdateIterator(
            block.extraData->blockUpdates.end(), block.extraData.get(), chunk->blockUpdateListHead);
    }
    class Updates final
    {
        friend class BlockIterator;
        Updates(const Updates &) = delete;
        Updates &operator=(const Updates &) = delete;

    private:
        const BlockIterator *blockIterator;
        WorldLockManager &lock_manager;
        constexpr Updates(const BlockIterator *blockIterator, WorldLockManager &lock_manager)
            : blockIterator(blockIterator), lock_manager(lock_manager)
        {
        }

    public:
        Updates(Updates &&) = default;
        typedef BlockUpdateIterator iterator;
        BlockUpdateIterator begin() const noexcept
        {
            return blockIterator->updatesBegin(lock_manager);
        }
        BlockUpdateIterator end() const noexcept
        {
            return blockIterator->updatesBegin(lock_manager);
        }
        bool empty() const noexcept
        {
            return begin() == end();
        }
    };
    Updates updates(WorldLockManager &lock_manager) const noexcept
    {
        return Updates(this, lock_manager);
    }
};
}
}

#endif // BLOCK_ITERATOR_H_INCLUDED
