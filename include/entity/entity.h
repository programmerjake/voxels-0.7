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
#ifndef ENTITY_H_INCLUDED
#define ENTITY_H_INCLUDED

#include "physics/physics.h"
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
        return descriptor != nullptr;
    }
    explicit operator bool() const
    {
        return good();
    }
    bool operator !() const
    {
        return !good();
    }
    void destroy()
    {
        if(physicsObject)
            physicsObject->destroy();
        data = nullptr;
    }
};

class World;

class EntityDescriptor
{
    EntityDescriptor(const EntityDescriptor &) = delete;
    void operator =(const EntityDescriptor &) = delete;
protected:
    EntityDescriptor(std::wstring name, std::shared_ptr<const PhysicsObjectConstructor> physicsObjectConstructor);
public:
    virtual ~EntityDescriptor();
    const std::wstring name;
    const std::shared_ptr<const PhysicsObjectConstructor> physicsObjectConstructor;

    virtual void moveStep(Entity &entity, World &world, double deltaTime) const
    {
    }
    virtual void render(const Entity &entity, Mesh &dest, RenderLayer rl) const = 0;
    virtual RayCasting::Collision getRayCollision(Entity &entity, World &world, RayCasting::Ray ray) const
    {
        return RayCasting::Collision(world);
    }
    virtual Matrix getSelectionBoxTransform(const Entity &entity) const = 0;
    virtual void makeData(Entity &entity, World &world) const
    {
        entity.data = nullptr;
    }
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
    class iterator final : public std::iterator<std::forward_iterator_tag, const EntityDescriptorPointer>
    {
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
