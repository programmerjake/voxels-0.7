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
#ifndef ITERATOR_H_INCLUDED
#define ITERATOR_H_INCLUDED

#include <iterator>
#include <type_traits>
#include <utility>
#include <memory>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{

template <typename IT1, typename IT2>
struct joined_iterator_value_type_helper final
{
    static_assert(std::is_same<typename std::remove_const<typename std::iterator_traits<IT1>::value_type>::type, typename std::remove_const<typename std::iterator_traits<IT1>::value_type>::type>::value, "can't join incompatible iterators");
    typedef typename std::conditional<std::is_const<typename std::iterator_traits<IT1>::value_type>::value || std::is_const<typename std::iterator_traits<IT1>::value_type>::value, typename std::add_const<typename std::iterator_traits<IT1>::value_type>::type, typename std::iterator_traits<IT1>::value_type>::type type;
};

template <typename TT1, typename TT2>
struct combined_iterator_category final
{
};

template <typename T>
struct combined_iterator_category<T, T> final
{
    typedef T type;
};

template<>
struct combined_iterator_category<std::input_iterator_tag, std::forward_iterator_tag> final
{
    typedef std::input_iterator_tag type;
};

template<>
struct combined_iterator_category<std::input_iterator_tag, std::bidirectional_iterator_tag> final
{
    typedef std::input_iterator_tag type;
};

template<>
struct combined_iterator_category<std::input_iterator_tag, std::random_access_iterator_tag> final
{
    typedef std::input_iterator_tag type;
};

template<>
struct combined_iterator_category<std::forward_iterator_tag, std::input_iterator_tag> final
{
    typedef std::input_iterator_tag type;
};

template<>
struct combined_iterator_category<std::forward_iterator_tag, std::bidirectional_iterator_tag> final
{
    typedef std::forward_iterator_tag type;
};

template<>
struct combined_iterator_category<std::forward_iterator_tag, std::random_access_iterator_tag> final
{
    typedef std::forward_iterator_tag type;
};

template<>
struct combined_iterator_category<std::bidirectional_iterator_tag, std::input_iterator_tag> final
{
    typedef std::input_iterator_tag type;
};

template<>
struct combined_iterator_category<std::bidirectional_iterator_tag, std::forward_iterator_tag> final
{
    typedef std::forward_iterator_tag type;
};

template<>
struct combined_iterator_category<std::bidirectional_iterator_tag, std::random_access_iterator_tag> final
{
    typedef std::bidirectional_iterator_tag type;
};

template<>
struct combined_iterator_category<std::random_access_iterator_tag, std::input_iterator_tag> final
{
    typedef std::input_iterator_tag type;
};

template<>
struct combined_iterator_category<std::random_access_iterator_tag, std::forward_iterator_tag> final
{
    typedef std::forward_iterator_tag type;
};

