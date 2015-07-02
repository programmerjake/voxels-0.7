/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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

namespace programmerjake
{
namespace voxels
{
class World;

struct WorldLockManager final
{
    friend class World;
    template <typename T>
    class LockManager final
    {
        T *the_lock;
#ifndef NDEBUG
        static const char *fast_tid()
        {
            static thread_local const char v = 0;
            return &v;
        }
        const char *my_tid = fast_tid();
        void verify_tid() const
        {
            assert(my_tid == fast_tid());
        }
#else
        void verify_tid() const
        {
        }
#endif
    public:
        LockManager()
            : the_lock(nullptr)
        {
        }
        LockManager(const LockManager &) = delete;
        const LockManager &operator =(const LockManager &) = delete;
        ~LockManager()
        {
            clear();
        }
        void clear()
        {
            verify_tid();
            if(the_lock != nullptr)
                the_lock->unlock();
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
            }
        }
        void adopt(T &new_lock)
        {
            verify_tid();
            assert(the_lock == nullptr);
            the_lock = &new_lock;
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
                auto rest = join_ranges(unit_range(new_lock), range<typename std::decay<U>::type>(restBegin, restEnd));
                lock_all(rest.begin(), rest.end());
                the_lock = &new_lock;
            }
        }
    };
    LockManager<generic_lock_wrapper> block_biome_lock;
    const bool needLock;
    TLS &tls;
private:
    WorldLockManager(bool needLock, TLS &tls)
        : block_biome_lock(),
        needLock(needLock),
        tls(tls)
    {
    }
public:
    explicit WorldLockManager(TLS &tls)
        : WorldLockManager(true, tls)
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
