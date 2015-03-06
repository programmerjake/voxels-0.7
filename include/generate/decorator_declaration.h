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
class Decorator;
typedef const Decorator *DecoratorPointer;
typedef std::size_t DecoratorIndex;
static constexpr DecoratorIndex DecoratorIndexNone = ~(DecoratorIndex)0;
class Decorators_t final
{
    friend class Decorator;
private:
    static std::unordered_map<std::wstring, DecoratorPointer> *map;
    static std::vector<DecoratorPointer> *list;
    static void makeMap()
    {
        if(map == nullptr)
        {
            map = new std::unordered_map<std::wstring, DecoratorPointer>;
            list = new std::vector<DecoratorPointer>;
        }
    }
    static void addDecorator(Decorator *decorator);
    static DecoratorIndex getIndex(DecoratorPointer decorator);
public:
    std::size_t size() const
    {
        makeMap();
        return list->size();
    }
    typedef typename std::vector<DecoratorPointer>::const_iterator iterator;
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
    iterator find(DecoratorPointer decorator)
    {
        makeMap();
        if(decorator == nullptr)
            return list->end();
        return list->begin() + getIndex(decorator);
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

static constexpr Decorators_t DecoratorsList;
}
}

#endif // DECORATOR_DECLARATION_H_INCLUDED
