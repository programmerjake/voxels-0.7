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

namespace programmerjake
{
namespace voxels
{
class Player;
namespace Entities
{
namespace builtin
{
class EntityItem : public EntityDescriptor
{
protected:
    enum_array<Mesh, RenderLayer> meshes;
    Matrix preorientSelectionBoxTransform;
    static constexpr float baseSize = 0.5f, extraHeight = 0.1f;
    EntityItem(std::wstring name, enum_array<Mesh, RenderLayer> meshes, Matrix preorientSelectionBoxTransform)
        : EntityDescriptor(name, PhysicsObjectConstructor::cylinderMaker(baseSize / 2, baseSize / 2 + extraHeight / 2, true, false, PhysicsProperties(PhysicsProperties::blockCollisionMask, PhysicsProperties::itemCollisionMask))), meshes(meshes), preorientSelectionBoxTransform(preorientSelectionBoxTransform)
    {
    }
    struct ItemData final
    {
        float angle = 0, bobPhase = 0;
        double timeLeft = 5 * 60;
        double ignorePlayerTime = 0;
        const Player *ignorePlayer = nullptr;
        std::int8_t count = 1;
        bool followingPlayer = false;
        void init()
        {
            std::minstd_rand generator((int)(std::intptr_t)this);
            generator.discard(1000);
            std::uniform_real_distribution<float> distribution(0, 2 * M_PI);
            angle = distribution(generator);
            bobPhase = distribution(generator);
        }
        ItemData()
        {
            init();
        }
        ItemData(const Player *ignorePlayer, double ignorePlayerTime)
            : ignorePlayerTime(ignorePlayerTime), ignorePlayer(ignorePlayer)
        {
            init();
        }
    };
    static std::shared_ptr<ItemData> getItemData(const Entity &entity)
    {
        std::shared_ptr<ItemData> retval = std::static_pointer_cast<ItemData>(entity.data);
        if(retval == nullptr)
        {
            retval = std::shared_ptr<ItemData>(new ItemData);
        }
        return retval;
    }
    static std::shared_ptr<ItemData> getOrMakeItemData(Entity &entity)
    {
        std::shared_ptr<ItemData> retval = std::static_pointer_cast<ItemData>(entity.data);
        if(retval == nullptr)
        {
            retval = std::shared_ptr<ItemData>(new ItemData);
            entity.data = std::static_pointer_cast<void>(retval);
        }
        return retval;
    }
    static Matrix getTransform(const std::shared_ptr<ItemData> &data)
    {
        return Matrix::rotateY(data->angle).concat(Matrix::translate(0, extraHeight / 2 * std::sin(data->bobPhase), 0));
    }
    virtual void onGiveToPlayer(Player &player) const = 0;
public:
    virtual void render(Entity &entity, Mesh &dest, RenderLayer rl) const override
    {
        std::shared_ptr<ItemData> data = getOrMakeItemData(entity);
        dest.append(transform(getTransform(data).concat(Matrix::translate(entity.physicsObject->getPosition())), meshes[rl]));
    }
    virtual void moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const override;
    virtual Matrix getSelectionBoxTransform(const Entity &entity) const override
    {
        std::shared_ptr<ItemData> data = getItemData(entity);
        return preorientSelectionBoxTransform.concat(getTransform(data)).concat(Matrix::translate(entity.physicsObject->getPosition()));
    }
    virtual void makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const override
    {
        getOrMakeItemData(entity);
    }
    virtual std::shared_ptr<void> makeItemDataIgnorePlayer(const Player *player, double ignoreTime = 1) const
    {
        return std::shared_ptr<void>(new ItemData(player, ignoreTime));
    }
    virtual bool ignorePlayer(Entity &entity, const Player *player) const
    {
        std::shared_ptr<ItemData> data = getOrMakeItemData(entity);
        if(data->ignorePlayerTime > 0 && data->ignorePlayer == player)
            return true;
        return false;
    }
    static constexpr float dropSpeed = 3;
};
}
}
}
}

#endif // ENTITY_ITEM_H_INCLUDED
