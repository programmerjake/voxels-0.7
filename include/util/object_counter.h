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
#ifndef OBJECT_COUNTER_H_INCLUDED
#define OBJECT_COUNTER_H_INCLUDED

#include <atomic>
#include "util/logging.h"
#include "util/string_cast.h"

namespace programmerjake
{
namespace voxels
{
template <typename T, size_t N>
class ObjectCounter final
{
private:
    static std::atomic_size_t theCount;
    static void inc()
    {
        theCount.fetch_add(1, std::memory_order_relaxed);
    }
    static void dec()
    {
        theCount.fetch_sub(1, std::memory_order_relaxed);
    }

public:
    ObjectCounter()
    {
        inc();
    }
    ~ObjectCounter()
    {
        dec();
    }
    static std::size_t count()
    {
        return theCount.load(std::memory_order_relaxed);
    }
    static void dumpCount()
    {
        getDebugLog() << L"Object Count: " << count() << L" "
                      << string_cast<std::wstring>(__PRETTY_FUNCTION__) << postnl;
    }
    ObjectCounter(const ObjectCounter &)
    {
        inc();
    }
    ObjectCounter(ObjectCounter &&)
    {
        inc();
    }
    ObjectCounter &operator=(const ObjectCounter &)
    {
        return *this;
    }
    ObjectCounter &operator=(ObjectCounter &&)
    {
        return *this;
    }
};

template <typename T, size_t N>
std::atomic_size_t ObjectCounter<T, N>::theCount(0);
}
}

#endif // OBJECT_COUNTER_H_INCLUDED
