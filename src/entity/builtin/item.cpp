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
#include "entity/builtin/item.h"
#include "player/player.h"
#include "physics/physics.h"
#include "util/math_constants.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
void EntityItem::ItemData::write(stream::Writer &writer) const
{
    stream::write<float32_t>(writer, angle);
    stream::write<float32_t>(writer, bobPhase);
    stream::write<float64_t>(writer, timeLeft);
    stream::write<float64_t>(writer, ignorePlayerTime);
    Player::writeReference(writer, ignorePlayer.lock());
    stream::write<bool>(writer, followingPlayer);
    stream::write<ItemStack>(writer, itemStack);
}

EntityItem::ItemData EntityItem::ItemData::read(stream::Reader &reader)
{
    float angle = stream::read<float32_t>(reader);
    float bobPhase = stream::read<float32_t>(reader);
    double timeLeft = stream::read<float64_t>(reader);
    double ignorePlayerTime = stream::read<float64_t>(reader);
    std::weak_ptr<Player> ignorePlayer = Player::readReference(reader);
    bool followingPlayer = stream::read<bool>(reader);
    ItemStack itemStack = stream::read<ItemStack>(reader);
    return ItemData(
        angle, bobPhase, timeLeft, ignorePlayerTime, ignorePlayer, followingPlayer, itemStack);
}

std::shared_ptr<PhysicsObject> EntityItem::makePhysicsObject(
    Entity &entity,
    PositionF position,
    VectorF velocity,
    std::shared_ptr<PhysicsWorld> physicsWorld) const
{
    std::shared_ptr<ItemData> data = getItemData(entity);
    assert(data);
    if(data->followingPlayer)
    {
        return PhysicsObject::makeEmpty(position, velocity, physicsWorld);
    }
    PhysicsProperties pp = PhysicsProperties(PhysicsProperties::blockCollisionMask,
                                             PhysicsProperties::itemCollisionMask);
    pp.gravity = VectorF(0, -16, 0);
    return PhysicsObject::makeCylinder(position,
                                       velocity,
                                       true,
                                       false,
                                       baseSize / 2,
                                       baseSize / 2 + extraHeight / 2,
                                       pp,
                                       physicsWorld);
}


void EntityItem::moveStep(Entity &entity,
                          World &world,
                          WorldLockManager &lock_manager,
                          double deltaTime) const
{
    std::shared_ptr<ItemData> data = getItemData(entity);
    constexpr float angleSpeed = 2 * M_PI / 7.5f;
    constexpr float bobSpeed = 2.1f * angleSpeed;
    data->angle = std::fmod(data->angle + angleSpeed * (float)deltaTime, (float)M_PI * 2);
    data->bobPhase = std::fmod(data->bobPhase + bobSpeed * (float)deltaTime, (float)M_PI * 2);
    data->ignorePlayerTime = std::max<double>(0, data->ignorePlayerTime - deltaTime);
    PositionF position = entity.physicsObject->getPosition();
    std::shared_ptr<Player> closestPlayer = nullptr;
    float distanceSquared = 0;
    PositionF playerPosition;
    for(std::shared_ptr<Player> player : world.players().lock())
    {
        PositionF currentPlayerPosition = player->getPosition();
        if(currentPlayerPosition.d != position.d)
            continue;
        if(ignorePlayer(entity, player))
            continue;
        if(closestPlayer == nullptr
           || absSquared(currentPlayerPosition - position) < distanceSquared)
        {
            closestPlayer = player;
            playerPosition = currentPlayerPosition;
            distanceSquared = absSquared(currentPlayerPosition - position);
        }
    }
    if(!data->followingPlayer && closestPlayer != nullptr && distanceSquared < 2 * 2)
    {
        data->followingPlayer = true;
        auto oldPhysicsObject = entity.physicsObject;
        entity.physicsObject =
            PhysicsObject::makeEmpty(position, VectorF(0), entity.physicsObject->getWorld());
        oldPhysicsObject->destroy();
    }
    if(data->followingPlayer && closestPlayer == nullptr)
    {
        entity.destroy();
        return;
    }
    if(data->followingPlayer)
    {
        VectorF displacement = playerPosition - position;
        float speed = std::max<float>(1, absSquared(displacement)) * 3;
        VectorF velocity = normalize(displacement) * speed;
        if(abs(displacement) < deltaTime * speed)
        {
            for(unsigned i = 0; i < data->itemStack.count; i++)
            {
                closestPlayer->addItem(data->itemStack.item);
                FIXME_MESSAGE(check for player not accepting item)
            }
            entity.destroy();
            return;
        }
        entity.physicsObject->setCurrentState(position, velocity);
        return;
    }
    data->timeLeft -= deltaTime;
    if(data->timeLeft <= 0 || position.y < -64)
    {
        entity.destroy();
        return;
    }
    else
    {
        VectorF gravity = VectorF(0, -16, 0);
        if(entity.physicsObject->getBlockEffects().canSwim)
        {
            gravity = VectorF(0, 1, 0);
        }
        entity.physicsObject->setGravity(gravity);
        // getDebugLog() << L"Entity " << (void *)&entity << L": pos:" <<
        // (VectorF)entity.physicsObject->getPosition() << postnl;
    }
}
}
}
}
}
