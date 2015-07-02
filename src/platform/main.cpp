/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "platform/platform.h"
#include "platform/audio.h"
#include "world/world.h"
#include "util/logging.h"
#include "ui/gameui.h"
#include <vector>
#include <string>
#include <memory>
#include "render/renderer.h"
#include "render/render_settings.h"
#include "util/tls.h"
#include "platform/video_input.h"

namespace programmerjake
{
namespace voxels
{
int main(std::vector<std::wstring> args)
{
    TLS tls;
    //globalRenderSettings.useFancyLeaves = true;
    startGraphics();
    Renderer renderer;
    std::shared_ptr<ui::GameUi> theUi = std::make_shared<ui::GameUi>(renderer, tls);
#if 0
    const std::vector<const VideoInputDevice *> &videoInputDevices = getVideoInputDeviceList();
    if(!videoInputDevices.empty())
        theUi->setBackgroundCamera(videoInputDevices.front()->makeVideoInput());
#endif
    theUi->run(renderer);
    theUi = nullptr;
    endGraphics();
    return 0;
}
}
}
