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
#ifndef DECORATOR_DECLARATION_H_INCLUDED
#define DECORATOR_DECLARATION_H_INCLUDED

#include <memory>
#include <string>
#include <unordered_map>
#include <cassert>
#include <vector>

namespace programmerjake
{
namespace voxels
{
class DecoratorDescriptor;
typedef const DecoratorDescriptor *DecoratorDescriptorPointer;
typedef std::size_t DecoratorDescriptorIndex;
static constexpr DecoratorDescriptorIndex DecoratorDescriptorIndexNone = ~(DecoratorDescriptorIndex)0;
class DecoratorDescriptors_t final
{
    friend class DecoratorDescriptor;
private:
    static std::unordered_map<std::wstring, DecoratorDescriptorPointer> *map;
    static std::vector<DecoratorDescriptorPointer> *list;
    static void makeMap()
    {
        if(map == nullptr)
        {
            map = new std::unordered_map<std::wstring, DecoratorDescriptorPointer>;
            list = new std::vector<DecoratorDescriptorPointer>;
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
    typedef typename std::vector<DecoratorDescriptorPointer>::const_iterator iterator;
    iterator begin() const
    {
        makeMap();
        return list->begin();
    }
    iterator end() const
    {
        makeMap();
        return list->end();
    }
    iterator find(DecoratorDescriptorPointer descriptor)
    {
        makeMap();
        if(descriptor == nullptr)
            return list->end();
        return list->begin() + getIndex(descriptor);
    }
    iterator find(std::wstring name)
    {
        makeMap();
        auto iter = map->find(name);
        if(iter == map->end())
            return list->end();
        return list->begin() + getIndex(std::get<1>(*iter));
    }
};

static constexpr DecoratorDescriptors_t DecoratorDescriptors;
}
}

#endif // DECORATOR_DECLARATION_H_INCLUDED
