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
#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

#include <memory>
#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{
class KeyDownEvent;
struct TouchUpEvent;
struct TouchDownEvent;
struct TouchMoveEvent;
struct MouseUpEvent;
struct MouseDownEvent;
struct MouseMoveEvent;
struct MouseScrollEvent;
class KeyUpEvent;
struct KeyPressEvent;
struct QuitEvent;

struct EventHandler
{
    virtual bool handleTouchUp(TouchUpEvent &event) = 0;
    virtual bool handleTouchDown(TouchDownEvent &event) = 0;
    virtual bool handleTouchMove(TouchMoveEvent &event) = 0;
    virtual bool handleMouseUp(MouseUpEvent &event) = 0;
    virtual bool handleMouseDown(MouseDownEvent &event) = 0;
    virtual bool handleMouseMove(MouseMoveEvent &event) = 0;
    virtual bool handleMouseScroll(MouseScrollEvent &event) = 0;
    virtual bool handleKeyUp(KeyUpEvent &event) = 0;
    virtual bool handleKeyDown(KeyDownEvent &event) = 0;
    virtual bool handleKeyPress(KeyPressEvent &event) = 0;
    virtual bool handleQuit(QuitEvent &event) = 0;
    virtual ~EventHandler()
    {
    }
};
}
}

#include "platform/platform.h"

namespace programmerjake
{
namespace voxels
{
class PlatformEvent
{
public:
    enum class Type
    {
        TouchUp,
        TouchDown,
        TouchMove,
        MouseUp,
        MouseDown,
        MouseMove,
        MouseScroll,
        KeyUp,
        KeyDown,
        KeyPress,
        Quit,
        DEFINE_ENUM_LIMITS(MouseUp, Quit)
    };
    const Type type;
protected:
    PlatformEvent(Type type)
        : type(type)
    {
    }
public:
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) = 0;
};

class TouchEvent : public PlatformEvent
{
public:
    const float x, y; /// normalized to the range -1 to 1
    const float deltaX, deltaY;
    const int touchId;
    const float pressure;
protected:
    TouchEvent(Type type, float x, float y, float deltaX, float deltaY, int touchId, float pressure)
        : PlatformEvent(type), x(x), y(y), deltaX(deltaX), deltaY(deltaY), touchId(touchId), pressure(pressure)
    {
    }
};

struct TouchDownEvent : public TouchEvent
{
    TouchDownEvent(float x, float y, float deltaX, float deltaY, int touchId, float pressure)
        : TouchEvent(Type::TouchDown, x, y, deltaX, deltaY, touchId, pressure)
    {
    }
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleTouchDown(*this);
    }
};

struct TouchUpEvent : public TouchEvent
{
    TouchUpEvent(float x, float y, float deltaX, float deltaY, int touchId, float pressure)
        : TouchEvent(Type::TouchUp, x, y, deltaX, deltaY, touchId, pressure)
    {
    }
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleTouchUp(*this);
    }
};

struct TouchMoveEvent : public TouchEvent
{
    TouchMoveEvent(float x, float y, float deltaX, float deltaY, int touchId, float pressure)
        : TouchEvent(Type::TouchMove, x, y, deltaX, deltaY, touchId, pressure)
    {
    }
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleTouchMove(*this);
    }
};

class MouseEvent : public PlatformEvent
{
public:
    const float x, y;
    const float deltaX, deltaY;
protected:
    MouseEvent(Type type, float x, float y, float deltaX, float deltaY)
        : PlatformEvent(type), x(x), y(y), deltaX(deltaX), deltaY(deltaY)
    {
    }
};

class KeyEvent : public PlatformEvent
{
public:
    const KeyboardKey key;
    const KeyboardModifiers mods;
protected:
    KeyEvent(Type type, KeyboardKey key, KeyboardModifiers mods)
        : PlatformEvent(type), key(key), mods(mods)
    {
    }
};

class KeyDownEvent final : public KeyEvent
{
public:
    const bool isRepetition;
    KeyDownEvent(KeyboardKey key, KeyboardModifiers mods, bool isRepetition = false)
        : KeyEvent(Type::KeyDown, key, mods), isRepetition(isRepetition)
    {
    }
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleKeyDown(*this);
    }
};

