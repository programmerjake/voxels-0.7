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
#ifndef DECORATOR_DECLARATION_H_INCLUDED
#define DECORATOR_DECLARATION_H_INCLUDED

#include <memory>
#include <string>
#include <unordered_map>
#include <cassert>
#include <vector>
#include <map>
#include "util/iterator.h"
#include <iterator>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
class DecoratorDescriptor;
typedef const DecoratorDescriptor *DecoratorDescriptorPointer;
typedef std::size_t DecoratorDescriptorIndex;
static constexpr DecoratorDescriptorIndex DecoratorDescriptorIndexNone = ~static_cast<DecoratorDescriptorIndex>(0);
class DecoratorDescriptors_t final
{
    friend class DecoratorDescriptor;
private:
    static std::unordered_map<std::wstring, DecoratorDescriptorPointer> *map;
    static std::vector<std::multimap<float, DecoratorDescriptorPointer>::const_iterator> *list;
    static std::multimap<float, DecoratorDescriptorPointer> *sortedList;
    static void makeMap()
    {
        if(map == nullptr)
        {
            map = new std::unordered_map<std::wstring, DecoratorDescriptorPointer>;
            list = new std::vector<std::multimap<float, DecoratorDescriptorPointer>::const_iterator>;
            sortedList = new std::multimap<float, DecoratorDescriptorPointer>;
        }
    }
    static void addDescriptor(DecoratorDescriptor *descriptor);
    static DecoratorDescriptorIndex getIndex(DecoratorDescriptorPointer descriptor);
public:
    std::size_t size() const
    {
        makeMap();
        return list->size();
    }
GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
    class iterator final : public std::iterator<std::forward_iterator_tag, const DecoratorDescriptorPointer>
    {
GCC_PRAGMA(diagnostic pop)
        friend class DecoratorDescriptors_t;
    private:
        std::multimap<float, DecoratorDescriptorPointer>::const_iterator iter;
        iterator(std::multimap<float, DecoratorDescriptorPointer>::const_iterator iter)
            : iter(iter)
        {
        }
    public:
        iterator() = default;
        const DecoratorDescriptorPointer operator *() const
        {
            return std::get<1>(*iter);
        }
        const iterator &operator ++()
        {
            ++iter;
            return *this;
        }
        iterator operator ++(int)
        {
            iterator retval = *this;
            operator ++();
            return retval;
        }
        bool operator ==(const iterator &rt) const
        {
            return iter == rt.iter;
        }
        bool operator !=(const iterator &rt) const
        {
            return iter != rt.iter;
        }
    };
    iterator begin() const
    {
        makeMap();
        return iterator(sortedList->begin());
    }
    iterator end() const
    {
        makeMap();
        return iterator(sortedList->end());
    }
    iterator find(DecoratorDescriptorPointer descriptor)
    {
        makeMap();
        if(descriptor == nullptr)
            return end();
        return iterator(list->at(getIndex(descriptor)));
    }
    iterator find(std::wstring name)
    {
        makeMap();
        auto iter = map->find(name);
        if(iter == map->end())
            return end();
        return iterator(list->at(getIndex(std::get<1>(*iter))));
    }
};

static constexpr DecoratorDescriptors_t DecoratorDescriptors;
}
}

#endif // DECORATOR_DECLARATION_H_INCLUDED
