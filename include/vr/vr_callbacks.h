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
#ifndef VR_CALLBACKS_H_INCLUDED
#define VR_CALLBACKS_H_INCLUDED

#include "platform/event.h"
#include "texture/image.h"
#include "render/renderer.h"
#include "util/matrix.h"
#include <tuple>

namespace programmerjake
{
namespace voxels
{
class VirtualRealityCallbacks : public EventHandler
{
public:
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        return false;
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        return false;
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        return false;
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        return false;
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        return false;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        return false;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event) override
    {
        return false;
    }
    virtual bool handleKeyUp(KeyUpEvent &event) override
    {
        return false;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        return false;
    }
    virtual bool handleKeyPress(KeyPressEvent &event) override
    {
        return false;
    }
    virtual bool handleQuit(QuitEvent &event) override
    {
        return false;
    }
    virtual Image transformBackgroundImage(Image backgroundImage)
    {
        return backgroundImage;
    }
    virtual void render(Renderer &renderer)
    {

    }
    virtual void move(double deltaTime, std::function<void(Matrix viewMatrix)> setViewMatrix)
    {
    }
};
}
}

#endif // VR_CALLBACKS_H_INCLUDED
