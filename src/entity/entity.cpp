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
#include "entity/entity.h"
#include "physics/physics.h"
#include "util/util.h"

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

namespace
{
struct StreamEntityDescriptors final
{
    typedef std::uint16_t Descriptor;
    static constexpr Descriptor NullDescriptor = 0;
    std::unordered_map<Descriptor, EntityDescriptorPointer> descriptorToEntityDescriptorPointerMap;
    std::unordered_map<EntityDescriptorPointer, Descriptor> entityDescriptorPointerToDescriptorMap;
    std::size_t descriptorCount = 0;
    StreamEntityDescriptors()
        : descriptorToEntityDescriptorPointerMap(), entityDescriptorPointerToDescriptorMap()
    {
    }
    static StreamEntityDescriptors &get(stream::Stream &stream)
    {
        struct tag_t
        {
        };
        std::shared_ptr<StreamEntityDescriptors> retval =
            stream.getAssociatedValue<StreamEntityDescriptors, tag_t>();
        if(retval)
            return *retval;
        retval = std::make_shared<StreamEntityDescriptors>();
        stream.setAssociatedValue<StreamEntityDescriptors, tag_t>(retval);
        return *retval;
    }
    static EntityDescriptorPointer read(stream::Reader &reader)
    {
        StreamEntityDescriptors &me = get(reader);
        Descriptor upperLimit = static_cast<Descriptor>(me.descriptorCount + 1);
        if(upperLimit != me.descriptorCount + 1)
            upperLimit = static_cast<Descriptor>(me.descriptorCount);
        Descriptor descriptor =
            stream::read_limited<Descriptor>(reader, NullDescriptor, upperLimit);
        if(descriptor == NullDescriptor)
            return nullptr;
        if(descriptor <= me.descriptorCount)
            return me.descriptorToEntityDescriptorPointerMap[descriptor];
        assert(descriptor == me.descriptorCount + 1); // check for overflow
        me.descriptorCount++;
        std::wstring name = stream::read<std::wstring>(reader);
        EntityDescriptorPointer retval = EntityDescriptors[name];
        if(retval == nullptr)
            throw stream::InvalidDataValueException("entity name not found");
        me.descriptorToEntityDescriptorPointerMap[descriptor] = retval;
        me.entityDescriptorPointerToDescriptorMap[retval] = descriptor;
        return retval;
    }
    static void write(stream::Writer &writer, EntityDescriptorPointer ed)
    {
        if(ed == nullptr)
        {
            stream::write<Descriptor>(writer, NullDescriptor);
            return;
        }
        StreamEntityDescriptors &me = get(writer);
        auto iter = me.entityDescriptorPointerToDescriptorMap.find(ed);
        if(iter == me.entityDescriptorPointerToDescriptorMap.end())
        {
            Descriptor descriptor = static_cast<Descriptor>(++me.descriptorCount);
            assert(descriptor == me.descriptorCount); // check for overflow
            me.entityDescriptorPointerToDescriptorMap[ed] = descriptor;
            me.descriptorToEntityDescriptorPointerMap[descriptor] = ed;
            stream::write<Descriptor>(writer, descriptor);
            stream::write<std::wstring>(writer, ed->name);
        }
        else
        {
            stream::write<Descriptor>(writer, std::get<1>(*iter));
        }
    }
};
}

EntityDescriptorPointer Entity::readDescriptor(stream::Reader &reader)
{
    return StreamEntityDescriptors::read(reader);
}

void Entity::writeDescriptor(stream::Writer &writer, EntityDescriptorPointer ed)
{
    StreamEntityDescriptors::write(writer, ed);
}

void Entity::write(stream::Writer &writer) const
{
    if(!good())
    {
        writeDescriptor(writer, nullptr);
        return;
    }
    writeDescriptor(writer, descriptor);
    PositionF position = physicsObject->getPosition();
    VectorF velocity = physicsObject->getVelocity();
    stream::write<PositionF>(writer, position);
    stream::write<VectorF>(writer, velocity);
    descriptor->write(position, velocity, data, writer);
}

void Entity::readInternal(stream::Reader &reader, PositionF &position, VectorF &velocity)
{
    assert(descriptor);
    position = stream::read<PositionF>(reader);
    velocity = stream::read<VectorF>(reader);
    data = descriptor->read(position, velocity, reader);
}

EntityDescriptor::EntityDescriptor(wstring name) : name(name)
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
    ignore_unused_variable_warning(wasInserted);
    assert(wasInserted);
}

void EntityDescriptors_t::remove(EntityDescriptorPointer bd) const
{
    assert(bd != nullptr);
    assert(entitiesMap != nullptr);
    size_t removedCount = entitiesMap->erase(bd->name);
    ignore_unused_variable_warning(removedCount);
    assert(removedCount == 1);
    if(entitiesMap->empty())
    {
        delete entitiesMap;
        entitiesMap = nullptr;
    }
}
}
}
