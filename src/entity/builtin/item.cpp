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
#include "entity/builtin/item.h"
#include "player/player.h"
#include "physics/physics.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{

void Item::moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const
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
        entity.physicsObject = PhysicsObject::makeEmpty(position, VectorF(0), entity.physicsObject->getWorld());
    }
    if(data->followingPlayer && closestPlayer == nullptr)
    {
        entity.destroy();
        return;
    }
    if(data->followingPlayer && distanceSquared < 0.5 * 0.5)
    {
        closestPlayer->addItem();
        entity.destroy();
        return;
    }
    if(data->followingPlayer)
    {
        VectorF displacement = playerPosition - position;
        VectorF velocity = normalize(displacement) * std::min<float>(3, abs(displacement));
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
