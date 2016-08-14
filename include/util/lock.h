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
#ifndef LOCK_H_INCLUDED
#define LOCK_H_INCLUDED

#include <utility>
#include <type_traits>
#include <memory>
#include <cassert>
#include <set>
#include <unordered_map>
#include <exception>
#include <mutex>
#include "util/tls.h"

namespace programmerjake
{
namespace voxels
{
template <typename T>
void lock_all(T begin, T end)
{
    auto start_lock = begin;
    if(begin == end) // lock nothing
        return;
retry:
    start_lock->lock();
    for(auto i = begin; i != end; i++)
    {
        if(start_lock == i)
            continue;
        try
        {
            if(!i->try_lock())
            {
                for(auto j = begin; j != i; j++)
                {
                    if(start_lock == j)
                        continue;
                    j->unlock();
                }
                start_lock->unlock();
                start_lock = i;
                goto retry;
            }
        }
        catch(...)
        {
            for(auto j = begin; j != i; j++)
            {
                if(start_lock == j)
                    continue;
                j->unlock();
            }
            start_lock->unlock();
            throw;
        }
    }
}

template <typename T>
bool try_lock_all(T begin, T end)
{
    for(auto i = begin; i != end; i++)
    {
        try
        {
            if(!i->try_lock())
            {
                for(auto j = begin; j != i; j++)
                {
                    j->unlock();
                }
                return false;
            }
        }
        catch(...)
        {
            for(auto j = begin; j != i; j++)
            {
                j->unlock();
            }
            throw;
        }
    }
    return true;
}

class generic_lock_wrapper final
{
private:
    void *pLock;
    void (*lockFn)(void *pLock);
    bool (*try_lockFn)(void *pLock);
    void (*unlockFn)(void *pLock);

public:
    template <typename T>
    explicit generic_lock_wrapper(T &theLock)
        : pLock(static_cast<void *>(std::addressof(theLock))), lockFn(), try_lockFn(), unlockFn()
    {
        lockFn = [](void *pLock) -> void
        {
            static_cast<T *>(pLock)->lock();
        };
        try_lockFn = [](void *pLock) -> bool
        {
            return static_cast<T *>(pLock)->try_lock();
        };
        unlockFn = [](void *pLock) -> void
        {
            static_cast<T *>(pLock)->unlock();
        };
    }
    void lock()
    {
        lockFn(pLock);
    }
    bool try_lock()
    {
        return try_lockFn(pLock);
    }
    void unlock()
    {
        unlockFn(pLock);
    }
};

class lock_hierarchy final
{
private:
    struct variables_t final
    {
        std::multiset<std::size_t> lock_levels;
        std::unordered_map<const void *, std::size_t> locks;
        variables_t() : lock_levels(), locks()
        {
        }
    };
    static variables_t &get_variables()
    {
        struct retvalTag
        {
        };
        thread_local_variable<variables_t, retvalTag> retval(TLS::getSlow());
        return retval.get();
    }
    static void handleError(const char *what, std::size_t lock_level, const void *theLock);

public:
    static void check_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        if(variables.locks[theLock] != 0)
        {
            handleError("check_lock: already locked", lock_level, theLock);
        }
        auto iter = variables.lock_levels.lower_bound(lock_level);
        if(iter != variables.lock_levels.end())
        {
            handleError("check_lock: hierarchy violation", lock_level, theLock);
        }
    }
    static void check_recursive_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        if(variables.locks[theLock] != 0)
        {
            return; // already locked
        }
        auto iter = variables.lock_levels.lower_bound(lock_level);
        if(iter != variables.lock_levels.end())
        {
            handleError("check_recursive_lock: hierarchy violation", lock_level, theLock);
        }
    }
    static void add_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        if(variables.locks[theLock]++ == 0)
            variables.lock_levels.insert(lock_level);
    }
    static void remove_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        auto iter = variables.lock_levels.find(lock_level);
        if(iter == variables.lock_levels.end())
        {
            handleError("remove_lock: unlock of unlocked lock", lock_level, theLock);
        }
        if(--variables.locks[theLock] == 0)
        {
            variables.lock_levels.erase(iter);
        }
    }
};

template <std::size_t level>
class checked_lock final
{
    std::mutex theLock;

public:
    checked_lock() : theLock()
    {
    }
    static constexpr std::size_t lock_level = level;
#ifdef DEBUG_LOCKS
    static constexpr bool DoCheck = true;
#else
    static constexpr bool DoCheck = false;
#endif
    void lock()
    {
        if(DoCheck)
        {
            lock_hierarchy::check_lock(lock_level, this);
            theLock.lock();
            lock_hierarchy::add_lock(lock_level, this);
        }
        else
            theLock.lock();
    }
    bool try_lock()
    {
        if(DoCheck)
        {
            if(theLock.try_lock())
            {
                lock_hierarchy::add_lock(lock_level, this);
                return true;
            }
            return false;
        }
        else
            return theLock.try_lock();
    }
    void unlock()
    {
        if(DoCheck)
        {
            theLock.unlock();
            lock_hierarchy::remove_lock(lock_level, this);
        }
        else
            theLock.unlock();
    }
};

template <std::size_t level>
class checked_recursive_lock final
{
    std::recursive_mutex theLock;

public:
    checked_recursive_lock() : theLock()
    {
    }
    static constexpr std::size_t lock_level = level;
#ifdef DEBUG_LOCKS
    static constexpr bool DoCheck = true;
#else
    static constexpr bool DoCheck = false;
#endif
    void lock()
    {
        if(DoCheck)
        {
            lock_hierarchy::check_recursive_lock(lock_level, this);
            theLock.lock();
            lock_hierarchy::add_lock(lock_level, this);
        }
        else
            theLock.lock();
    }
    bool try_lock()
    {
        if(DoCheck)
        {
            if(theLock.try_lock())
            {
                lock_hierarchy::add_lock(lock_level, this);
                return true;
            }
            return false;
        }
        else
            return theLock.try_lock();
    }
    void unlock()
    {
        if(DoCheck)
        {
            theLock.unlock();
            lock_hierarchy::remove_lock(lock_level, this);
        }
        else
            theLock.unlock();
    }
};
}
}

#endif // LOCK_H_INCLUDED
