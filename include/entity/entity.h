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
#ifndef ENTITY_H_INCLUDED
#define ENTITY_H_INCLUDED

#include "util/linked_map.h"
#include "ray_casting/ray_casting.h"
#include <cwchar>
#include <memory>
#include <string>
#include <cstdint>
#include "entity/entity_struct.h"
#include "util/world_lock_manager.h"
#include "render/mesh.h"
#include "render/render_layer.h"
#include "stream/stream.h"
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
class PhysicsObject;
class PhysicsWorld;
class Player;
class World;

class EntityDescriptor
{
    EntityDescriptor(const EntityDescriptor &) = delete;
    void operator =(const EntityDescriptor &) = delete;
protected:
    explicit EntityDescriptor(std::wstring name);
public:
    virtual ~EntityDescriptor();
    const std::wstring name;

    virtual void moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const
    {
    }
    virtual void render(Entity &entity, Mesh &dest, RenderLayer rl, const Transform &cameraToWorldMatrix) const = 0;
    virtual RayCasting::Collision getRayCollision(Entity &entity, World &world, RayCasting::Ray ray) const
    {
        return RayCasting::Collision(world);
    }
    virtual Transform getSelectionBoxTransform(const Entity &entity) const = 0;
    virtual void makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const
    {
        entity.data = nullptr;
    }
    virtual bool onUse(Entity &entity, World &world, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const
    {
        return false;
    }
    virtual std::shared_ptr<PhysicsObject> makePhysicsObject(Entity &entity, PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> physicsWorld) const = 0;
    virtual void write(PositionF position, VectorF velocity, std::shared_ptr<void> data, stream::Writer &writer) const = 0;
    virtual std::shared_ptr<void> read(PositionF position, VectorF velocity, stream::Reader &reader) const = 0;
};

class EntityDescriptors_t final
{
    friend class EntityDescriptor;
private:
    typedef linked_map<std::wstring, EntityDescriptorPointer> MapType;
    static MapType *entitiesMap;
    void add(EntityDescriptorPointer bd) const;
    void remove(EntityDescriptorPointer bd) const;
public:
    constexpr EntityDescriptors_t()
    {
    }
    EntityDescriptorPointer operator [](std::wstring name) const
    {
        if(entitiesMap == nullptr)
            return nullptr;
        auto iter = entitiesMap->find(name);
        if(iter == entitiesMap->end())
            return nullptr;
        return std::get<1>(*iter);
    }
    EntityDescriptorPointer at(std::wstring name) const
    {
        EntityDescriptorPointer retval = operator [](name);
        if(retval == nullptr)
            throw std::out_of_range("EntityDescriptor not found");
        return retval;
    }
GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
    class iterator final : public std::iterator<std::forward_iterator_tag, const EntityDescriptorPointer>
    {
GCC_PRAGMA(diagnostic pop)
        friend class EntityDescriptors_t;
        MapType::const_iterator iter;
        bool empty;
        explicit iterator(MapType::const_iterator iter)
            : iter(iter), empty(false)
        {
        }
    public:
        iterator()
            : iter(), empty(true)
        {
        }
        bool operator ==(const iterator &r) const
        {
            if(empty)
                return r.empty;
            if(r.empty)
                return false;
            return iter == r.iter;
        }
        bool operator !=(const iterator &r) const
        {
            return !operator ==(r);
        }
        const EntityDescriptorPointer &operator *() const
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
            return iterator(iter++);
        }
    };
    iterator begin() const
    {
        if(entitiesMap == nullptr)
            return iterator();
        return iterator(entitiesMap->cbegin());
    }
    iterator end() const
    {
        if(entitiesMap == nullptr)
            return iterator();
        return iterator(entitiesMap->cend());
    }
    iterator cbegin() const
    {
        return begin();
    }
    iterator cend() const
    {
        return end();
    }
    iterator find(std::wstring name) const
    {
        if(entitiesMap == nullptr)
            return iterator();
        return iterator(entitiesMap->find(name));
    }
    size_t size() const
    {
        if(entitiesMap == nullptr)
            return 0;
        return entitiesMap->size();
    }
};

static constexpr EntityDescriptors_t EntityDescriptors;
}
}

#include "world/world.h"

#endif // ENTITY_H_INCLUDED
