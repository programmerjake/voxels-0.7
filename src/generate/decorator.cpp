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
#include "generate/decorator.h"
#include <cassert>

namespace programmerjake
{
namespace voxels
{
Decorator::Decorator(std::wstring name, int chunkSearchDistance)
    : index(DecoratorIndexNone), name(name), chunkSearchDistance(chunkSearchDistance)
{
    assert(chunkSearchDistance >= 0);
    Decorators_t::addDecorator(this);
}

std::unordered_map<std::wstring, DecoratorPointer> *Decorators_t::map = nullptr;
std::vector<DecoratorPointer> *Decorators_t::list = nullptr;

void Decorators_t::addDecorator(Decorator *decorator)
{
    assert(decorator != nullptr);
    makeMap();
    if(!std::get<1>(map->insert(std::pair<std::wstring, DecoratorPointer>(decorator->name, decorator))))
    {
        assert(false);
    }
    decorator->index = list->size();
    list->push_back(decorator);
}

DecoratorIndex Decorators_t::getIndex(DecoratorPointer decorator)
{
    return decorator->getIndex();
}

}
}
