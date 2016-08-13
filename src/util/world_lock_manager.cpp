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

#include "util/world_lock_manager.h"
#include "util/logging.h"
#include "util/block_chunk.h"
#include "world/world.h"

namespace programmerjake
{
namespace voxels
{
void WorldLock::unlock(WorldLockManager &lock_manager) noexcept
{
    assert(lock_manager.lockedBlockChunkList);
    delete lock_manager.lockedBlockChunkList;
    lock_manager.lockedBlockChunkList = nullptr;
}

WorldLock::WorldLock(WorldLockManager &lock_manager,
                     World &world,
                     const PositionI *positions,
                     std::size_t positionCount)
    : WorldLock(lock_manager, world.physicsWorld->chunks, positions, positionCount)
{
}

WorldLock::WorldLock(WorldLockManager &lock_manager,
                     BlockChunks &blockChunks,
                     const PositionI *positions,
                     std::size_t positionCount)
    : lock_manager(&lock_manager)
{
    assert(!lock_manager.lockedBlockChunkList);
    lock_manager.lockedBlockChunkList = new LockedIndirectBlockChunkList(
        blockChunks.lockChunks<true>(positions, positionCount, lock_manager.tls));
}
}
}
