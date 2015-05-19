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
#include "ui/ui.h"
#include "platform/platform.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
void Ui::clear(Renderer &renderer)
{
    Display::clear(background);
}

void Ui::run(Renderer &renderer)
{
    Display::initFrame();
    reset();
    double doneTime = 0.2;
    while(doneTime > 0)
    {
        Display::handleEvents(shared_from_this());
        move(Display::frameDeltaTime());
        if(isDone())
            doneTime -= Display::frameDeltaTime();
        Display::initFrame();
        clear(renderer);
        layout();
        render(renderer, 1, 32, true);
        Display::flip(-1);
    }
    handleFinish();
}
}
}
}
