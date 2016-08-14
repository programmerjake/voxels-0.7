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

#include "util/lock.h"
#include "util/iterator.h"
#include "util/tls.h"
#include <thread>
#include <chrono>

namespace programmerjake
{
namespace voxels
{
class World;

struct WorldLockManager final
{
    static void handleLockedForTooLong(std::chrono::high_resolution_clock::duration lockedDuration);
    friend class World;
    template <typename T>
    class LockManager final
    {
    private:
        T *the_lock;
#ifdef DEBUG_CHUNK_LOCK_TIME
        std::chrono::high_resolution_clock::time_point lockTime;
#endif
#ifndef NDEBUG
        std::thread::id my_tid = std::this_thread::get_id();
        void verify_tid() const
        {
            assert(my_tid == std::this_thread::get_id());
        }
#else
        void verify_tid() const
        {
        }
#endif
        const std::chrono::duration<float> lockTimeLimit;

    public:
        explicit LockManager(float lockTimeLimit) : the_lock(nullptr), lockTimeLimit(lockTimeLimit)
        {
        }
        LockManager(const LockManager &) = delete;
        const LockManager &operator=(const LockManager &) = delete;
        ~LockManager()
        {
            clear();
        }
        void clear()
        {
            verify_tid();
            if(the_lock != nullptr)
            {
#ifdef DEBUG_CHUNK_LOCK_TIME
                auto unlockTime = std::chrono::high_resolution_clock::now();
#endif
                the_lock->unlock();
#ifdef DEBUG_CHUNK_LOCK_TIME
                auto lockedDuration = unlockTime - lockTime;
                if(lockedDuration > lockTimeLimit)
                {
                    handleLockedForTooLong(lockedDuration);
                }
#endif
            }
            the_lock = nullptr;
        }
        void set(T &new_lock)
        {
            verify_tid();
            if(the_lock != &new_lock)
            {
                clear();
                new_lock.lock();
                the_lock = &new_lock;
#ifdef DEBUG_CHUNK_LOCK_TIME
                lockTime = std::chrono::high_resolution_clock::now();
#endif
            }
        }
        void adopt(T &new_lock)
        {
            verify_tid();
            assert(the_lock == nullptr);
            the_lock = &new_lock;
#ifdef DEBUG_CHUNK_LOCK_TIME
            lockTime = std::chrono::high_resolution_clock::now();
#endif
        }
        bool try_set(T &new_lock)
        {
            verify_tid();
            if(the_lock != &new_lock)
            {
                clear();
                if(!new_lock.try_lock())
                    return false;
                the_lock = &new_lock;
#ifdef DEBUG_CHUNK_LOCK_TIME
                lockTime = std::chrono::high_resolution_clock::now();
#endif
            }
            return true;
        }
        template <typename U>
        void set(T &new_lock, U restBegin, U restEnd)
        {
            verify_tid();
            if(the_lock != &new_lock)
            {
                clear();
                auto rest = join_ranges(unit_range(new_lock),
                                        range<typename std::decay<U>::type>(restBegin, restEnd));
                lock_all(rest.begin(), rest.end());
                the_lock = &new_lock;
#ifdef DEBUG_CHUNK_LOCK_TIME
                lockTime = std::chrono::high_resolution_clock::now();
#endif
            }
        }
    };
    LockManager<generic_lock_wrapper> block_biome_lock;
    const bool needLock;
    TLS &tls;

private:
    WorldLockManager(bool needLock, TLS &tls, float lockTimeLimit = 0.05f)
        : block_biome_lock(lockTimeLimit), needLock(needLock), tls(tls)
    {
    }

public:
    explicit WorldLockManager(TLS &tls) : WorldLockManager(true, tls)
    {
    }
    WorldLockManager(TLS &tls, float lockTimeLimit) : WorldLockManager(true, tls, lockTimeLimit)
    {
    }
    void clear()
    {
        block_biome_lock.clear();
    }
};
}
}

#endif // WORLD_LOCK_MANAGER_H_INCLUDED
