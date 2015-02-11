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
#ifndef ELEMENT_H_INCLUDED
#define ELEMENT_H_INCLUDED

#include "platform/event.h"
#include "render/renderer.h"
#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class Container;

class Element : public EventHandler, public std::enable_shared_from_this<Element>
{
public:
    Element(float minX, float maxX, float minY, float maxY);
    virtual ~Element();
    virtual bool isPointInside(float x, float y) const
    {
        return x >= minX && x < maxX && y >= minY && y < maxY;
    }
    virtual bool canHaveKeyboardFocus() const
    {
        return false;
    }
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
    virtual bool handleMouseMoveOut(MouseEvent &event)
    {
        return true;
    }
    virtual bool handleMouseMoveIn(MouseEvent &event)
    {
        return true;
    }
    virtual bool handleTouchMoveOut(TouchEvent &event)
    {
        return true;
    }
    virtual bool handleTouchMoveIn(TouchEvent &event)
    {
        return true;
    }
    virtual std::shared_ptr<Element> getFocusElement()
    {
        return shared_from_this();
    }
    virtual bool nextFocusElement() /// returns true when reached container boundary
    {
        return true;
    }
    virtual bool prevFocusElement() /// returns true when reached container boundary
    {
        return true;
    }
    virtual void firstFocusElement()
    {
    }
    virtual void lastFocusElement()
    {
    }
    void setFocus();
    float minX, maxX, minY, maxY;
    virtual void reset()
    {
    }
    std::shared_ptr<Container> getParent() const
    {
        return parent.lock();
    }
    virtual std::shared_ptr<Container> getTopLevelParent();
    virtual void move(double deltaTime)
    {
    }
    virtual void moveBy(float dx, float dy)
    {
        minX += dx;
        maxX += dx;
        minY += dy;
        maxY += dy;
    }
    void moveCenterTo(float x, float y)
    {
        moveBy(x - 0.5f * (minX + maxX), y - 0.5f * (minY + maxY));
    }
    void moveTopLeftTo(float x, float y)
    {
        moveBy(x - minX, y - minY);
    }
    void moveTopRightTo(float x, float y)
    {
        moveBy(x - maxX, y - minY);
    }
    void moveBottomLeftTo(float x, float y)
    {
        moveBy(x - minX, y - maxY);
    }
    void moveBottomRightTo(float x, float y)
    {
        moveBy(x - maxX, y - maxY);
    }
    virtual void layout()
    {
    }
protected:
    /**
     * @brief render this element
     * @param renderer the renderer
     * @param minZ the minimum depth to render at
     * @param maxZ the maximum depth to render at (this element can render at minZ <= z < maxZ
     * @param hasFocus if this element currently has focus
     */
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) = 0;
private:
    friend class Container; // so Container can set parent
    std::weak_ptr<Container> parent;
};
}
}
}

#include "ui/container.h"

#endif // ELEMENT_H_INCLUDED
