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
#include "platform/platform.h"
#include "platform/audio.h"
#include "ui/ui.h"
#include "ui/label.h"
#include "ui/button.h"
#include "ui/checkbox.h"
#include "ui/shaded_container.h"
#include "world/view_point.h"
#include "world/world.h"
#include "block/block.h"
#include "block/builtin/air.h"
#include "block/builtin/stone.h"
#include "ui/dynamic_label.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <cstdlib>
#include "util/logging.h"

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace
{
const int startingBoxSize = 30;
class MyUi : public ui::Ui
{
private:
    float viewAngle = 0;
    World world;
    WorldLockManager &render_thread_lock_manager;
    std::shared_ptr<ViewPoint> viewPoint;
    PositionI origin()
    {
        return PositionI(0, World::AverageGroundHeight + 8, 0, Dimension::Overworld);
    }
public:
    MyUi(Renderer &renderer, WorldLockManager &lock_manager)
        : world(), render_thread_lock_manager(lock_manager)
    {
        viewPoint = make_shared<ViewPoint>(world, origin(), 16);
    }
    virtual void move(double deltaTime) override
    {
        viewAngle = std::fmod(viewAngle + deltaTime * 2 * M_PI / 50, 2 * M_PI);
        Ui::move(deltaTime);
        world.move(deltaTime, render_thread_lock_manager);
    }
protected:
    virtual void clear(Renderer &renderer) override
    {
        background = HSVF(0.6, 0.5, 1);
        Ui::clear(renderer);
        Matrix tform = Matrix::translate(-((VectorF)origin() + VectorF(0.5, 0.5, 0.5))).concat(Matrix::rotateY(viewAngle)).concat(Matrix::rotateX(0.25 * M_PI));
        viewPoint->setPosition(origin());
        viewPoint->render(renderer, tform, render_thread_lock_manager);
        renderer << start_overlay << enable_depth_buffer;
    }
};
}

int main(std::vector<std::wstring> args)
{
    startGraphics();
    Audio sound(L"background7.ogg", true);
    Renderer renderer;
    WorldLockManager lock_manager;
    auto ui = make_shared<MyUi>(renderer, lock_manager);
    {
        wostringstream ss;
        ss << L"test label";
        ui->add(make_shared<voxels::ui::Label>(ss.str(), -0.5, 0.5, -0.8, -0.7));
    }
    weak_ptr<MyUi> wui = ui;
    ui->add(make_shared<voxels::ui::DynamicLabel>([](double deltaTime)->std::wstring
    {
        wostringstream ss;
        ss << L"FPS: ";
        ss.precision(1);
        ss.setf(ios::fixed, ios::floatfield);
        ss.width(5);
        if(deltaTime <= 1e-7)
            ss << L"     ";
        else
            ss << 1.0 / deltaTime;
        return ss.str();
    }, -0.5, 0.5, -0.95, -0.85));
    auto buttonContainer = make_shared<voxels::ui::ShadedContainer>(-0.7, 0.7, -0.6, 0.6);
    ui->add(buttonContainer);
    auto quitButton = make_shared<voxels::ui::Button>(L"Quit", -0.5, 0.5, -0.1, 0.1);
    buttonContainer->add(quitButton);
    quitButton->click.bind([=](EventArguments &)->Event::ReturnType
    {
        wui.lock()->quit();
        return Event::Discard;
    });
    auto button2 = make_shared<voxels::ui::Button>(L"button2", -0.5, 0.5, -0.4, -0.2);
    weak_ptr<voxels::ui::Button> wbutton2 = button2;
    buttonContainer->add(button2);
    button2->click.bind([](EventArguments &)->Event::ReturnType
    {
        getDebugLog() << L"button2 click" << postnl;
        return Event::Discard;
    });
    button2->pressed.onChange.bind([=](EventArguments &)->Event::ReturnType
    {
        getDebugLog() << L"button2.pressed changed: " << wbutton2.lock()->pressed.get() << postnl;
        return Event::Propagate;
    });
    auto checkbox = make_shared<voxels::ui::Checkbox>(L"checkbox", -0.5, 0.5, 0.2, 0.4);
    weak_ptr<voxels::ui::Checkbox> wcheckbox = checkbox;
    buttonContainer->add(checkbox);
    checkbox->checked.onChange.bind([=](EventArguments &)->Event::ReturnType
    {
        getDebugLog() << L"checkbox.checked changed: " << wcheckbox.lock()->checked.get() << postnl;
        return Event::Propagate;
    });
    shared_ptr<PlayingAudio> playingSound = sound.play(0.5f, true);
    ui->run(renderer);
    lock_manager.clear();
    endGraphics();
    return 0;
}
}
}
