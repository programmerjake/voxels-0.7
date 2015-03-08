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
DecoratorDescriptor::DecoratorDescriptor(std::wstring name, int chunkSearchDistance)
    : index(DecoratorDescriptorIndexNone), name(name), chunkSearchDistance(chunkSearchDistance)
{
    assert(chunkSearchDistance >= 0);
    DecoratorDescriptors_t::addDescriptor(this);
}

std::unordered_map<std::wstring, DecoratorDescriptorPointer> *DecoratorDescriptors_t::map = nullptr;
std::vector<DecoratorDescriptorPointer> *DecoratorDescriptors_t::list = nullptr;

void DecoratorDescriptors_t::addDescriptor(DecoratorDescriptor *descriptor)
{
    assert(descriptor != nullptr);
    makeMap();
    if(!std::get<1>(map->insert(std::pair<std::wstring, DecoratorDescriptorPointer>(descriptor->name, descriptor))))
    {
        assert(false);
    }
    descriptor->index = list->size();
    list->push_back(descriptor);
}

DecoratorDescriptorIndex DecoratorDescriptors_t::getIndex(DecoratorDescriptorPointer descriptor)
{
    return descriptor->getIndex();
}

}
}
