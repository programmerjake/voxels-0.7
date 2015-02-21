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

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class GameUi : public Ui
{
public:
    GameInput gameInput;
private:
    float viewAngle = 0;
    float lastThrowBlockTime = 10;
    World &world;
    WorldLockManager &lock_manager;
    std::shared_ptr<ViewPoint> viewPoint;
    PositionI origin()
    {
        return PositionI(0, World::AverageGroundHeight + 8, 0, Dimension::Overworld);
    }
public:
    GameUi(Renderer &renderer, World &world, WorldLockManager &lock_manager)
        : world(world), lock_manager(lock_manager)
    {
        viewPoint = make_shared<ViewPoint>(world, origin(), 16);
    }
    virtual void move(double deltaTime) override
    {
        viewAngle = std::fmod(viewAngle + deltaTime * 2 * M_PI / 50, 2 * M_PI);
        Ui::move(deltaTime);
        lastThrowBlockTime -= deltaTime;
        while(lastThrowBlockTime < 0)
        {
            lastThrowBlockTime += 1;
            world.addEntity(Entities::builtin::items::Stone::descriptor(), origin() + VectorF(0.5, 0.5 - 1, 0.5), VectorF(0), render_thread_lock_manager);
        }
        world.move(deltaTime, lock_manager);
    }
protected:
    virtual void clear(Renderer &renderer) override
    {
        background = HSVF(0.6, 0.5, 1);
        Ui::clear(renderer);
        Matrix tform = Matrix::translate(-((VectorF)origin() + VectorF(0.5, 0.5, 0.5))).concat(Matrix::rotateY(viewAngle)).concat(Matrix::rotateX(0.25 * M_PI));
        viewPoint->setPosition(origin());
        viewPoint->render(renderer, tform, lock_manager);
        renderer << start_overlay << enable_depth_buffer;
    }
};
}
}
}

#endif // GAMEUI_H_INCLUDED
