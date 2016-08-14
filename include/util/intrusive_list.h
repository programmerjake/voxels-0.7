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
#ifndef INTRUSIVE_LIST_H_INCLUDED
#define INTRUSIVE_LIST_H_INCLUDED

#include <iterator>
#include <cassert>
#include <functional>
#include <algorithm>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
template <typename T>
class intrusive_list_members;
template <typename T, intrusive_list_members<T> T::*member>
class intrusive_list;

template <typename T>
class intrusive_list_members
{
    intrusive_list_members(const intrusive_list_members &) = delete;
    const intrusive_list_members &operator=(const intrusive_list_members &) = delete;
    template <typename T2, intrusive_list_members<T2> T2::*member>
    friend class intrusive_list;

private:
    T *next;
    T *prev;

public:
    constexpr intrusive_list_members() : next(nullptr), prev(nullptr)
    {
    }
    constexpr bool is_linked() const
    {
        return next != nullptr;
    }
    ~intrusive_list_members()
    {
        //        assert(!is_linked());
    }
};
template <typename T, intrusive_list_members<T> T::*member>
class intrusive_list
{
public:
    typedef T value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;

private:
    std::size_t elementCount;
    std::function<void(T *)> deleter;
    alignas(T) char endOfListElement[sizeof(T)];
    T *getEndOfListElement()
    {
        return reinterpret_cast<T *>(endOfListElement);
    }
    const T *getEndOfListElement() const
    {
        return reinterpret_cast<const T *>(endOfListElement);
    }
    intrusive_list_members<T> *getEndOfListMember()
    {
        return &(getEndOfListElement()->*member);
    }
    const intrusive_list_members<T> *getEndOfListMember() const
    {
        return &(getEndOfListElement()->*member);
    }
    void deleteNode(T *node) noexcept
    {
        deleter(node);
    }

public:
    explicit intrusive_list(std::function<void(T *)> deleter)
        : elementCount(0), deleter(std::move(deleter)), endOfListElement{0}
    {
        if(this->deleter == nullptr)
        {
            this->deleter = [](T *)
            {
            };
        }
        getEndOfListMember()->next = getEndOfListElement();
        getEndOfListMember()->prev = getEndOfListElement();
    }
    ~intrusive_list()
    {
        clear();
    }
    intrusive_list(intrusive_list &&rt)
    {
        (rt.getEndOfListMember()->next->*member).prev = getEndOfListElement();
        (rt.getEndOfListMember()->prev->*member).next = getEndOfListElement();
        getEndOfListMember()->next = rt.getEndOfListMember()->next;
        getEndOfListMember()->prev = rt.getEndOfListMember()->prev;
        rt.getEndOfListMember()->next = rt.getEndOfListElement();
        rt.getEndOfListMember()->prev = rt.getEndOfListElement();
        elementCount = rt.elementCount;
        rt.elementCount = 0;
        deleter = rt.deleter;
    }
    const intrusive_list &operator=(intrusive_list &&rt)
    {
        clear();
        (rt.getEndOfListMember()->next->*member).prev = getEndOfListElement();
        (rt.getEndOfListMember()->prev->*member).next = getEndOfListElement();
        getEndOfListMember()->next = rt.getEndOfListMember()->next;
        getEndOfListMember()->prev = rt.getEndOfListMember()->prev;
        rt.getEndOfListMember()->next = rt.getEndOfListElement();
        rt.getEndOfListMember()->prev = rt.getEndOfListElement();
        elementCount = rt.elementCount;
        rt.elementCount = 0;
        deleter = rt.deleter;
        return *this;
    }
    void swap(intrusive_list &rt)
    {
        intrusive_list v = std::move(rt);
        rt = std::move(*this);
        *this = std::move(v);
    }
    void clear()
    {
        while(!empty())
            pop_front();
    }
    std::size_t size() const
    {
        return elementCount;
    }
    bool empty() const
    {
        return elementCount == 0;
    }
    GCC_PRAGMA(diagnostic push)
    GCC_PRAGMA(diagnostic ignored "-Weffc++")
    class iterator : public std::iterator<std::bidirectional_iterator_tag, T>
    {
        GCC_PRAGMA(diagnostic pop)
        friend class intrusive_list;

    private:
        T *node;
        constexpr iterator(T *node) : node(node)
        {
        }

    public:
        constexpr iterator() : iterator(nullptr)
        {
        }
        T &operator*() const
        {
            return *node;
        }
        T *operator->() const
        {
            return node;
        }
        constexpr bool operator==(iterator rt) const
        {
            return node == rt.node;
        }
        constexpr bool operator!=(iterator rt) const
        {
            return node != rt.node;
        }
        const iterator &operator++()
        {
            node = (node->*member).next;
            return *this;
        }
        iterator operator++(int)
        {
            iterator retval = *this;
            operator++();
            return *this;
        }
        const iterator &operator--()
        {
            node = (node->*member).prev;
            return *this;
        }
        iterator operator--(int)
        {
            iterator retval = *this;
            operator--();
            return *this;
        }
    };
    GCC_PRAGMA(diagnostic push)
    GCC_PRAGMA(diagnostic ignored "-Weffc++")
    class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const T>
    {
        GCC_PRAGMA(diagnostic pop)
        friend class intrusive_list;

