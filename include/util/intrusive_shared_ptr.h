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

#ifndef UTIL_INTRUSIVE_SHARED_PTR_H_INCLUDED
#define UTIL_INTRUSIVE_SHARED_PTR_H_INCLUDED

#include <atomic>
#include <utility>
#include <algorithm>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
template <typename T>
class IntrusiveSharedPtr;
class IntrusiveSharedPtrBase
{
    template <typename T>
    friend class IntrusiveSharedPtr;
    IntrusiveSharedPtrBase(const IntrusiveSharedPtrBase &) = delete;
    IntrusiveSharedPtrBase &operator=(const IntrusiveSharedPtrBase &) = delete;

private:
    std::atomic_size_t referenceCount;

public:
    IntrusiveSharedPtrBase() : referenceCount(0)
    {
    }
    virtual ~IntrusiveSharedPtrBase() = default;
    typedef void (*SharedPtrDeleterFunction)(IntrusiveSharedPtrBase *pointer);
    virtual SharedPtrDeleterFunction getSharedPtrDeleter() const;
};

template <typename T>
class IntrusiveSharedPtr final
{
private:
    IntrusiveSharedPtrBase *countedPointer;
    T *visiblePointer;

public:
    constexpr IntrusiveSharedPtr(std::nullptr_t = nullptr)
        : countedPointer(nullptr), visiblePointer(nullptr)
    {
    }
    IntrusiveSharedPtr(const IntrusiveSharedPtr &rt)
        : countedPointer(rt.countedPointer), visiblePointer(rt.visiblePointer)
    {
        if(countedPointer)
            countedPointer->referenceCount++;
    }
    IntrusiveSharedPtr(IntrusiveSharedPtr &&rt)
        : countedPointer(rt.countedPointer), visiblePointer(rt.visiblePointer)
    {
        rt.countedPointer = nullptr;
        rt.visiblePointer = nullptr;
    }
    ~IntrusiveSharedPtr()
    {
        if(countedPointer)
        {
            if(--countedPointer->referenceCount == 0)
            {
                IntrusiveSharedPtrBase::SharedPtrDeleterFunction deleter = countedPointer->getSharedPtrDeleter();
                if(deleter)
                    deleter(countedPointer);
            }
        }
    }
    friend void swap(IntrusiveSharedPtr &l, IntrusiveSharedPtr &r)
    {
        using std::swap;
        swap(l.countedPointer, r.countedPointer);
        swap(l.visiblePointer, r.visiblePointer);
    }
    IntrusiveSharedPtr &operator=(IntrusiveSharedPtr rt)
    {
        swap(*this, rt);
        return *this;
    }
    constexpr T *get() const
    {
        return visiblePointer;
    }
    T &operator *() const
    {
        assert(visiblePointer != nullptr);
        return *visiblePointer;
    }
    T *operator ->() const
    {
        assert(visiblePointer != nullptr);
        return *visiblePointer;
    }
    template <typename U>
    U &operator ->*(U T::*memberPointer) const
    {
        assert(visiblePointer != nullptr);
        assert(memberPointer != nullptr);
        return visiblePointer->*memberPointer;
    }
#ifndef _MSC_VER
    explicit
#endif
    constexpr operator bool() const
    {
        return visiblePointer != nullptr;
    }
    constexpr bool operator !() const
    {
        return visiblePointer == nullptr;
    }
    friend constexpr bool operator ==(std::nullptr_t, const IntrusiveSharedPtr &v)
    {
        return !v;
    }
    friend constexpr bool operator ==(const IntrusiveSharedPtr &v, std::nullptr_t)
    {
        return !v;
    }
    friend constexpr bool operator !=(std::nullptr_t, const IntrusiveSharedPtr &v)
    {
        return static_cast<bool>(v);
    }
    friend constexpr bool operator !=(const IntrusiveSharedPtr &v, std::nullptr_t)
    {
        return static_cast<bool>(v);
    }
    template <typename U>
    friend constexpr bool operator ==(const IntrusiveSharedPtr &a, const IntrusiveSharedPtr<U> &b)
    {
        return a.get == b.get();
    }
    template <typename U>
    friend constexpr bool operator !=(const IntrusiveSharedPtr &a, const IntrusiveSharedPtr<U> &b)
    {
        return a.get != b.get();
    }
};

IntrusiveSharedPtrBase::SharedPtrDeleterFunction IntrusiveSharedPtrBase::getSharedPtrDeleter() const
{
    return [](IntrusiveSharedPtrBase *pointer)
    {
        delete pointer;
    };
}

template <typename T, typename... Args>
IntrusiveSharedPtr<T> makeIntrusiveShared(Args &&... args)
{
}
}
}

#endif /* UTIL_INTRUSIVE_SHARED_PTR_H_INCLUDED */
