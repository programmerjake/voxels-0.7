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
    auto button1 = make_shared<voxels::ui::Button>(L"Quit", -0.5, 0.5, -0.1, 0.1);
    ui->add(button1);
    button1->click.bind([=](EventArguments &)->Event::ReturnType
    {
        ui->quit();
        return Event::Discard;
    });
    ui->run();
    endGraphics();
    return 0;
}
}
}