    private:
        const T *node;
        constexpr const_iterator(const T *node) : node(node)
        {
        }

    public:
        constexpr const_iterator() : const_iterator(nullptr)
        {
        }
        const_iterator(const typename intrusive_list::iterator &rt)
            : const_iterator(rt.operator->())
        {
        }
        constexpr const T &operator*() const
        {
            return *node;
        }
        constexpr const T *operator->() const
        {
            return node;
        }
        constexpr bool operator==(const_iterator rt) const
        {
            return node == rt.node;
        }
        constexpr bool operator!=(const_iterator rt) const
        {
            return node != rt.node;
        }
        constexpr bool operator==(iterator rt) const
        {
            return *this == const_iterator(rt);
        }
        constexpr bool operator!=(iterator rt) const
        {
            return *this != const_iterator(rt);
        }
        const const_iterator &operator++()
        {
            node = (node->*member).next;
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator retval = *this;
            operator++();
            return *this;
        }
        const const_iterator &operator--()
        {
            node = (node->*member).prev;
            return *this;
        }
        const_iterator operator--(int)
        {
            const_iterator retval = *this;
            operator--();
            return *this;
        }
    };
    iterator begin()
    {
        return iterator(getEndOfListMember()->next);
    }
    iterator end()
    {
        return iterator(getEndOfListElement());
    }
    const_iterator begin() const
    {
        return const_iterator(getEndOfListMember()->next);
    }
    const_iterator end() const
    {
        return const_iterator(getEndOfListElement());
    }
    const_iterator cbegin() const
    {
        return const_iterator(getEndOfListMember()->next);
    }
    const_iterator cend() const
    {
        return const_iterator(getEndOfListElement());
    }
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(cbegin());
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(cbegin());
    }
    T &front() const
    {
        assert(!empty());
        return *getEndOfListMember()->next;
    }
    T &back() const
    {
        assert(!empty());
        return *getEndOfListMember()->prev;
    }
    iterator to_iterator(T *node)
    {
        assert(node != nullptr);
        assert((node->*member).is_linked());
        return iterator(node);
    }
    const_iterator to_iterator(const T *node) const
    {
        assert(node != nullptr);
        assert((node->*member).is_linked());
        return iterator(node);
    }
    iterator insert(const_iterator pos, T *node)
    {
        assert(node != nullptr);
        assert(!(node->*member).is_linked());
        elementCount++;
        T *pos_node = const_cast<T *>(pos.node);
        T *prev_node = (pos_node->*member).prev;
        (node->*member).prev = prev_node;
        (node->*member).next = pos_node;
        (prev_node->*member).next = node;
        (pos_node->*member).prev = node;
        return iterator(node);
    }
    iterator erase(const_iterator pos)
    {
        assert(pos != cend());
        T *node = const_cast<T *>(pos.node);
        T *prev_node = (node->*member).prev;
        T *next_node = (node->*member).next;
        (node->*member).next = nullptr;
        (node->*member).prev = nullptr;
        (prev_node->*member).next = next_node;
        (next_node->*member).prev = prev_node;
        elementCount--;
        deleteNode(node);
        return iterator(next_node);
    }
    T *detach(const_iterator pos)
    {
        assert(pos != cend());
        T *node = const_cast<T *>(pos.node);
        T *prev_node = (node->*member).prev;
        T *next_node = (node->*member).next;
        (node->*member).next = nullptr;
        (node->*member).prev = nullptr;
        (prev_node->*member).next = next_node;
        (next_node->*member).prev = prev_node;
        elementCount--;
        return node;
    }
    void push_front(T *node)
    {
        insert(cbegin(), node);
    }
    void pop_front()
    {
        erase(cbegin());
    }
    void push_back(T *node)
    {
        insert(cend(), node);
    }
    void pop_back()
    {
        const_iterator back = cend();
        --back;
        erase(back);
    }
    void splice(const_iterator pos, intrusive_list &other, const_iterator it)
    {
        assert(it != other.cend());
        if(it == pos)
            return;
        const_iterator next = it;
        ++next;
        if(pos == next)
            return;
        insert(pos, other.detach(it));
    }
    void splice(const_iterator pos, intrusive_list &&other, const_iterator it)
    {
        splice(pos, other, it);
    }
    void splice(const_iterator pos,
                intrusive_list &other,
                const_iterator first,
                const_iterator last)
    {
        const_iterator i = first;
        while(i != last)
        {
            const_iterator next = i;
            ++next;
            splice(pos, other, i);
            i = next;
        }
    }
    void splice(const_iterator pos,
                intrusive_list &&other,
                const_iterator first,
                const_iterator last)
    {
        splice(pos, other, first, last);
    }
    void splice(const_iterator pos, intrusive_list &other)
    {
        assert(&other != this);
        splice(pos, other, other.begin(), other.end());
    }
    void splice(const_iterator pos, intrusive_list &&other)
    {
        splice(pos, other);
    }
};
}
}

#endif // INTRUSIVE_LIST_H_INCLUDED