class KeyUpEvent : public KeyEvent
{
public:
    KeyUpEvent(KeyboardKey key, KeyboardModifiers mods)
        : KeyEvent(Type::KeyUp, key, mods)
    {
    }
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleKeyUp(*this);
    }
};

struct KeyPressEvent : public PlatformEvent
{
    const wchar_t character;
    KeyPressEvent(wchar_t character)
        : PlatformEvent(Type::KeyPress), character(character)
    {
    }
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleKeyPress(*this);
    }
};

struct MouseButtonEvent : public MouseEvent
{
    const MouseButton button;
protected:
    MouseButtonEvent(Type type, float x, float y, float deltaX, float deltaY, MouseButton button)
        : MouseEvent(type, x, y, deltaX, deltaY), button(button)
    {
    }
};

struct MouseUpEvent : public MouseButtonEvent
{
    MouseUpEvent(float x, float y, float deltaX, float deltaY, MouseButton button) : MouseButtonEvent(Type::MouseUp, x, y, deltaX, deltaY, button)
    {
    }

    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseUp(*this);
    }
};

struct MouseDownEvent : public MouseButtonEvent
{
    MouseDownEvent(float x, float y, float deltaX, float deltaY, MouseButton button)
        : MouseButtonEvent(Type::MouseDown, x, y, deltaX, deltaY, button)
    {
    }

    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseDown(*this);
    }
};

struct MouseMoveEvent : public MouseEvent
{
    MouseMoveEvent(float x, float y, float deltaX, float deltaY)
        : MouseEvent(Type::MouseMove, x, y, deltaX, deltaY)
    {
    }

    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseMove(*this);
    }
};

struct MouseScrollEvent : public PlatformEvent
{
    const int scrollX, scrollY;
    MouseScrollEvent(int scrollX, int scrollY)
        : PlatformEvent(Type::MouseScroll), scrollX(scrollX), scrollY(scrollY)
    {
    }

    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseScroll(*this);
    }
};

struct QuitEvent : public PlatformEvent
{
    QuitEvent()
        : PlatformEvent(Type::Quit)
    {
    }
    virtual bool dispatch(std::shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleQuit(*this);
    }
};

class CombinedEventHandler final : public EventHandler
{
private:
    std::shared_ptr<EventHandler> first, second;
public:
    CombinedEventHandler(std::shared_ptr<EventHandler> first, std::shared_ptr<EventHandler> second)
        : first(first), second(second)
    {
    }
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        if(first->handleTouchUp(event))
        {
            return true;
        }

        return second->handleTouchUp(event);
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        if(first->handleTouchDown(event))
        {
            return true;
        }

        return second->handleTouchDown(event);
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        if(first->handleTouchMove(event))
        {
            return true;
        }

        return second->handleTouchMove(event);
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        if(first->handleMouseUp(event))
        {
            return true;
        }

        return second->handleMouseUp(event);
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        if(first->handleMouseDown(event))
        {
            return true;
        }

        return second->handleMouseDown(event);
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        if(first->handleMouseMove(event))
        {
            return true;
        }

        return second->handleMouseMove(event);
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event)override
    {
        if(first->handleMouseScroll(event))
        {
            return true;
        }

        return second->handleMouseScroll(event);
    }
    virtual bool handleKeyUp(KeyUpEvent &event)override
    {
        if(first->handleKeyUp(event))
        {
            return true;
        }

        return second->handleKeyUp(event);
    }
    virtual bool handleKeyDown(KeyDownEvent &event)override
    {
        if(first->handleKeyDown(event))
        {
            return true;
        }

        return second->handleKeyDown(event);
    }
    virtual bool handleKeyPress(KeyPressEvent &event)override
    {
        if(first->handleKeyPress(event))
        {
            return true;
        }

        return second->handleKeyPress(event);
    }
    virtual bool handleQuit(QuitEvent &event)override
    {
        if(first->handleQuit(event))
        {
            return true;
        }

        return second->handleQuit(event);
    }
};
}
}

#endif // EVENT_H_INCLUDED
