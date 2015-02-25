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
#ifndef LOCK_H_INCLUDED
#define LOCK_H_INCLUDED

#include <utility>
#include <type_traits>

namespace programmerjake
{
namespace voxels
{
template <typename T>
void lock_all(T begin, T end)
{
    struct failure_t
    {
    };
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
            try
            {
                if(!i->try_lock())
                    throw failure_t();
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
        catch(failure_t &)
        {
            start_lock = i;
            goto retry;
        }
    }
}
}
}

#endif // LOCK_H_INCLUDED
