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
#ifndef CACHED_VARIABLE_H_INCLUDED
#define CACHED_VARIABLE_H_INCLUDED

#include <atomic>
#include <cstdint>
#include "util/spin_lock.h"
#include <mutex> // for lock_guard

namespace programmerjake
{
namespace voxels
{
template <typename T>
class CachedVariable
{
    mutable simple_spin_lock theLock;
    typedef std::lock_guard<simple_spin_lock> my_lock_guard;
    T view;
public:
    CachedVariable()
        : view()
    {
    }
    explicit CachedVariable(const T &v)
        : view(v)
    {
    }
    explicit CachedVariable(T &&v)
        : view(std::move(v))
    {
    }
    CachedVariable(const CachedVariable &rt)
        : CachedVariable(rt.read())
    {
    }
    T read() const
    {
        T retval;
        {
            my_lock_guard lockIt(theLock);
            retval = view;
        }
        return std::move(retval);
    }
    void write(T value)
    {
        my_lock_guard lockIt(theLock);
        view = std::move(value);
    }
    operator T() const
    {
        return read();
    }
    const CachedVariable &operator =(const T &value)
    {
        write(value);
        return *this;
    }
    const CachedVariable &operator =(const CachedVariable &rt)
    {
        write(rt.read());
        return *this;
    }
    const CachedVariable &operator =(T &&value)
    {
        write(std::move(value));
        return *this;
    }
    class LockedView final
    {
        friend class CachedVariable;
        T *v;
        std::unique_lock<simple_spin_lock> lockIt;
        explicit LockedView(CachedVariable *cv)
            : lockIt(cv->theLock)
        {
            v = &cv->view;
        }
        LockedView(const LockedView &) = delete;
    public:
        ~LockedView()
        {
        }
        LockedView(LockedView &&rt)
            : v(rt.v), lockIt(std::move(rt.lockIt))
        {
            rt.v = nullptr;
        }
        const T &operator =(const LockedView &rt)
        {
            return *v = *rt.v;
        }
        const T &operator =(const T &value)
        {
            return *v = value;
        }
        const T &operator =(T &&value)
        {
            return *v = std::move(value);
        }
        void write(const T &value)
        {
            *v = value;
        }
        void write(T &&value)
        {
            *v = std::move(value);
        }
        const T &read()
        {
            return *v;
        }
        T &get()
        {
            return *v;
        }
        operator T &()
        {
            return *v;
        }
        T &operator ->()
        {
            return *v;
        }
        bool valid() const
        {
            return v != nullptr;
        }
        void release()
        {
            v = nullptr;
            lockIt.unlock();
        }
    };
    LockedView lock()
    {
        return LockedView(this);
    }
};
}
}

#endif // CACHED_VARIABLE_H_INCLUDED
