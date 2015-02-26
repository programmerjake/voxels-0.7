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

void EntityItem::moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const
{
    std::shared_ptr<ItemData> data = getOrMakeItemData(entity);
    constexpr float angleSpeed = 2 * M_PI / 7.5f;
    constexpr float bobSpeed = 2.1f * angleSpeed;
    data->angle = std::fmod(data->angle + angleSpeed * (float)deltaTime, M_PI * 2);
    data->bobPhase = std::fmod(data->bobPhase + bobSpeed * (float)deltaTime, M_PI * 2);
    data->ignorePlayerTime = std::max<double>(0, data->ignorePlayerTime - deltaTime);
    PositionF position = entity.physicsObject->getPosition();
    Player *closestPlayer = nullptr;
    float distanceSquared = 0;
    PositionF playerPosition;
    for(Player &player : Players.lock())
    {
        PositionF currentPlayerPosition = player.getPosition();
        if(currentPlayerPosition.d != position.d)
            continue;
        if(ignorePlayer(entity, &player))
            continue;
        if(closestPlayer == nullptr || absSquared(currentPlayerPosition - position) < distanceSquared)
        {
            closestPlayer = &player;
            playerPosition = currentPlayerPosition;
            distanceSquared = absSquared(currentPlayerPosition - position);
        }
    }
    if(!data->followingPlayer && closestPlayer != nullptr && distanceSquared < 2 * 2)
    {
        data->followingPlayer = true;
        auto oldPhysicsObject = entity.physicsObject;
        entity.physicsObject = PhysicsObject::makeEmpty(position, VectorF(0), entity.physicsObject->getWorld());
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
            onGiveToPlayer(*closestPlayer);
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
        //getDebugLog() << L"Entity " << (void *)&entity << L": pos:" << (VectorF)entity.physicsObject->getPosition() << postnl;
    }
}

}
}
}
}
