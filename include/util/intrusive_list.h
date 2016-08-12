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

private:
    bool isInList(const T *head) const noexcept
    {
        return this == head || next != nullptr || prev != nullptr;
    }
    template <intrusive_list_members<T> T::*member>
    void insert(T *self, T *&head, T *&tail, T *followingNode) noexcept
    {
        assert(this != head && this != tail && next == nullptr && prev == nullptr && &(self->*member) == this);
        if(followingNode)
        {
            prev = (followingNode->*member).prev;
            next = followingNode;
            if(prev == nullptr)
            {
                assert(head == followingNode);
                head = self;
            }
            else
            {
                (prev->*member).next = self;
            }
            (followingNode->*member).prev = self;
        }
        else
        {
            prev = tail;
            next = nullptr;
            if(prev == nullptr)
            {
                assert(head == nullptr);
                head = self;
            }
            else
            {
                (prev->*member).next = self;
            }
            tail = self;
        }
    }
    template <intrusive_list_members<T> T::*member>
    T *remove(T *&head, T *&tail) noexcept // returns node following this node
    {
        if(prev == nullptr)
        {
            assert(this == head);
            head = next;
        }
        else
        {
            (prev->*member).next = next;
        }
        if(next == nullptr)
        {
            assert(this == tail);
            tail = prev;
        }
        else
        {
            (next->*member).prev = prev;
        }
        prev = nullptr;
        T *retval = next;
        next = nullptr;
        return retval;
    }

public:
    constexpr intrusive_list_members() : next(nullptr), prev(nullptr)
    {
    }
    ~intrusive_list_members()
    {
        assert(next == nullptr && prev == nullptr);
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
    T *head;
    T *tail;
    void deleteNode(T *node) noexcept
    {
        if(deleter)
            deleter(node);
    }

public:
    explicit intrusive_list(std::function<void(T *)> deleter)
        : elementCount(0), deleter(std::move(deleter)), head(nullptr), tail(nullptr)
    {
    }
    ~intrusive_list()
    {
        clear();
    }
    intrusive_list(intrusive_list &&rt)
        : elementCount(rt.elementCount), deleter(rt.deleter), head(rt.head), tail(rt.tail)
    {
        rt.head = nullptr;
        rt.tail = nullptr;
        rt.elementCount = 0;
    }
    intrusive_list &operator=(intrusive_list rt)
    {
        swap(rt);
        return *this;
    }
    void swap(intrusive_list &rt) noexcept
    {
        using std::swap;
        swap(elementCount, rt.elementCount);
        swap(deleter, rt.deleter);
        swap(head, rt.head);
        swap(tail, rt.tail);
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
    class const_iterator;
    class iterator final
    {
        friend class intrusive_list;
        friend class const_iterator;

    private:
        T *node;
        intrusive_list *list;
        constexpr iterator(T *node, intrusive_list *list) : node(node), list(list)
        {
        }

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef T value_type;
        typedef std::ptrdiff_t difference_type;
        typedef T *pointer;
        typedef T &reference;
        constexpr iterator() : node(nullptr), list(nullptr)
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
            if(node)
                node = (node->*member).prev;
            else
                node = list->tail;
            return *this;
        }
        iterator operator--(int)
        {
            iterator retval = *this;
            operator--();
            return *this;
        }
    };
    class const_iterator final
    {
        friend class intrusive_list;

    private:
        const T *node;
        const intrusive_list *list;
        constexpr const_iterator(const T *node, const intrusive_list *list) : node(node), list(list)
        {
        }

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef T value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const T *pointer;
        typedef const T &reference;
        constexpr const_iterator() : const_iterator(nullptr)
        {
        }
        const_iterator(const iterator rt) : node(rt.node), list(rt.list)
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
            if(node)
                node = (node->*member).prev;
            else
                node = list->tail;
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
        return iterator(head, this);
    }
    iterator end()
    {
        return iterator(nullptr, this);
    }
    const_iterator begin() const
    {
        return const_iterator(head, this);
    }
    const_iterator end() const
    {
        return const_iterator(nullptr, this);
    }
    const_iterator cbegin() const
    {
        return const_iterator(head, this);
    }
    const_iterator cend() const
    {
        return const_iterator(nullptr, this);
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
        return *head;
    }
    T &back() const
    {
        assert(!empty());
        return *tail;
    }
    iterator to_iterator(T *node)
    {
        assert(node != nullptr);
        if(head == node || (node->*member).next != nullptr || (node->*member).prev != nullptr)
            return iterator(node, this);
        return end();
    }
    const_iterator to_iterator(const T *node) const
    {
        assert(node != nullptr);
        if(head == node || (node->*member).next != nullptr || (node->*member).prev != nullptr)
            return const_iterator(node, this);
        return end();
    }
    iterator insert(const_iterator pos, T *node)
    {
        assert(node != nullptr);
        assert(to_iterator(node) == end());
        elementCount++;
        (node->*member).template insert<member>(node, head, tail, const_cast<T *>(pos.node));
        return iterator(node, this);
    }
    iterator erase(const_iterator pos)
    {
        assert(pos != cend());
        T *node = const_cast<T *>(pos.node);
        T *next_node = (node->*member).template remove<member>(head, tail);
        elementCount--;
        deleteNode(node);
        return iterator(next_node, this);
    }
    T *detach(const_iterator pos)
    {
        assert(pos != cend());
        T *node = const_cast<T *>(pos.node);
        (node->*member).template remove<member>(head, tail);
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
