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
#include "item/item.h"
#include "entity/builtin/item.h"
#include "player/player.h"
#include <cassert>

namespace programmerjake
{
namespace voxels
{
ItemDescriptor::ItemDescriptor(std::wstring name, Matrix entityPreorientSelectionBoxTransform)
    : name(name)
{
    entity = new Entities::builtin::EntityItem(this, entityPreorientSelectionBoxTransform);
    ItemDescriptors.add(this);
}

ItemDescriptor::ItemDescriptor(std::wstring name, enum_array<Mesh, RenderLayer> entityMeshes, Matrix entityPreorientSelectionBoxTransform)
    : name(name)
{
    entity = new Entities::builtin::EntityItem(this, entityMeshes, entityPreorientSelectionBoxTransform);
    ItemDescriptors.add(this);
}

ItemDescriptor::~ItemDescriptor()
{
    ItemDescriptors.remove(this);
    delete entity;
}

Entity *ItemDescriptor::addToWorld(World &world, WorldLockManager &lock_manager, ItemStack itemStack, PositionF position, VectorF velocity)
{
    assert(itemStack.good());
    return world.addEntity(itemStack.item.descriptor->getEntity(), position, velocity, lock_manager, itemStack.item.descriptor->getEntity()->makeItemData(itemStack, world));
}

Entity *ItemDescriptor::addToWorld(World &world, WorldLockManager &lock_manager, ItemStack itemStack, PositionF position, VectorF velocity, const Player *player, double ignoreTime)
{
    assert(itemStack.good());
    return world.addEntity(itemStack.item.descriptor->getEntity(), position, velocity, lock_manager, itemStack.item.descriptor->getEntity()->makeItemDataIgnorePlayer(itemStack, world, player, ignoreTime));
}

Entity *ItemDescriptor::dropAsEntity(Item item, World &world, WorldLockManager &lock_manager, Player &player) const
{
    return player.createDroppedItemEntity(ItemStack(item), world, lock_manager);
}

linked_map<std::wstring, ItemDescriptorPointer> *ItemDescriptors_t::itemsMap = nullptr;

void ItemDescriptors_t::add(ItemDescriptorPointer bd) const
{
    assert(bd != nullptr);
    if(itemsMap == nullptr)
        itemsMap = new linked_map<std::wstring, ItemDescriptorPointer>();
    bool wasInserted = std::get<1>(itemsMap->insert(make_pair(bd->name, bd)));
    assert(wasInserted);
}

void ItemDescriptors_t::remove(ItemDescriptorPointer bd) const
{
    assert(bd != nullptr);
    assert(itemsMap != nullptr);
    size_t removedCount = itemsMap->erase(bd->name);
    assert(removedCount == 1);
    if(itemsMap->empty())
    {
        delete itemsMap;
        itemsMap = nullptr;
    }
}
}
}
