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
#include "player/player.h"
#include "render/generate.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
void PlayerEntity::generateMeshes()
{
    head = (Mesh)transform(Matrix::translate(-0.5, -0.5, -0.5).concat(Matrix::scale(0.5)), Generate::unitBox(TextureAtlas::Player1HeadLeft.td(), TextureAtlas::Player1HeadRight.td(), TextureAtlas::Player1HeadBottom.td(), TextureAtlas::Player1HeadTop.td(), TextureAtlas::Player1HeadBack.td(), TextureAtlas::Player1HeadFront.td()));
    #warning add rest of player
}
void PlayerEntity::render(Entity &entity, Mesh &dest, RenderLayer rl) const
{
    #warning implement
}
void PlayerEntity::moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const
{
    std::shared_ptr<Player> player = std::static_pointer_cast<Player>(entity.data);
    if(player == nullptr)
    {
        entity.destroy();
        return;
    }
    player->lastPosition = entity.physicsObject->getPosition();
    VectorF moveDirection = transform(player->getWorldOrientationXZTransform(), player->gameInput->moveDirectionPlayerRelative.get());
    VectorF newVelocity = entity.physicsObject->getVelocity();
    newVelocity.x = moveDirection.x;
    newVelocity.z = moveDirection.z;
    if(player->gameInputMonitoring->retrieveGotJump() || player->gameInput->jump.get())
    {
        if(entity.physicsObject->isSupported())
            newVelocity.y += 5;
    }
    entity.physicsObject->setCurrentState(player->lastPosition, newVelocity);
    if(player->gameInput->isCreativeMode.get())
    {

    }
    #warning implement
}
void PlayerEntity::makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const
{
    std::shared_ptr<Player> player = std::static_pointer_cast<Player>(entity.data);
    if(player == nullptr)
    {
        entity.destroy();
        return;
    }
    player->lastPosition = entity.physicsObject->getPosition();
}
}
}
}
}
