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
#ifndef WORLD_LOCK_MANAGER_H_INCLUDED
#define WORLD_LOCK_MANAGER_H_INCLUDED

#include <cassert>
#include <utility>
#include "util/position.h"
#include "util/tls.h"
#include <vector>

namespace programmerjake
{
namespace voxels
{
class LockedIndirectBlockChunkList;
class World;
struct WorldLockManager final
{
    WorldLockManager(const WorldLockManager &) = delete;
    WorldLockManager &operator=(const WorldLockManager &) = delete;
    explicit WorldLockManager(TLS &tls) : tls(tls), lockedBlockChunkList(nullptr)
    {
    }
    TLS &tls;
    LockedIndirectBlockChunkList *lockedBlockChunkList;
    void clearAndCheckLocks() noexcept
    {
        assert(lockedBlockChunkList == nullptr);
    }
};

class BlockChunks;

class WorldLock final
{
private:
    WorldLockManager *lock_manager;
    static void unlock(WorldLockManager &lock_manager) noexcept;
    WorldLock(WorldLockManager &lock_manager,
              World &world,
              const PositionI *positions,
              std::size_t positionCount);
    WorldLock(WorldLockManager &lock_manager,
              BlockChunks &blockChunks,
              const PositionI *positions,
              std::size_t positionCount);
    struct PositionsTag final
    {
    };
    static std::vector<PositionI> &makePositionList(WorldLockManager &lock_manager)
    {
        thread_local_variable<std::vector<PositionI>, PositionsTag> positions(lock_manager.tls);
        positions.get().clear();
        return positions.get();
    }
    static void appendChunksToPositionList(std::vector<PositionI> &positions,
                                           PositionI minPosition,
                                           VectorI maxPosition);
    template <typename... Args>
    static std::vector<PositionI> &makePositionList(WorldLockManager &lock_manager,
                                                    Args &&... args,
                                                    PositionI minPosition,
                                                    VectorI maxPosition)
    {
        auto &positions = makePositionList(lock_manager, std::forward<Args>(args)...);
        appendChunksToPositionList(positions, minPosition, maxPosition);
        return positions;
    }

public:
    constexpr WorldLock() noexcept : lock_manager(nullptr)
    {
    }
    template <typename... Args>
    static WorldLock make(WorldLockManager &lock_manager, Args &&... args)
    {
        auto &positions = makePositionList(lock_manager, std::forward<Args>(args)...);
        return WorldLock(lock_manager, positions.data(), positions.size());
    }
    ~WorldLock()
    {
        if(lock_manager)
            unlock(*lock_manager);
    }
    WorldLock(WorldLock &&rt) noexcept : lock_manager(rt.lock_manager)
    {
        rt.lock_manager = nullptr;
    }
    void swap(WorldLock &other) noexcept
    {
        std::swap(lock_manager, other.lock_manager);
    }
    WorldLock &operator=(WorldLock rt) noexcept
    {
        swap(rt);
        return *this;
    }
};
}
}

#endif // WORLD_LOCK_MANAGER_H_INCLUDED
