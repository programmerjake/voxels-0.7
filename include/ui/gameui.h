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
#ifndef GAMEUI_H_INCLUDED
#define GAMEUI_H_INCLUDED

#include "ui/ui.h"
#include "input/input.h"
#include "player/player.h"
#include "platform/platform.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class GameUi : public Ui
{
private:
    World &world;
    WorldLockManager &lock_manager;
    std::shared_ptr<ViewPoint> viewPoint;
    std::shared_ptr<GameInput> gameInput;
    std::shared_ptr<Player> player;
    const Entity *playerEntity;
    bool isDialogUp = false;
    bool isWDown = false;
    bool isADown = false;
    bool isSDown = false;
    bool isDDown = false;
    void calculateMoveDirection()
    {
        VectorF v = VectorF(0);
        VectorF forward(0, 0, -1);
        VectorF left(-1, 0, 0);
        if(isWDown)
            v += forward;
        if(isSDown)
            v -= forward;
        if(isADown)
            v += left;
        if(isDDown)
            v -= left;
        if(gameInput->paused.get())
            v = VectorF(0);
        gameInput->moveDirectionPlayerRelative.set(v * 3.5f);
    }
public:
    GameUi(Renderer &renderer, World &world, WorldLockManager &lock_manager)
        : world(world), lock_manager(lock_manager), gameInput(std::make_shared<GameInput>())
    {
        PositionF startingPosition = PositionF(0.5f, World::AverageGroundHeight + 8.5f, 0.5f, Dimension::Overworld);
        viewPoint = std::make_shared<ViewPoint>(world, startingPosition, 32);
        player = std::make_shared<Player>(L"default-player-name", gameInput);
        playerEntity = world.addEntity(Entities::builtin::PlayerEntity::descriptor(), startingPosition, VectorF(0), lock_manager, std::static_pointer_cast<void>(player));
    }
    virtual void move(double deltaTime) override
    {
        Display::grabMouse(!isDialogUp && !gameInput->paused.get());
        Ui::move(deltaTime);
        world.move(deltaTime, lock_manager);
    }
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        if(Ui::handleTouchUp(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        if(Ui::handleTouchDown(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        if(Ui::handleTouchMove(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        if(Ui::handleMouseUp(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        if(Ui::handleMouseDown(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        if(!isDialogUp && !gameInput->paused.get())
        {
            float viewTheta = gameInput->viewTheta.get();
            float viewPhi = gameInput->viewPhi.get();
            VectorF deltaPos = Display::transformMouseTo3D(event.x + event.deltaX, event.y + event.deltaY) - Display::transformMouseTo3D(event.x, event.y);
            viewTheta -= deltaPos.x;
            viewPhi += deltaPos.y;
            viewTheta = std::fmod(viewTheta, 2 * M_PI);
            viewPhi = limit<float>(viewPhi, -M_PI / 2, M_PI / 2);
            gameInput->viewTheta.set(viewTheta);
            gameInput->viewPhi.set(viewPhi);
        }
        if(Ui::handleMouseMove(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event) override
    {
        if(Ui::handleMouseScroll(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent &event) override
    {
        if(Ui::handleKeyUp(event))
            return true;
        if(event.key == KeyboardKey::Space)
        {
            gameInput->jump.set(false);
            return true;
        }
        if(event.key == KeyboardKey::W)
        {
            isWDown = false;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::A)
        {
            isADown = false;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::S)
        {
            isSDown = false;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::D)
        {
            isDDown = false;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::Escape)
        {
            return true;
        }
        return false;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        if(Ui::handleKeyDown(event))
            return true;
        if(event.key == KeyboardKey::Space)
        {
            gameInput->jump.set(true);
            return true;
        }
        if(event.key == KeyboardKey::W)
        {
            isWDown = true;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::A)
        {
            isADown = true;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::S)
        {
            isSDown = true;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::D)
        {
            isDDown = true;
            calculateMoveDirection();
            return true;
        }
        if(event.key == KeyboardKey::Escape)
        {
            gameInput->paused.set(!gameInput->paused.get());
            return true;
        }
        return false;
    }
    virtual bool handleKeyPress(KeyPressEvent &event) override
    {
        if(Ui::handleKeyPress(event))
            return true;
        #warning implement
        return false;
    }
    virtual bool handleMouseMoveOut(MouseEvent &event) override
    {
        if(Ui::handleMouseMoveOut(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseMoveIn(MouseEvent &event) override
    {
        if(Ui::handleMouseMoveIn(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchMoveOut(TouchEvent &event) override
    {
        if(Ui::handleTouchMoveOut(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchMoveIn(TouchEvent &event) override
    {
        if(Ui::handleTouchMoveIn(event))
            return true;
        #warning implement
        return true;
    }
protected:
    virtual void clear(Renderer &renderer) override;
};
}
}
}

#endif // GAMEUI_H_INCLUDED
