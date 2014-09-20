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

#include "platform/platform.h"
#include <memory>

class KeyDownEvent;
class MouseUpEvent;
class MouseDownEvent;
class MouseMoveEvent;
class MouseScrollEvent;
class KeyUpEvent;
class KeyPressEvent;
class QuitEvent;

struct EventHandler
{
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

class Event
{
public:
    enum Type
    {
        Type_MouseUp,
        Type_MouseDown,
        Type_MouseMove,
        Type_MouseScroll,
        Type_KeyUp,
        Type_KeyDown,
        Type_KeyPress,
        Type_Quit,
    };
    const Type type;
protected:
    Event(Type type)
        : type(type)
    {
    }
public:
    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) = 0;
};

class MouseEvent : public Event
{
public:
    const float x, y;
    const float deltaX, deltaY;
protected:
    MouseEvent(Type type, float x, float y, float deltaX, float deltaY)
        : Event(type), x(x), y(y), deltaX(deltaX), deltaY(deltaY)
    {
    }
};

class KeyEvent : public Event
{
public:
    const KeyboardKey key;
    const KeyboardModifiers mods;
protected:
    KeyEvent(Type type, KeyboardKey key, KeyboardModifiers mods): Event(type), key(key), mods(mods)
    {
    }
};

class KeyDownEvent final : public KeyEvent
{
public:
    const bool isRepetition;
    KeyDownEvent(KeyboardKey key, KeyboardModifiers mods, bool isRepetition = false) : KeyEvent(Type_KeyDown, key, mods), isRepetition(isRepetition)
    {
    }
    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleKeyDown(*this);
    }
};

class KeyUpEvent : public KeyEvent
{
public:
    KeyUpEvent(KeyboardKey key, KeyboardModifiers mods)
        : KeyEvent(Type_KeyUp, key, mods)
    {
    }
    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleKeyUp(*this);
    }
};

struct KeyPressEvent : public Event
{
    const wchar_t character;
    KeyPressEvent(wchar_t character)
        : Event(Type_KeyPress), character(character)
    {
    }
    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
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
    MouseUpEvent(float x, float y, float deltaX, float deltaY, MouseButton button) : MouseButtonEvent(Type_MouseUp, x, y, deltaX, deltaY, button)
    {
    }

    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseUp(*this);
    }
};

struct MouseDownEvent : public MouseButtonEvent
{
    MouseDownEvent(float x, float y, float deltaX, float deltaY, MouseButton button)
        : MouseButtonEvent(Type_MouseDown, x, y, deltaX, deltaY, button)
    {
    }

    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseDown(*this);
    }
};

struct MouseMoveEvent : public MouseEvent
{
    MouseMoveEvent(float x, float y, float deltaX, float deltaY)
        : MouseEvent(Type_MouseMove, x, y, deltaX, deltaY)
    {
    }

    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseMove(*this);
    }
};

struct MouseScrollEvent : public MouseEvent
{
    const int scrollX, scrollY;
    MouseScrollEvent(float x, float y, float deltaX, float deltaY, int scrollX, int scrollY)
        : MouseEvent(Type_MouseScroll, x, y, deltaX, deltaY), scrollX(scrollX), scrollY(scrollY)
    {
    }

    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleMouseScroll(*this);
    }
};

struct QuitEvent : public Event
{
    QuitEvent()
        : Event(Type_Quit)
    {
    }
    virtual bool dispatch(shared_ptr<EventHandler> eventHandler) override
    {
        return eventHandler->handleQuit(*this);
    }
};

class CombinedEventHandler final : public EventHandler
{
private:
    shared_ptr<EventHandler> first, second;
public:
    CombinedEventHandler(shared_ptr<EventHandler> first, shared_ptr<EventHandler> second)
        : first(first), second(second)
    {
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

#endif // EVENT_H_INCLUDED
