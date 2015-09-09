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
#include "vr/vr_callbacks.h"
#include "util/math_constants.h"
namespace programmerjake
{
namespace voxels
{
namespace
{
class MyVirtualRealityCallbacks final : public VirtualRealityCallbacks
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
    virtual bool handleTextInput(TextInputEvent &event) override
    {
        return false;
    }
    virtual bool handleTextEdit(TextEditEvent &event) override
    {
        return false;
    }
    virtual bool handleQuit(QuitEvent &event) override
    {
        return false;
    }
    virtual void transformBackgroundImage(Image &backgroundImage)
    {
    }
    virtual void render(Renderer &renderer)
    {

    }
    virtual void move(double deltaTime, Matrix originalViewMatrix, std::function<void(Matrix viewMatrix)> setViewMatrix) override
    {
        Matrix originalCameraToWorldMatrix = inverse(originalViewMatrix);
        static double t = 0;
        t += deltaTime;
        setViewMatrix(Matrix::translate(-transform(originalCameraToWorldMatrix, VectorF(0))).concat(Matrix::rotateY(t)).concat(Matrix::rotateX(M_PI / 6)).concat(Matrix::translate(0, 0, -20)));
    }
};
}
std::unique_ptr<VirtualRealityCallbacks> VirtualRealityCallbacks::make()
{
    return std::unique_ptr<VirtualRealityCallbacks>(new MyVirtualRealityCallbacks);
}
}
}