template<>
struct combined_iterator_category<std::random_access_iterator_tag, std::bidirectional_iterator_tag> final
{
    typedef std::bidirectional_iterator_tag type;
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
template <typename IT1, typename IT2>
class joined_iterator final : public std::iterator<typename combined_iterator_category<std::forward_iterator_tag, typename combined_iterator_category<typename std::iterator_traits<IT1>::iterator_category, typename std::iterator_traits<IT2>::iterator_category>::type>::type, typename joined_iterator_value_type_helper<IT1, IT2>::type>
{
GCC_PRAGMA(diagnostic pop)
public:
    typedef IT1 iterator_type1;
    typedef IT2 iterator_type2;
private:
    iterator_type1 iterator1;
    iterator_type1 iterator1end;
    iterator_type2 iterator2;
public:
    joined_iterator(iterator_type1 iterator1, iterator_type1 iterator1end, iterator_type2 iterator2)
        : iterator1(iterator1), iterator1end(iterator1end), iterator2(iterator2)
    {
    }
    joined_iterator()
    {
    }
    typename joined_iterator_value_type_helper<IT1, IT2>::type &operator *() const
    {
        static_assert(std::is_same<typename std::remove_const<typename std::iterator_traits<IT1>::value_type &>::type, typename std::remove_const<decltype(*std::declval<IT1>())>::type>::value, "unsupported return type for operator *()");
        static_assert(std::is_same<typename std::remove_const<typename std::iterator_traits<IT1>::value_type &>::type, typename std::remove_const<decltype(*std::declval<IT2>())>::type>::value, "unsupported return type for operator *()");
        return iterator1 == iterator1end ? *iterator2 : *iterator1;
    }
    typename joined_iterator_value_type_helper<IT1, IT2>::type *operator ->() const
    {
        return std::addressof(operator *());
    }
    const joined_iterator &operator ++()
    {
        if(iterator1 == iterator1end)
            ++iterator2;
        else
            ++iterator1;
        return *this;
    }
    joined_iterator operator ++(int)
    {
        joined_iterator retval = *this;
        if(iterator1 == iterator1end)
            ++iterator2;
        else
            ++iterator1;
        return retval;
    }
    bool operator ==(const joined_iterator &rt) const
    {
        return iterator1 == rt.iterator1 && iterator2 == rt.iterator2;
    }
    bool operator !=(const joined_iterator &rt) const
    {
        return !operator ==(rt);
    }
};

template <typename IT1, typename IT2>
joined_iterator<typename std::decay<IT1>::type, typename std::decay<IT2>::type> join_iterators(IT1 iterator1, IT1 iterator1end, IT2 iterator2)
{
    return joined_iterator<typename std::decay<IT1>::type, typename std::decay<IT2>::type>(iterator1, iterator1end, iterator2);
}

template <typename T>
class range final
{
public:
    typedef T iterator;
private:
    iterator beginV, endV;
public:
    range(T beginV, T endV)
        : beginV(beginV), endV(endV)
    {
    }
    iterator begin() const
    {
        return beginV;
    }
    iterator end() const
    {
        return endV;
    }
};

template <typename T>
struct iterator_type_helper
{
    typedef typename std::decay<decltype(begin(std::declval<T>()))>::type type;
    static type beginImp(T &c)
    {
        return begin(c);
    }
    static type endImp(T &c)
    {
        return end(c);
    }
};

template <typename T, typename = void>
struct iterator_type final
{
    typedef typename iterator_type_helper<T>::type type;
    static type begin(T &&c)
    {
        return iterator_type_helper<T>::beginImp(c);
    }
    static type end(T &&c)
    {
        return iterator_type_helper<T>::endImp(c);
    }
};

template <typename T, std::size_t N>
struct iterator_type<T[N], void> final
{
    typedef typename std::decay<T[N]>::type type;
    static type begin(T (&c)[N])
    {
        return c;
    }
    static type end(T (&c)[N])
    {
        return c + N;
    }
};

template <typename CT, typename = void>
class iterator_type_helper_has_begin final
{
public:
    static constexpr bool has_begin = false;
};

template <typename CT>
class iterator_type_helper_has_begin<CT, typename std::enable_if<std::is_class<CT>::value>::type> final
{
private:
    struct yes
    {
    };
    struct no
    {
    };
    template <typename T, typename RT, RT (T::*)() = &CT::begin>
    static yes fn(T *);
    static no fn(...);
public:
    static constexpr bool has_begin = std::is_same<yes, decltype(fn(std::declval<CT *>()))>::value;
};

template <typename CT>
struct iterator_type<CT, typename std::enable_if<iterator_type_helper_has_begin<CT>::has_begin>::type> final
{
    typedef typename std::decay<decltype(std::declval<CT>().begin())>::type type;
    static type begin(CT &&c)
    {
        return c.begin();
    }
    static type end(CT &&c)
    {
        return c.end();
    }
};

template <typename CT>
range<typename iterator_type<CT>::type> to_range(CT &&container)
{
    return range<typename iterator_type<CT>::type>(iterator_type<CT>::begin(container), iterator_type<CT>::end(container));
}

template <typename CT1, typename CT2>
range<joined_iterator<typename iterator_type<CT1>::type, typename iterator_type<CT2>::type>> join_ranges(CT1 &&c1, CT2 &&c2)
{
    typedef joined_iterator<typename iterator_type<CT1>::type, typename iterator_type<CT2>::type> joined_iterator_type;
    range<typename iterator_type<CT1>::type> r1 = to_range(c1);
    range<typename iterator_type<CT2>::type> r2 = to_range(c2);
    joined_iterator_type beginV = joined_iterator_type(r1.begin(), r1.end(), r2.begin());
    joined_iterator_type endV = joined_iterator_type(r1.end(), r1.end(), r2.end());
    return range<joined_iterator_type>(beginV, endV);
}

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
template <typename T>
struct empty_range_iterator final : public std::iterator<std::random_access_iterator_tag, T>
{
GCC_PRAGMA(diagnostic pop)
public:
    typedef T value_type;
    bool operator ==(const empty_range_iterator &rt) const
    {
        return true;
    }
    bool operator !=(const empty_range_iterator &rt) const
    {
        return false;
    }
    const empty_range_iterator &operator ++()
    {
        return *this;
    }
    empty_range_iterator operator ++(int)
    {
        return *this;
    }
    const empty_range_iterator &operator --()
    {
        return *this;
    }
    empty_range_iterator operator --(int)
    {
        return *this;
    }
    const empty_range_iterator &operator +=(std::ptrdiff_t)
    {
        return *this;
    }
    const empty_range_iterator &operator -=(std::ptrdiff_t)
    {
        return *this;
    }
    std::ptrdiff_t operator -(const empty_range_iterator &rt) const
    {
        return 0;
    }
    friend empty_range_iterator operator +(std::ptrdiff_t, empty_range_iterator v)
    {
        return v;
    }
    empty_range_iterator operator +(std::ptrdiff_t) const
    {
        return *this;
    }
    empty_range_iterator operator -(std::ptrdiff_t) const
    {
        return *this;
    }
    bool operator >(const empty_range_iterator &) const
    {
        return false;
    }
    bool operator <(const empty_range_iterator &) const
    {
        return false;
    }
    bool operator >=(const empty_range_iterator &) const
    {
        return true;
    }
    bool operator <=(const empty_range_iterator &) const
    {
        return true;
    }
    T &operator *() const
    {
        return *operator ->();
    }
    T *operator ->() const
    {
        return nullptr;
    }
    T &operator [](std::ptrdiff_t) const
    {
        return operator *();
    }
};

template <typename T>
range<empty_range_iterator<T>> empty_range()
{
    return range<empty_range_iterator<T>>(empty_range_iterator<T>(), empty_range_iterator<T>());
}

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
template <typename T>
struct unit_range_iterator final : public std::iterator<std::forward_iterator_tag, T>
{
GCC_PRAGMA(diagnostic pop)
private:
    T *value;
public:
    unit_range_iterator(const unit_range_iterator &) = default;
    unit_range_iterator &operator =(const unit_range_iterator &) = default;
    typedef T value_type;
    explicit unit_range_iterator(T &value)
        : value(std::addressof(value))
    {
    }
    unit_range_iterator()
        : value(nullptr)
    {
    }
    bool operator ==(const unit_range_iterator &rt) const
    {
        return value == rt.value;
    }
    bool operator !=(const unit_range_iterator &rt) const
    {
        return value != rt.value;
    }
    const unit_range_iterator &operator ++()
    {
        value = nullptr;
        return *this;
    }
    unit_range_iterator operator ++(int)
    {
        unit_range_iterator retval = *this;
        value = nullptr;
        return retval;
    }
    T &operator *() const
    {
        return *value;
    }
    T *operator ->() const
    {
        return value;
    }
};

template <typename T>
range<unit_range_iterator<T>> unit_range(T &value)
{
    return range<unit_range_iterator<T>>(unit_range_iterator<T>(value), unit_range_iterator<T>());
}

}
}

#endif // ITERATOR_H_INCLUDED
