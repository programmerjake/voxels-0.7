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

#ifndef UTIL_SEMAPHORE_H_
#define UTIL_SEMAPHORE_H_

#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <type_traits>

namespace programmerjake
{
namespace voxels
{

template <typename CountType>
class GenericSemaphore final
{
    GenericSemaphore(const GenericSemaphore &) = delete;
    GenericSemaphore &operator=(const GenericSemaphore &) = delete;

public:
    static_assert(std::is_integral<CountType>::value && std::is_signed<CountType>::value,
                  "invalid CountType; must be a signed integer");
    typedef CountType count_type;

private:
    count_type count;
    std::mutex stateLock;
    std::condition_variable stateCond;

public:
    explicit GenericSemaphore(count_type count = 0) : count(count), stateLock(), stateCond()
    {
    }
    bool try_lock(count_type lockCount = 1)
    {
        assert(lockCount >= 0);
        std::unique_lock<std::mutex> lockedState(stateLock, std::try_to_lock);
        if(!lockedState.owns_lock())
            return false;
        if(count < lockCount)
            return false;
        count -= lockCount;
        return true;
    }
    void lock(count_type lockCount = 1)
    {
        assert(lockCount >= 0);
        std::unique_lock<std::mutex> lockedState(stateLock);
        while(count < lockCount)
        {
            stateCond.wait(lockedState);
        }
        count -= lockCount;
    }
    void unlock(count_type unlockCount = 1)
    {
        assert(unlockCount >= 0);
        std::unique_lock<std::mutex> lockedState(stateLock);
        if(count <= 0)
        {
            count += unlockCount;
            stateCond.notify_all();
        }
        else
        {
            count += unlockCount;
        }
    }
};

typedef GenericSemaphore<std::int_fast32_t> Semaphore;

}
}

#endif /* UTIL_SEMAPHORE_H_ */
