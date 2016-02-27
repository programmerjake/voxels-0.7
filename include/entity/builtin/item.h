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
#ifndef ENTITY_ITEM_H_INCLUDED
#define ENTITY_ITEM_H_INCLUDED

#include "entity/entity.h"
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <random>
#include "util/logging.h"
#include "util/math_constants.h"
#include "item/item.h"
#include "world/world.h"

namespace programmerjake
{
namespace voxels
{
class Player;
namespace Entities
{
namespace builtin
{
class EntityItem final : public EntityDescriptor
{
    EntityItem(const EntityItem &) = delete;
    EntityItem &operator=(const EntityItem &) = delete;

private:
    const enum_array<Mesh, RenderLayer> meshes;
    const bool renderDynamic;
    const Transform preorientSelectionBoxTransform;
    static constexpr float baseSize = 0.5f, extraHeight = 0.1f;
    static std::wstring makeName(ItemDescriptorPointer itemDescriptor)
    {
        return L"builtin.item(itemDescriptor=" + itemDescriptor->name + L")";
    }

public:
    const ItemDescriptorPointer itemDescriptor;
    EntityItem(ItemDescriptorPointer itemDescriptor,
               enum_array<Mesh, RenderLayer> meshes,
               Transform preorientSelectionBoxTransform)
        : EntityDescriptor(makeName(itemDescriptor)),
          meshes(meshes),
          renderDynamic(false),
          preorientSelectionBoxTransform(preorientSelectionBoxTransform),
          itemDescriptor(itemDescriptor)
    {
    }
    EntityItem(ItemDescriptorPointer itemDescriptor, Transform preorientSelectionBoxTransform)
        : EntityDescriptor(makeName(itemDescriptor)),
          meshes(),
          renderDynamic(true),
          preorientSelectionBoxTransform(preorientSelectionBoxTransform),
          itemDescriptor(itemDescriptor)
    {
    }
    virtual std::shared_ptr<PhysicsObject> makePhysicsObject(
        Entity &entity,
        PositionF position,
        VectorF velocity,
        std::shared_ptr<PhysicsWorld> physicsWorld) const override;

private:
    struct ItemData final
    {
        ItemData(const ItemData &) = default;
        ItemData(ItemData &&) = default;
        ItemData &operator=(const ItemData &) = delete;
        float angle = 0, bobPhase = 0;
        double timeLeft = 5 * 60;
        double ignorePlayerTime = 0;
        std::weak_ptr<Player> ignorePlayer = std::weak_ptr<Player>();
        bool followingPlayer = false;
        ItemStack itemStack;
        void init(uint32_t seed)
        {
            std::minstd_rand generator(seed);
            generator.discard(10);
            std::uniform_real_distribution<float> distribution(0, 2 * M_PI);
            angle = distribution(generator);
            bobPhase = distribution(generator);
        }
        ItemData(uint32_t seed, ItemStack itemStack) : itemStack(itemStack)
        {
            init(seed);
        }
        ItemData(uint32_t seed,
                 ItemStack itemStack,
                 std::weak_ptr<Player> ignorePlayer,
                 double ignorePlayerTime)
            : ignorePlayerTime(ignorePlayerTime), ignorePlayer(ignorePlayer), itemStack(itemStack)
        {
            init(seed);
        }
        ItemData(float angle,
                 float bobPhase,
                 double timeLeft,
                 double ignorePlayerTime,
                 std::weak_ptr<Player> ignorePlayer,
                 bool followingPlayer,
                 ItemStack itemStack)
            : angle(angle),
              bobPhase(bobPhase),
              timeLeft(timeLeft),
              ignorePlayerTime(ignorePlayerTime),
              ignorePlayer(ignorePlayer),
              followingPlayer(followingPlayer),
              itemStack(itemStack)
        {
        }
        void write(stream::Writer &writer) const;
        static ItemData read(stream::Reader &reader);
    };
    static std::shared_ptr<ItemData> getItemData(const Entity &entity)
    {
        return getItemData(entity.data);
    }
    static std::shared_ptr<ItemData> getItemData(std::shared_ptr<void> data)
    {
        std::shared_ptr<ItemData> retval = std::static_pointer_cast<ItemData>(data);
        assert(retval != nullptr);
        return retval;
    }
    static Transform getTransform(const std::shared_ptr<ItemData> &data)
    {
        return Transform::rotateY(data->angle)
            .concat(Transform::translate(0, extraHeight / 2 * std::sin(data->bobPhase), 0));
    }
    bool ignorePlayer(Entity &entity, std::shared_ptr<Player> player) const
    {
        std::shared_ptr<ItemData> data = getItemData(entity);
        if(data->ignorePlayerTime > 0 && data->ignorePlayer.lock() == player)
            return true;
        return false;
    }

public:
    virtual void render(Entity &entity,
                        Mesh &dest,
                        RenderLayer rl,
                        const Transform &cameraToWorldMatrix) const override
    {
        std::shared_ptr<ItemData> data = getItemData(entity);
        Transform tform =
            getTransform(data).concat(Transform::translate(entity.physicsObject->getPosition()));
        if(renderDynamic)
            itemDescriptor->entityRender(data->itemStack.item, tform, dest, rl);
        else
            dest.append(transform(tform, meshes[rl]));
    }
    virtual void moveStep(Entity &entity,
                          World &world,
                          WorldLockManager &lock_manager,
                          double deltaTime) const override;
    virtual Transform getSelectionBoxTransform(const Entity &entity) const override
    {
        std::shared_ptr<ItemData> data = getItemData(entity);
        return preorientSelectionBoxTransform.concat(getTransform(data))
            .concat(Transform::translate(entity.physicsObject->getPosition()));
    }
    virtual void makeData(Entity &entity,
                          World &world,
                          WorldLockManager &lock_manager) const override
    {
        assert(getItemData(entity) != nullptr);
    }
    static std::shared_ptr<void> makeItemData(ItemStack itemStack, World &world)
    {
        return std::shared_ptr<void>(new ItemData(world.getRandomGenerator()(), itemStack));
    }
    static std::shared_ptr<void> makeItemDataIgnorePlayer(ItemStack itemStack,
                                                          World &world,
                                                          std::weak_ptr<Player> player,
                                                          double ignoreTime = 1)
    {
        return std::shared_ptr<void>(
            new ItemData(world.getRandomGenerator()(), itemStack, player, ignoreTime));
    }
    static constexpr float dropSpeed = 3;
    virtual void write(PositionF position,
                       VectorF velocity,
                       std::shared_ptr<void> data,
                       stream::Writer &writer) const override
    {
        ItemData itemData = *getItemData(data);
        stream::write<ItemData>(writer, itemData);
    }
    virtual std::shared_ptr<void> read(PositionF position,
                                       VectorF velocity,
                                       stream::Reader &reader) const override
    {
        ItemData itemData = stream::read<ItemData>(reader);
        return std::shared_ptr<void>(new ItemData(itemData));
    }
};
}
}
}
}

#endif // ENTITY_ITEM_H_INCLUDED
