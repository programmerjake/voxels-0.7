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
#ifndef CHECKED_ARRAY_H_INCLUDED
#define CHECKED_ARRAY_H_INCLUDED

#include <initializer_list>
#include <cassert>
#include <iterator>
#include <algorithm>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
#ifndef DOXYGEN
template <typename T, std::size_t N>
struct checked_array_helper
{
    typedef T array_type[N];
    static constexpr T &deref(const array_type &a, std::size_t n) noexcept
    {
        return const_cast<T &>(a[n]);
    }
};

template <typename T>
struct checked_array_helper<T, 0>
{
    struct array_type
    {
    };
    static constexpr T &deref(const array_type &, std::size_t) noexcept
    {
        return *static_cast<T *>(nullptr);
    }
};
#endif

template <typename T, std::size_t N>
struct checked_array
{
#ifdef DOXYGEN
private:
    T m_data[N]; // public for initialization
public:
#else
private:
    typedef checked_array_helper<T, N> array_helper;
public:
    typename array_helper::array_type m_data; // public for initialization
#endif
    typedef T value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    constexpr const T &operator [](std::size_t pos) const
    {
        return constexpr_assert(pos < N), array_helper::deref(m_data, pos);
    }
    T &operator [](std::size_t pos)
    {
        assert(pos < N);
        return array_helper::deref(m_data, pos);
    }
    constexpr const T &at(std::size_t pos) const
    {
        return constexpr_assert(pos < N), array_helper::deref(m_data, pos);
    }
    T &at(std::size_t pos)
    {
        assert(pos < N);
        return array_helper::deref(m_data, pos);
    }
    T &front()
    {
        return array_helper::deref(m_data, 0);
    }
    constexpr const T &front() const
    {
        return array_helper::deref(m_data, 0);
    }
    T &back()
    {
        return array_helper::deref(m_data, N - 1);
    }
    constexpr const T &back() const
    {
        return array_helper::deref(m_data, N - 1);
    }
    T *data()
    {
        return &array_helper::deref(m_data, 0);
    }
    const T *data() const
    {
        return &array_helper::deref(m_data, 0);
    }
    iterator begin()
    {
        return &array_helper::deref(m_data, 0);
    }
    const_iterator begin() const
    {
        return &array_helper::deref(m_data, 0);
    }
    const_iterator cbegin() const
    {
        return &array_helper::deref(m_data, 0);
    }
    iterator end()
    {
        return &array_helper::deref(m_data, N);
    }
    const_iterator end() const
    {
        return &array_helper::deref(m_data, N);
    }
    const_iterator cend() const
    {
        return &array_helper::deref(m_data, N);
    }
    reverse_iterator rbegin()
    {
        return reverse_iterator(&array_helper::deref(m_data, N));
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(&array_helper::deref(m_data, N));
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(&array_helper::deref(m_data, N));
    }
    reverse_iterator rend()
    {
        return reverse_iterator(&array_helper::deref(m_data, 0));
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(&array_helper::deref(m_data, 0));
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(&array_helper::deref(m_data, 0));
    }
    constexpr std::size_t size() const
    {
        return N;
    }
    constexpr bool empty() const
    {
        return N == 0;
    }
    void fill(const T &value)
    {
        for(std::size_t i = 0; i < N; i++)
            array_helper::deref(m_data, i) = value;
    }
private:
    static constexpr bool is_swap_noexcept()
    {
        using std::swap;
        return noexcept(std::swap(std::declval<T &>(), std::declval<T &>()));
    }
public:
    void swap(checked_array &other) noexcept(is_swap_noexcept())
    {
        using std::swap;
        for(std::size_t i = 0; i < N; i++)
            swap(array_helper::deref(m_data, i), array_helper::deref(other.m_data, i));
    }
};
}
}

namespace std
{
template <typename T, size_t N>
void swap(programmerjake::voxels::checked_array<T, N> &a, programmerjake::voxels::checked_array<T, N> &b) noexcept(noexcept(swap(declval<T &>(), declval<T &>())))
{
    a.swap(b);
}
}

#endif // CHECKED_ARRAY_H_INCLUDED
