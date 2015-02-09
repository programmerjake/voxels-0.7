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
#include <iostream>

using namespace std;

namespace programmerjake
{
namespace voxels
{
int main(std::vector<std::wstring> args)
{
    startGraphics();
    Audio sound(L"background7.ogg", true);
    shared_ptr<PlayingAudio> playingSound = sound.play(true);

    auto ui = make_shared<voxels::ui::Ui>();
    ui->add(make_shared<voxels::ui::Label>(L"label", -0.5, 0.5, -0.8, -0.7));
    auto quitButton = make_shared<voxels::ui::Button>(L"Quit", -0.5, 0.5, -0.1, 0.1);
    ui->add(quitButton);
    quitButton->click.bind([=](EventArguments &)->Event::ReturnType
    {
        ui->quit();
        return Event::Discard;
    });
    auto button2 = make_shared<voxels::ui::Button>(L"button2", -0.5, 0.5, -0.4, -0.2);
    ui->add(button2);
    button2->click.bind([=](EventArguments &)->Event::ReturnType
    {
        cout << "button2 click" << endl;
        return Event::Discard;
    });
    button2->pressed.onChange.bind([=](EventArguments &)->Event::ReturnType
    {
        cout << "button2.pressed changed: " << button2->pressed.get() << endl;
        return Event::Propagate;
    });
    auto checkbox = make_shared<voxels::ui::Checkbox>(L"checkbox", -0.5, 0.5, 0.2, 0.4);
    ui->add(checkbox);
    checkbox->checked.onChange.bind([=](EventArguments &)->Event::ReturnType
    {
        cout << "checkbox.checked changed: " << checkbox->checked.get() << endl;
        return Event::Propagate;
    });
    ui->run();
    endGraphics();
    return 0;
}
}
}
