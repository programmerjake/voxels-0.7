/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef UNLOCK_GUARD_H_INCLUDED
#define UNLOCK_GUARD_H_INCLUDED

#include <mutex>

namespace programmerjake
{
namespace voxels
{
template <typename T>
class unlock_guard
{
    T *plock;
    unlock_guard(const unlock_guard &) = delete;
    const unlock_guard &operator=(const unlock_guard &) = delete;

public:
    explicit unlock_guard(T &lock) : plock(&lock)
    {
        plock->unlock();
    }
    unlock_guard(T &lock, std::adopt_lock_t) : plock(&lock)
    {
    }
    unlock_guard(unlock_guard &&rt) : plock(rt.plock)
    {
        rt.plock = nullptr;
    }
    ~unlock_guard()
    {
        if(plock != nullptr)
            plock->lock();
    }
};
}
}

#endif // UNLOCK_GUARD_H_INCLUDED
