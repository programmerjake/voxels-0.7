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
#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "stream/stream.h"
#include "world/world.h"
#include "entity/entity.h"
#include "physics/physics.h"
#include "util/global_instance_maker.h"
#include "input/input.h"
#include <string>
#include <cassert>
#include <memory>
#include <atomic>

namespace programmerjake
{
namespace voxels
{
class Player;

namespace Entities
{
namespace builtin
{
class PlayerEntity final : public EntityDescriptor
{
    friend class global_instance_maker<PlayerEntity>;
private:
    Mesh head;
    Mesh body;
    Mesh leftArm;
    Mesh rightArm;
    Mesh leftLeg;
    Mesh rightLeg;
    void generateMeshes();
    PlayerEntity()
        : EntityDescriptor(L"builtin.player", PhysicsObjectConstructor::cylinderMaker(0.3, 0.9, true, false, PhysicsProperties(PhysicsProperties::defaultCollisionMask, PhysicsProperties::defaultCollisionMask, 0, 1)))
    {
        generateMeshes();
    }
    Player *getPlayer(Entity &entity) const
    {
        return static_cast<Player *>(entity.data.get());
    }
public:
    static const PlayerEntity *descriptor()
    {
        return global_instance_maker<PlayerEntity>::getInstance();
    }
    virtual void render(Entity &entity, Mesh &dest, RenderLayer rl) const override;
    virtual void moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const override;
    virtual Matrix getSelectionBoxTransform(const Entity &entity) const override
    {
        return Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.25, 1.8, 0.25)).concat(Matrix::translate(entity.physicsObject->getPosition()));
    }
    virtual void makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const override;
};
}
}

class Player final
{
    friend class Entities::builtin::PlayerEntity;
private:
    PositionF lastPosition;
    std::shared_ptr<GameInput> gameInput;
    struct GameInputMonitoring
    {
        std::atomic_bool gotJump;
        GameInputMonitoring()
            : gotJump(false)
        {
        }
        bool retrieveGotJump()
        {
            return gotJump.exchange(false);
        }
    };
    std::shared_ptr<GameInputMonitoring> gameInputMonitoring;
public:
    const std::wstring name;
    Player(std::wstring name, std::shared_ptr<GameInput> gameInput)
        : gameInput(gameInput), name(name)
    {
        assert(gameInput != nullptr);
        gameInputMonitoring = std::make_shared<GameInputMonitoring>();
        gameInput->jump.onChange.bind([this](EventArguments&)
        {
            if(this->gameInput->jump.get())
                gameInputMonitoring->gotJump = true;
            return Event::ReturnType::Propagate;
        });
    }
    Matrix getWorldOrientationXZTransform() const
    {
        return Matrix::rotateY(gameInput->viewTheta.get());
    }
    Matrix getWorldOrientationTransform() const
    {
        return Matrix::rotateX(gameInput->viewPhi.get()).concat(Matrix::rotateY(gameInput->viewTheta.get()));
    }
    Matrix getViewTransform() const
    {
        VectorF pos = lastPosition + VectorF(0, 0.6, 0);
        return Matrix::translate(-pos).concat(Matrix::rotateY(-gameInput->viewTheta.get())).concat(Matrix::rotateX(-gameInput->viewPhi.get()));
    }
    PositionF getPosition() const
    {
        return lastPosition;
    }
};
}
}

#endif // PLAYER_H_INCLUDED
