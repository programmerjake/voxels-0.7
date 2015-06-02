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
#ifndef ENUM_TRAITS_H_INCLUDED
#define ENUM_TRAITS_H_INCLUDED

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <cassert>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
template <typename T, typename = void>
struct enum_traits;

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
template <typename T>
struct enum_iterator : public std::iterator<std::random_access_iterator_tag, const T>
{
GCC_PRAGMA(diagnostic pop)
    T value;
    constexpr enum_iterator()
        : value((T)0)
    {
    }
    constexpr enum_iterator(T value)
        : value(value)
    {
    }
    constexpr const T operator *() const
    {
        return value;
    }
    friend constexpr enum_iterator<T> operator +(std::ptrdiff_t a, enum_iterator b)
    {
        return enum_iterator<T>((T)(a + (std::ptrdiff_t)b.value));
    }
    constexpr enum_iterator<T> operator +(std::ptrdiff_t b) const
    {
        return enum_iterator<T>((T)((std::ptrdiff_t)value + b));
    }
    constexpr enum_iterator<T> operator -(std::ptrdiff_t b) const
    {
        return enum_iterator<T>((T)((std::ptrdiff_t)value - b));
    }
    constexpr std::ptrdiff_t operator -(enum_iterator b) const
    {
        return (std::ptrdiff_t)value - (std::ptrdiff_t)b.value;
    }
    constexpr const T operator [](std::ptrdiff_t b) const
    {
        return (T)((std::ptrdiff_t)value + b);
    }
    const enum_iterator & operator +=(std::ptrdiff_t b)
    {
        value = (T)((std::ptrdiff_t)value + b);
        return *this;
    }
    const enum_iterator & operator -=(std::ptrdiff_t b)
    {
        value = (T)((std::ptrdiff_t)value - b);
        return *this;
    }
    const enum_iterator & operator ++()
    {
        value = (T)((std::ptrdiff_t)value + 1);
        return *this;
    }
    const enum_iterator & operator --()
    {
        value = (T)((std::ptrdiff_t)value - 1);
        return *this;
    }
    enum_iterator operator ++(int)
    {
        T retval = value;
        value = (T)((std::ptrdiff_t)value + 1);
        return enum_iterator(retval);
    }
    enum_iterator operator --(int)
    {
        T retval = value;
        value = (T)((std::ptrdiff_t)value - 1);
        return enum_iterator(retval);
    }
    constexpr bool operator ==(enum_iterator b) const
    {
        return value == b.value;
    }
    constexpr bool operator !=(enum_iterator b) const
    {
        return value != b.value;
    }
    constexpr bool operator >=(enum_iterator b) const
    {
        return value >= b.value;
    }
    constexpr bool operator <=(enum_iterator b) const
    {
        return value <= b.value;
    }
    constexpr bool operator >(enum_iterator b) const
    {
        return value > b.value;
    }
    constexpr bool operator <(enum_iterator b) const
    {
        return value < b.value;
    }
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
template <>
struct enum_iterator<bool> : public std::iterator<std::random_access_iterator_tag, const bool>
{
GCC_PRAGMA(diagnostic pop)
    unsigned char value;
    constexpr enum_iterator()
        : value(0)
    {
    }
    constexpr enum_iterator(bool value)
        : value(value ? 1 : 0)
    {
    }
    struct int_construct_t
    {
    };
    constexpr enum_iterator(int value, int_construct_t)
        : value(value)
    {
    }
    constexpr const bool operator *() const
    {
        return value != 0;
    }
    friend constexpr enum_iterator<bool> operator +(std::ptrdiff_t a, enum_iterator b)
    {
        return enum_iterator<bool>(a + b.value, int_construct_t());
    }
    constexpr enum_iterator<bool> operator +(std::ptrdiff_t b) const
    {
        return enum_iterator<bool>(value + b, int_construct_t());
    }
    constexpr enum_iterator<bool> operator -(std::ptrdiff_t b) const
    {
        return enum_iterator<bool>(value - b, int_construct_t());
    }
    constexpr std::ptrdiff_t operator -(enum_iterator b) const
    {
        return (std::ptrdiff_t)value - (std::ptrdiff_t)b.value;
    }
    constexpr const bool operator [](std::ptrdiff_t b) const
    {
        return operator +(b).operator *();
    }
    const enum_iterator & operator +=(std::ptrdiff_t b)
    {
        value += b;
        return *this;
    }
    const enum_iterator & operator -=(std::ptrdiff_t b)
    {
        value -= b;
        return *this;
    }
    const enum_iterator & operator ++()
    {
        value++;
        return *this;
    }
    const enum_iterator & operator --()
    {
        value--;
        return *this;
    }
    enum_iterator operator ++(int)
    {
        enum_iterator retval = *this;
        value++;
        return retval;
    }
    enum_iterator operator --(int)
    {
        enum_iterator retval = *this;
        value--;
        return retval;
    }
    constexpr bool operator ==(enum_iterator b) const
    {
        return value == b.value;
    }
    constexpr bool operator !=(enum_iterator b) const
    {
        return value != b.value;
    }
    constexpr bool operator >=(enum_iterator b) const
    {
        return value >= b.value;
    }
    constexpr bool operator <=(enum_iterator b) const
    {
        return value <= b.value;
    }
    constexpr bool operator >(enum_iterator b) const
    {
        return value > b.value;
    }
    constexpr bool operator <(enum_iterator b) const
    {
        return value < b.value;
    }
};

template <typename T, T minV, T maxV>
struct enum_traits_default
{
    typedef typename std::enable_if<std::is_enum<T>::value, T>::type type;
    typedef typename std::underlying_type<T>::type rwtype;
    static constexpr T minimum = minV;
    static constexpr T maximum = maxV;
    static constexpr enum_iterator<T> begin()
    {
        return enum_iterator<T>(minimum);
    }
    static constexpr enum_iterator<T> end()
    {
        return enum_iterator<T>(maximum) + 1;
    }
    static constexpr std::size_t size()
    {
        return end() - begin();
    }
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
template <typename T>
struct enum_traits<T, typename std::enable_if<std::is_enum<T>::value>::type> : public enum_traits_default<T, T::enum_first, T::enum_last>
{
};
GCC_PRAGMA(diagnostic pop)

template <>
struct enum_traits<bool, void>
{
    typedef bool type;
    typedef unsigned char rwtype;
    static constexpr bool minimum = false;
    static constexpr bool maximum = true;
    static constexpr enum_iterator<bool> begin()
    {
        return enum_iterator<bool>(minimum);
    }
    static constexpr enum_iterator<bool> end()
    {
        return enum_iterator<bool>(maximum) + 1;
    }
    static constexpr std::size_t size()
    {
        return 2;
    }
};

#define DEFINE_ENUM_LIMITS(first, last) \
enum_first = first, \
enum_last = last,

template <typename T, typename BT>
struct enum_struct
{
    typedef T type;
    typedef BT base_type;
    static_assert(std::is_integral<base_type>::value, "base type must be an integral type");
    base_type value;
    explicit constexpr enum_struct(base_type value)
        : value(value)
    {
    }
    enum_struct() = default;
    explicit constexpr operator base_type() const
    {
        return value;
    }
    explicit constexpr operator std::ptrdiff_t() const
    {
        return (std::ptrdiff_t)value;
    }
    constexpr bool operator ==(enum_struct rt) const
    {
        return value == rt.value;
    }
    constexpr bool operator !=(enum_struct rt) const
    {
        return value != rt.value;
    }
};

#define DEFINE_ENUM_STRUCT_LIMITS(first, last) \
static constexpr type enum_first() {return first();} \
static constexpr type enum_last() {return last();}

template <typename T>
struct enum_traits<T, typename std::enable_if<std::is_base_of<enum_struct<T, typename T::base_type>, T>::value>::type>
{
    typedef T type;
    typedef typename T::base_type rwtype;
    static constexpr T minimum = T::enum_first();
    static constexpr T maximum = T::enum_last();
    static constexpr enum_iterator<T> begin()
    {
        return enum_iterator<T>(minimum);
    }
    static constexpr enum_iterator<T> end()
    {
        return enum_iterator<T>(maximum) + 1;
    }
    static constexpr std::size_t size()
    {
        return end() - begin();
    }
};

template <typename T>
constexpr T enum_traits<T, typename std::enable_if<std::is_base_of<enum_struct<T, typename T::base_type>, T>::value>::type>::minimum;

template <typename T>
constexpr T enum_traits<T, typename std::enable_if<std::is_base_of<enum_struct<T, typename T::base_type>, T>::value>::type>::maximum;

template <typename T, typename EnumT>
struct enum_array
{
    typedef T value_type;
    typedef EnumT index_type;
    typedef value_type * iterator;
    typedef const value_type * const_iterator;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    value_type elements[enum_traits<index_type>::size()];
    value_type & operator [](index_type index)
    {
        assert(index >= enum_traits<index_type>::minimum && index <= enum_traits<index_type>::maximum);
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    const value_type & operator [](index_type index) const
    {
        assert(index >= enum_traits<index_type>::minimum && index <= enum_traits<index_type>::maximum);
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    value_type & at(index_type index)
    {
        assert(index >= enum_traits<index_type>::minimum && index <= enum_traits<index_type>::maximum);
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    const value_type & at(index_type index) const
    {
        assert(index >= enum_traits<index_type>::minimum && index <= enum_traits<index_type>::maximum);
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    static constexpr std::size_t size()
    {
        return enum_traits<index_type>::size();
    }
    iterator begin()
    {
        return &elements[0];
    }
    iterator end()
    {
        return begin() + size();
    }
    const_iterator begin() const
    {
        return &elements[0];
    }
    const_iterator end() const
    {
        return begin() + size();
    }
    const_iterator cbegin() const
    {
        return &elements[0];
    }
    const_iterator cend() const
    {
        return begin() + size();
    }
    const_iterator from_pointer(const_pointer v) const
    {
        return v;
    }
    iterator from_pointer(pointer v) const
    {
        return v;
    }
};
}
}

#endif // ENUM_TRAITS_H_INCLUDED
