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
#ifndef ORDERED_WEAK_PTR_H_INCLUDED
#define ORDERED_WEAK_PTR_H_INCLUDED

#include <memory>
#include <cstdint>
#include <algorithm>

namespace programmerjake
{
namespace voxels
{

template <typename T>
class ordered_weak_ptr final
{
    template <typename U>
    friend class ordered_weak_ptr;
private:
    std::weak_ptr<T> v;
    std::intptr_t compareValue;
    template <typename U>
    static std::intptr_t getCompareValue(const std::weak_ptr<U> &v)
    {
        return (std::intptr_t)v.lock().get();
    }
    template <typename U>
    static std::intptr_t getCompareValue(const std::shared_ptr<U> &v)
    {
        return (std::intptr_t)v.get();
    }
public:
    constexpr ordered_weak_ptr()
        : v(), compareValue(0)
    {
    }
    template <typename U>
    ordered_weak_ptr(const std::weak_ptr<U> &r)
        : v(r), compareValue(getCompareValue(r))
    {
    }
    template <typename U>
    ordered_weak_ptr(const std::shared_ptr<U> &r)
        : v(r), compareValue(getCompareValue(r))
    {
    }
    template <typename U>
    ordered_weak_ptr(const ordered_weak_ptr<U> &r)
        : v(r.v), compareValue(r.compareValue)
    {
    }
    template <typename U>
    const ordered_weak_ptr &operator =(const ordered_weak_ptr<U> &r)
    {
        v = r.v;
        compareValue = r.compareValue;
        return *this;
    }
    template <typename U>
    const ordered_weak_ptr &operator =(const std::weak_ptr<U> &r)
    {
        v = r;
        compareValue = getCompareValue(r);
        return *this;
    }
    template <typename U>
    const ordered_weak_ptr &operator =(const std::shared_ptr<U> &r)
    {
        v = r;
        compareValue = getCompareValue(r);
        return *this;
    }
    void reset()
    {
        v.reset();
        compareValue = 0;
    }
    void swap(ordered_weak_ptr &r)
    {
        v.swap(r.v);
        using std::swap;
        swap(compareValue, r.compareValue);
    }
    long use_count() const
    {
        return v.use_count();
    }
    bool expired() const
    {
        return v.expired();
    }
    std::shared_ptr<T> lock() const
    {
        return v.lock();
    }
    template <typename U>
    constexpr bool operator ==(const ordered_weak_ptr<U> &r) const
    {
        return compareValue == r.compareValue;
    }
    template <typename U>
    constexpr bool operator !=(const ordered_weak_ptr<U> &r) const
    {
        return compareValue != r.compareValue;
    }
    template <typename U>
    constexpr bool operator <=(const ordered_weak_ptr<U> &r) const
    {
        return compareValue <= r.compareValue;
    }
    template <typename U>
    constexpr bool operator >=(const ordered_weak_ptr<U> &r) const
    {
        return compareValue >= r.compareValue;
    }
    template <typename U>
    constexpr bool operator <(const ordered_weak_ptr<U> &r) const
    {
        return compareValue < r.compareValue;
    }
    template <typename U>
    constexpr bool operator >(const ordered_weak_ptr<U> &r) const
    {
        return compareValue > r.compareValue;
    }
    std::size_t hash() const
    {
        return (std::size_t)compareValue;
    }
};

}
}

namespace std
{
template <typename T>
void swap(programmerjake::voxels::ordered_weak_ptr<T> &l, programmerjake::voxels::ordered_weak_ptr<T> &r)
{
    l.swap(r);
}

template <typename T>
struct hash<programmerjake::voxels::ordered_weak_ptr<T>> final
{
    size_t operator ()(const programmerjake::voxels::ordered_weak_ptr<T> &v) const
    {
        return v.hash();
    }
};
}

#endif // ORDERED_WEAK_PTR_H_INCLUDED
