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

#ifndef UTIL_SEMAPHORE_H_
#define UTIL_SEMAPHORE_H_

#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <type_traits>
#include <atomic>

namespace programmerjake
{
namespace voxels
{
class Semaphore final
{
    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;

private:
    std::atomic_size_t atomicCount;
    std::mutex stateLock;
    std::condition_variable stateCond;

public:
    explicit Semaphore(std::size_t count);
    std::size_t getCount();
    bool try_lock(std::size_t lockCount = 1);
    void lock(std::size_t lockCount = 1);
    void unlock(std::size_t unlockCount = 1);
};
}
}

#endif /* UTIL_SEMAPHORE_H_ */
