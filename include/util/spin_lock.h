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
#ifndef SPIN_LOCK_H_INCLUDED
#define SPIN_LOCK_H_INCLUDED

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "util/cpu_relax.h"

namespace programmerjake
{
namespace voxels
{
struct simple_spin_lock final
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    bool try_lock()
    {
        if(flag.test_and_set(std::memory_order_acquire))
            return false;
        return true;
    }
    void lock()
    {
        while(flag.test_and_set(std::memory_order_acquire))
        {
            cpu_relax();
        }
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

struct blocking_spin_lock final
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    std::atomic_ulong waitingCount;
    std::condition_variable cond;
    std::mutex lockMutex;
    blocking_spin_lock()
        : waitingCount(0), cond(), lockMutex()
    {
    }
    bool try_lock()
    {
        if(flag.test_and_set(std::memory_order_acquire))
            return false;
        return true;
    }
    void lock()
    {
        size_t count = 0;
        constexpr size_t skipCount = 1000;
        waitingCount++;
        while(flag.test_and_set(std::memory_order_acquire))
        {
            if(count++ >= skipCount)
            {
                std::unique_lock<std::mutex> lockIt(lockMutex);
                cond.wait(lockIt);
                count = skipCount;
            }
            cpu_relax();
        }
        waitingCount--;
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
        if(waitingCount > 0)
            cond.notify_one();
    }
};
}
}

#endif // SPIN_LOCK_H_INCLUDED
