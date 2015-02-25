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
#ifndef ENTITY_STRUCT_H_INCLUDED
#define ENTITY_STRUCT_H_INCLUDED

#include "util/linked_map.h"
#include "ray_casting/ray_casting.h"
#include <cwchar>
#include <memory>
#include <string>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
class EntityDescriptor;
class PhysicsObject;

typedef const EntityDescriptor *EntityDescriptorPointer;

struct Entity final
{
    EntityDescriptorPointer descriptor;
    std::shared_ptr<PhysicsObject> physicsObject;
    std::shared_ptr<void> data;
    Entity()
        : descriptor(nullptr), physicsObject(nullptr), data(nullptr)
    {
    }
    Entity(EntityDescriptorPointer descriptor, std::shared_ptr<PhysicsObject> physicsObject, std::shared_ptr<void> data = nullptr)
        : descriptor(descriptor), physicsObject(physicsObject), data(data)
    {
    }
    bool good() const
    {
        return descriptor != nullptr && physicsObject != nullptr;
    }
    explicit operator bool() const
    {
        return good();
    }
    bool operator !() const
    {
        return !good();
    }
    void destroy();
};
}
}

#endif // ENTITY_STRUCT_H_INCLUDED

