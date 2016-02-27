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
#include "item/item.h"
#include "entity/builtin/item.h"
#include "player/player.h"
#include <cassert>

namespace programmerjake
{
namespace voxels
{
ItemDescriptor::ItemDescriptor(std::wstring name, Transform entityPreorientSelectionBoxTransform)
    : name(name), entity()
{
    entity = new Entities::builtin::EntityItem(this, entityPreorientSelectionBoxTransform);
    ItemDescriptors.add(this);
}

ItemDescriptor::ItemDescriptor(std::wstring name,
                               enum_array<Mesh, RenderLayer> entityMeshes,
                               Transform entityPreorientSelectionBoxTransform)
    : name(name), entity()
{
    entity =
        new Entities::builtin::EntityItem(this, entityMeshes, entityPreorientSelectionBoxTransform);
    ItemDescriptors.add(this);
}

ItemDescriptor::~ItemDescriptor()
{
    ItemDescriptors.remove(this);
    delete entity;
}

Entity *ItemDescriptor::addToWorld(World &world,
                                   WorldLockManager &lock_manager,
                                   ItemStack itemStack,
                                   PositionF position,
                                   VectorF velocity)
{
    assert(itemStack.good());
    return world.addEntity(itemStack.item.descriptor->getEntity(),
                           position,
                           velocity,
                           lock_manager,
                           itemStack.item.descriptor->getEntity()->makeItemData(itemStack, world));
}

Entity *ItemDescriptor::addToWorld(World &world,
                                   WorldLockManager &lock_manager,
                                   ItemStack itemStack,
                                   PositionF position,
                                   VectorF velocity,
                                   std::weak_ptr<Player> player,
                                   double ignoreTime)
{
    assert(itemStack.good());
    return world.addEntity(itemStack.item.descriptor->getEntity(),
                           position,
                           velocity,
                           lock_manager,
                           itemStack.item.descriptor->getEntity()->makeItemDataIgnorePlayer(
                               itemStack, world, player, ignoreTime));
}

Entity *ItemDescriptor::dropAsEntity(Item item,
                                     World &world,
                                     WorldLockManager &lock_manager,
                                     Player &player) const
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
    ignore_unused_variable_warning(wasInserted);
    assert(wasInserted);
}

void ItemDescriptors_t::remove(ItemDescriptorPointer bd) const
{
    assert(bd != nullptr);
    assert(itemsMap != nullptr);
    size_t removedCount = itemsMap->erase(bd->name);
    ignore_unused_variable_warning(removedCount);
    assert(removedCount == 1);
    if(itemsMap->empty())
    {
        delete itemsMap;
        itemsMap = nullptr;
    }
}

namespace
{
struct StreamItemDescriptors final
{
    typedef std::uint16_t Descriptor;
    static constexpr Descriptor NullDescriptor = 0;
    std::unordered_map<Descriptor, ItemDescriptorPointer> descriptorToItemDescriptorPointerMap;
    std::unordered_map<ItemDescriptorPointer, Descriptor> itemDescriptorPointerToDescriptorMap;
    std::size_t descriptorCount = 0;
    StreamItemDescriptors()
        : descriptorToItemDescriptorPointerMap(), itemDescriptorPointerToDescriptorMap()
    {
    }
    static StreamItemDescriptors &get(stream::Stream &stream)
    {
        struct tag_t
        {
        };
        std::shared_ptr<StreamItemDescriptors> retval =
            stream.getAssociatedValue<StreamItemDescriptors, tag_t>();
        if(retval)
            return *retval;
        retval = std::make_shared<StreamItemDescriptors>();
        stream.setAssociatedValue<StreamItemDescriptors, tag_t>(retval);
        return *retval;
    }
    static ItemDescriptorPointer read(stream::Reader &reader)
    {
        StreamItemDescriptors &me = get(reader);
        Descriptor upperLimit = static_cast<Descriptor>(me.descriptorCount + 1);
        if(upperLimit != me.descriptorCount + 1)
            upperLimit = static_cast<Descriptor>(me.descriptorCount);
        Descriptor descriptor =
            stream::read_limited<Descriptor>(reader, NullDescriptor, upperLimit);
        if(descriptor == NullDescriptor)
            return nullptr;
        if(descriptor <= me.descriptorCount)
            return me.descriptorToItemDescriptorPointerMap[descriptor];
        assert(descriptor == me.descriptorCount + 1); // check for overflow
        me.descriptorCount++;
        std::wstring name = stream::read<std::wstring>(reader);
        ItemDescriptorPointer retval = ItemDescriptors[name];
        if(retval == nullptr)
            throw stream::InvalidDataValueException("item name not found");
        me.descriptorToItemDescriptorPointerMap[descriptor] = retval;
        me.itemDescriptorPointerToDescriptorMap[retval] = descriptor;
        return retval;
    }
    static void write(stream::Writer &writer, ItemDescriptorPointer id)
    {
        if(id == nullptr)
        {
            stream::write<Descriptor>(writer, NullDescriptor);
            return;
        }
        StreamItemDescriptors &me = get(writer);
        auto iter = me.itemDescriptorPointerToDescriptorMap.find(id);
        if(iter == me.itemDescriptorPointerToDescriptorMap.end())
        {
            Descriptor descriptor = static_cast<Descriptor>(++me.descriptorCount);
            assert(descriptor == me.descriptorCount); // check for overflow
            me.itemDescriptorPointerToDescriptorMap[id] = descriptor;
            me.descriptorToItemDescriptorPointerMap[descriptor] = id;
            stream::write<Descriptor>(writer, descriptor);
            stream::write<std::wstring>(writer, id->name);
        }
        else
        {
            stream::write<Descriptor>(writer, std::get<1>(*iter));
        }
    }
};
}

Item Item::read(stream::Reader &reader)
{
    ItemDescriptorPointer descriptor = StreamItemDescriptors::read(reader);
    if(descriptor == nullptr)
        return Item();
    std::shared_ptr<void> data = descriptor->readItemData(reader);
    return Item(descriptor, data);
}

void Item::write(stream::Writer &writer) const
{
    if(!good())
    {
        StreamItemDescriptors::write(writer, nullptr);
        return;
    }
    StreamItemDescriptors::write(writer, descriptor);
    descriptor->writeItemData(writer, data);
}
}
}
