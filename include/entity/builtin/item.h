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
private:
    const enum_array<Mesh, RenderLayer> meshes;
    const bool renderDynamic;
    const Matrix preorientSelectionBoxTransform;
    static constexpr float baseSize = 0.5f, extraHeight = 0.1f;
    static std::wstring makeName(ItemDescriptorPointer itemDescriptor)
    {
        return L"builtin.item(itemDescriptor=" + itemDescriptor->name + L")";
    }
public:
    const ItemDescriptorPointer itemDescriptor;
    EntityItem(ItemDescriptorPointer itemDescriptor, enum_array<Mesh, RenderLayer> meshes, Matrix preorientSelectionBoxTransform)
        : EntityDescriptor(makeName(itemDescriptor), PhysicsObjectConstructor::cylinderMaker(baseSize / 2, baseSize / 2 + extraHeight / 2, true, false, PhysicsProperties(PhysicsProperties::blockCollisionMask, PhysicsProperties::itemCollisionMask))),
                           meshes(meshes), renderDynamic(false),
                           preorientSelectionBoxTransform(preorientSelectionBoxTransform),
                           itemDescriptor(itemDescriptor)
    {
    }
    EntityItem(ItemDescriptorPointer itemDescriptor, Matrix preorientSelectionBoxTransform)
        : EntityDescriptor(makeName(itemDescriptor), PhysicsObjectConstructor::cylinderMaker(baseSize / 2, baseSize / 2 + extraHeight / 2, true, false, PhysicsProperties(PhysicsProperties::blockCollisionMask, PhysicsProperties::itemCollisionMask))),
                           meshes(), renderDynamic(true),
                           preorientSelectionBoxTransform(preorientSelectionBoxTransform),
                           itemDescriptor(itemDescriptor)
    {
    }
private:
    struct ItemData final
    {
        float angle = 0, bobPhase = 0;
        double timeLeft = 5 * 60;
        double ignorePlayerTime = 0;
        const Player *ignorePlayer = nullptr;
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
        ItemData(uint32_t seed, ItemStack itemStack)
            : itemStack(itemStack)
        {
            init(seed);
        }
        ItemData(uint32_t seed, ItemStack itemStack, const Player *ignorePlayer, double ignorePlayerTime)
            : ignorePlayerTime(ignorePlayerTime), ignorePlayer(ignorePlayer), itemStack(itemStack)
        {
            init(seed);
        }
    };
    static std::shared_ptr<ItemData> getItemData(const Entity &entity)
    {
        std::shared_ptr<ItemData> retval = std::static_pointer_cast<ItemData>(entity.data);
        assert(retval != nullptr);
        return retval;
    }
    static Matrix getTransform(const std::shared_ptr<ItemData> &data)
    {
        return Matrix::rotateY(data->angle).concat(Matrix::translate(0, extraHeight / 2 * std::sin(data->bobPhase), 0));
    }
    bool ignorePlayer(Entity &entity, const Player *player) const
    {
        std::shared_ptr<ItemData> data = getItemData(entity);
        if(data->ignorePlayerTime > 0 && data->ignorePlayer == player)
            return true;
        return false;
    }
public:
    virtual void render(Entity &entity, Mesh &dest, RenderLayer rl) const override
    {
        std::shared_ptr<ItemData> data = getItemData(entity);
        Matrix tform = getTransform(data).concat(Matrix::translate(entity.physicsObject->getPosition()));
        if(renderDynamic)
            itemDescriptor->entityRender(data->itemStack.item, tform, dest, rl);
        else
            dest.append(transform(tform, meshes[rl]));
    }
    virtual void moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const override;
    virtual Matrix getSelectionBoxTransform(const Entity &entity) const override
    {
        std::shared_ptr<ItemData> data = getItemData(entity);
        return preorientSelectionBoxTransform.concat(getTransform(data)).concat(Matrix::translate(entity.physicsObject->getPosition()));
    }
    virtual void makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const override
    {
        assert(getItemData(entity) != nullptr);
    }
    static std::shared_ptr<void> makeItemData(ItemStack itemStack, World &world)
    {
        return std::shared_ptr<void>(new ItemData(world.getRandomGenerator()(), itemStack));
    }
    static std::shared_ptr<void> makeItemDataIgnorePlayer(ItemStack itemStack, World &world, const Player *player, double ignoreTime = 1)
    {
        return std::shared_ptr<void>(new ItemData(world.getRandomGenerator()(), itemStack, player, ignoreTime));
    }
    static constexpr float dropSpeed = 3;
};
}
}
}
}

#endif // ENTITY_ITEM_H_INCLUDED
