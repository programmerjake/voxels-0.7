/*
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
#include "entity/entity.h"
#include "physics/physics.h"

using namespace std;

namespace programmerjake
{
namespace voxels
{
void Entity::destroy()
{
    if(physicsObject)
        physicsObject->destroy();
    *this = Entity();
}

EntityDescriptor::EntityDescriptor(wstring name, shared_ptr<const PhysicsObjectConstructor> physicsObjectConstructor)
    : name(name), physicsObjectConstructor(physicsObjectConstructor)
{
    EntityDescriptors.add(this);
}

EntityDescriptor::~EntityDescriptor()
{
    EntityDescriptors.remove(this);
}

linked_map<wstring, EntityDescriptorPointer> *EntityDescriptors_t::entitiesMap = nullptr;

void EntityDescriptors_t::add(EntityDescriptorPointer bd) const
{
    assert(bd != nullptr);
    if(entitiesMap == nullptr)
        entitiesMap = new linked_map<wstring, EntityDescriptorPointer>();
    bool wasInserted = std::get<1>(entitiesMap->insert(make_pair(bd->name, bd)));
    assert(wasInserted);
}

void EntityDescriptors_t::remove(EntityDescriptorPointer bd) const
{
    assert(bd != nullptr);
    assert(entitiesMap != nullptr);
    size_t removedCount = entitiesMap->erase(bd->name);
    assert(removedCount == 1);
    if(entitiesMap->empty())
    {
        delete entitiesMap;
        entitiesMap = nullptr;
    }
}
}
}
