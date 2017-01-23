/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#include "ui/element.h"
#ifndef CONTAINER_H_INCLUDED
#define CONTAINER_H_INCLUDED

#include <vector>
#include <unordered_map>
#include <cassert>
#include "platform/platform.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class Container : public Element
{
private:
    std::vector<std::shared_ptr<Element>> elements;
    std::size_t currentFocusIndex = 0;
    std::size_t lastMouseElement = npos;
    struct TouchStruct final
    {
        std::size_t elementIndex = npos;
        bool captured = false;
    };
    std::unordered_map<int, TouchStruct> touchMap;
    TouchStruct &findOrGetTouch(int touchId)
    {
        auto iter = touchMap.find(touchId);
        if(iter == touchMap.end())
            iter = std::get<0>(touchMap.emplace(touchId, TouchStruct()));
        return std::get<1>(*iter);
    }

protected:
    static constexpr std::size_t npos = ~static_cast<std::size_t>(0);
    std::shared_ptr<Element> operator[](std::size_t index)
    {
        return elements[index];
    }
    typedef decltype(elements)::iterator iterator;
    iterator begin()
    {
        return elements.begin();
    }
    iterator end()
    {
        return elements.end();
    }
    std::size_t size() const
    {
        return elements.size();
    }

public:
    using Element::render;
    Container(float minX, float maxX, float minY, float maxY)
        : Element(minX, maxX, minY, maxY), elements(), touchMap()
    {
    }
    std::shared_ptr<Container> add(std::shared_ptr<Element> element)
    {
        assert(element != nullptr);
        elements.push_back(element);
        auto sthis = std::static_pointer_cast<Container>(shared_from_this());
        element->parent = sthis;
        return std::move(sthis);
    }

private:
    void onSetFocus(std::shared_ptr<Element> newFocusedElement,
                    std::shared_ptr<Element> oldFocusedElement)
    {
        if(newFocusedElement == oldFocusedElement)
            return;
        if(oldFocusedElement.get() != this)
            oldFocusedElement->handleFocusChange(false);
        if(newFocusedElement.get() != this)
            newFocusedElement->handleFocusChange(true);
    }
    void removeHelper(std::size_t removedIndex, std::size_t &adjustIndex) const
    {
        if(adjustIndex != npos)
        {
            if(adjustIndex == removedIndex)
                adjustIndex = npos;
            else if(adjustIndex > removedIndex)
                adjustIndex--;
        }
    }
    void removeHelper(std::size_t removedIndex, TouchStruct &adjustStruct) const
    {
        if(adjustStruct.elementIndex != npos)
        {
            if(adjustStruct.elementIndex == removedIndex)
            {
                adjustStruct.elementIndex = npos;
                adjustStruct.captured = false;
            }
            else if(adjustStruct.elementIndex > removedIndex)
                adjustStruct.elementIndex--;
        }
    }

public:
    bool remove(std::shared_ptr<Element> element)
    {
        assert(element != nullptr);
        for(std::size_t i = 0; i < elements.size(); i++)
        {
            if(elements[i] == element)
            {
                element->parent.reset();
                removeHelper(i, currentFocusIndex);
                removeHelper(i, lastMouseElement);
                for(auto &v : touchMap)
                {
                    removeHelper(i, std::get<1>(v));
                }
                elements.erase(elements.begin() + i);
                return true;
            }
        }
        return false;
    }
    void captureTouch(int touchIndex)
    {
        auto i = touchMap.find(touchIndex);
        if(i != touchMap.end())
        {
            TouchStruct &touch = std::get<1>(*i);
            touch.captured = true;
        }
        std::shared_ptr<Container> parent = getParent();
        if(parent != nullptr)
            parent->captureTouch(touchIndex);
    }
    virtual std::shared_ptr<Element> getFocusElement() override final
    {
        if(currentFocusIndex >= elements.size() || currentFocusIndex == npos)
            return shared_from_this();
        return elements[currentFocusIndex];
    }
    virtual bool canHaveKeyboardFocus() const override
    {
        for(const std::shared_ptr<Element> &e : elements)
            if(e->canHaveKeyboardFocus())
                return true;
        return false;
    }
    virtual void firstFocusElement() override final
    {
        auto oldFocusElement = getFocusElement();
        for(currentFocusIndex = 0; currentFocusIndex < elements.size(); currentFocusIndex++)
        {
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->firstFocusElement();
                onSetFocus(getFocusElement(), oldFocusElement);
                return;
            }
        }
        currentFocusIndex = 0;
        onSetFocus(getFocusElement(), oldFocusElement);
    }

    virtual void lastFocusElement() override final
    {
        auto oldFocusElement = getFocusElement();
        currentFocusIndex = elements.size() - 1;
        for(size_t i = 0; i < elements.size(); currentFocusIndex--, i++)
        {
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->lastFocusElement();
                onSetFocus(getFocusElement(), oldFocusElement);
                return;
            }
        }
        currentFocusIndex = 0;
        onSetFocus(getFocusElement(), oldFocusElement);
    }

    virtual bool prevFocusElement() override final /// returns true when reached container boundary
    {
        auto oldFocusElement = getFocusElement();
        if(elements.size() == 0)
            return true;
        if(currentFocusIndex >= elements.size())
        {
            currentFocusIndex = 0;
            onSetFocus(getFocusElement(), oldFocusElement);
            return true;
        }
        if(!elements[currentFocusIndex]->prevFocusElement())
            return false;
        std::size_t oldFocusIndex = currentFocusIndex;
        do
        {
            currentFocusIndex += elements.size() - 1;
            currentFocusIndex %= elements.size();
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->lastFocusElement();
                onSetFocus(getFocusElement(), oldFocusElement);
                return currentFocusIndex == elements.size() - 1;
            }
        } while(currentFocusIndex != elements.size() - 1);
        currentFocusIndex = oldFocusIndex;
        lastFocusElement();
        return true;
    }

    virtual bool nextFocusElement() override final /// returns true when reached container boundary
    {
        auto oldFocusElement = getFocusElement();
        if(elements.size() == 0)
            return true;
        if(currentFocusIndex >= elements.size())
        {
            currentFocusIndex = 0;
            onSetFocus(getFocusElement(), oldFocusElement);
            return true;
        }
        if(!elements[currentFocusIndex]->nextFocusElement())
            return false;
        std::size_t oldFocusIndex = currentFocusIndex;
        do
        {
            currentFocusIndex++;
            currentFocusIndex %= elements.size();
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->firstFocusElement();
                onSetFocus(getFocusElement(), oldFocusElement);
                return currentFocusIndex == 0;
            }
        } while(currentFocusIndex != 0);
        currentFocusIndex = oldFocusIndex;
        firstFocusElement();
        return true;
    }

private:
    std::size_t getIndexFromPosition(float x, float y) /// viewport position
    {
        for(std::size_t i = 0, retval = elements.size() - 1; i < elements.size(); i++, retval--)
        {
            if(elements[retval]->isPointInside(x, y))
                return retval;
        }
        return npos;
    }
    std::size_t getIndexFromPosition(VectorF pos) /// viewport position; pos.z must be -1
    {
        assert(pos.z == -1);
        return getIndexFromPosition(pos.x, pos.y);
    }

public:
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformTouchTo3D(event.x, event.y));
        auto iter = touchMap.find(event.touchId);
        if(iter == touchMap.end())
            return Element::handleTouchUp(event);
        TouchStruct touch = std::get<1>(*iter);
        touchMap.erase(iter);
        if(touch.elementIndex != index && !touch.captured)
        {
            if(touch.elementIndex != npos)
            {
                elements[touch.elementIndex]->handleTouchMoveOut(event);
            }
            if(!touch.captured) // might have been captured in touch move out
            {
                touch.elementIndex = index;
                if(index != npos)
                {
                    elements[index]->handleTouchMoveIn(event);
                }
            }
        }
        if(touch.elementIndex != npos)
        {
            return elements[touch.elementIndex]->handleTouchUp(event);
        }
        return Element::handleTouchUp(event);
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformTouchTo3D(event.x, event.y));
        TouchStruct &touch = findOrGetTouch(event.touchId);
        if(index != touch.elementIndex && !touch.captured)
        {
            if(touch.elementIndex != npos)
            {
                elements[touch.elementIndex]->handleTouchMoveOut(event);
            }
            if(!touch.captured) // might have been captured in touch move out
            {
                touch.elementIndex = index;
                if(index != npos)
                {
                    elements[index]->handleTouchMoveIn(event);
                }
            }
        }
        if(touch.elementIndex != npos)
        {
            return elements[touch.elementIndex]->handleTouchDown(event);
        }
        return Element::handleTouchDown(event);
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformTouchTo3D(event.x, event.y));
        TouchStruct &touch = findOrGetTouch(event.touchId);
        if(index != touch.elementIndex && !touch.captured)
        {
            if(touch.elementIndex != npos)
            {
                elements[touch.elementIndex]->handleTouchMoveOut(event);
            }
            if(!touch.captured) // might have been captured in touch move out
            {
                touch.elementIndex = index;
                if(index != npos)
                {
                    elements[index]->handleTouchMoveIn(event);
                }
            }
        }
        if(touch.elementIndex != npos)
        {
            return elements[touch.elementIndex]->handleTouchMove(event);
        }
        return Element::handleTouchMove(event);
    }
    virtual bool handleTouchMoveOut(TouchEvent &event) override
    {
        auto iter = touchMap.find(event.touchId);
        if(iter == touchMap.end())
            return Element::handleTouchMoveOut(event);
        TouchStruct touch = std::get<1>(*iter);
        touchMap.erase(iter);
        if(touch.elementIndex != npos)
        {
            const std::shared_ptr<Element> &e = elements[touch.elementIndex];
            touch.elementIndex = npos;
            return e->handleTouchMoveOut(event);
        }
        return Element::handleTouchMoveOut(event);
    }
    virtual bool handleTouchMoveIn(TouchEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformTouchTo3D(event.x, event.y));
        TouchStruct &touch = findOrGetTouch(event.touchId);
        touch.elementIndex = index;
        if(touch.elementIndex != npos)
        {
            return elements[touch.elementIndex]->handleTouchMoveIn(event);
        }
        return Element::handleTouchMoveIn(event);
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformMouseTo3D(event.x, event.y));
        if(index != lastMouseElement)
        {
            if(lastMouseElement != npos)
            {
                elements[lastMouseElement]->handleMouseMoveOut(event);
            }
            lastMouseElement = index;
            if(index != npos)
            {
                elements[index]->handleMouseMoveIn(event);
            }
        }
        if(index != npos)
        {
            return elements[index]->handleMouseUp(event);
        }
        return Element::handleMouseUp(event);
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformMouseTo3D(event.x, event.y));
        if(index != lastMouseElement)
        {
            if(lastMouseElement != npos)
            {
                elements[lastMouseElement]->handleMouseMoveOut(event);
            }
            lastMouseElement = index;
            if(index != npos)
            {
                elements[index]->handleMouseMoveIn(event);
            }
        }
        if(index != npos)
        {
            return elements[index]->handleMouseDown(event);
        }
        return Element::handleMouseDown(event);
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformMouseTo3D(event.x, event.y));
        if(index != lastMouseElement)
        {
            if(lastMouseElement != npos)
            {
                elements[lastMouseElement]->handleMouseMoveOut(event);
            }
            lastMouseElement = index;
            if(index != npos)
            {
                elements[index]->handleMouseMoveIn(event);
            }
        }
        if(index != npos)
        {
            return elements[index]->handleMouseMove(event);
        }
        return Element::handleMouseMove(event);
    }
    virtual bool handleMouseMoveOut(MouseEvent &event) override
    {
        if(lastMouseElement != npos)
        {
            const std::shared_ptr<Element> &e = elements[lastMouseElement];
            lastMouseElement = npos;
            return e->handleMouseMoveOut(event);
        }
        return Element::handleMouseMoveOut(event);
    }
    virtual bool handleMouseMoveIn(MouseEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformMouseTo3D(event.x, event.y));
        lastMouseElement = index;
        if(index != npos)
        {
            return elements[index]->handleMouseMoveIn(event);
        }
        return Element::handleMouseMoveIn(event);
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event) override
    {
        if(lastMouseElement != npos)
        {
            return elements[lastMouseElement]->handleMouseScroll(event);
        }
        return Element::handleMouseScroll(event);
    }
    virtual bool handleKeyUp(KeyUpEvent &event) override
    {
        auto e = getFocusElement();
        if(e.get() == this)
            return Element::handleKeyUp(event);
        return e->handleKeyUp(event);
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        auto e = getFocusElement();
        bool retval;
        if(e.get() == this)
            retval = Element::handleKeyDown(event);
        else
            retval = e->handleKeyDown(event);
        if(retval)
            return true;
        if(getParent() != nullptr
           || (event.mods & (KeyboardModifiers_Ctrl | KeyboardModifiers_Alt | KeyboardModifiers_Meta
                             | KeyboardModifiers_Mode)) != 0)
            return false;
        if(event.key == KeyboardKey::Tab)
        {
            if(event.mods & KeyboardModifiers_Shift)
                prevFocusElement();
            else
                nextFocusElement();
            return true;
        }
        else if(event.mods & KeyboardModifiers_Shift)
            return false;
        else if(event.key == KeyboardKey::Down)
        {
            nextFocusElement();
            return true;
        }
        else if(event.key == KeyboardKey::Up)
        {
            prevFocusElement();
            return true;
        }
        else if(event.key == KeyboardKey::Home)
        {
            firstFocusElement();
            return true;
        }
        else if(event.key == KeyboardKey::End)
        {
            lastFocusElement();
            return true;
        }
        return false;
    }
    virtual bool handleTextInput(TextInputEvent &event) override
    {
        auto e = getFocusElement();
        if(e.get() == this)
            return Element::handleTextInput(event);
        return e->handleTextInput(event);
    }
    virtual bool handleTextEdit(TextEditEvent &event) override
    {
        auto e = getFocusElement();
        if(e.get() == this)
            return Element::handleTextEdit(event);
        return e->handleTextEdit(event);
    }
    virtual bool handleQuit(QuitEvent &event) override
    {
        auto e = getFocusElement();
        if(e.get() == this)
            return Element::handleQuit(event);
        return e->handleQuit(event);
    }
    virtual bool handlePause(PauseEvent &event) override
    {
        bool retval = false;
        if(currentFocusIndex < elements.size())
            retval = elements[currentFocusIndex]->handlePause(event);
        for(std::size_t index = 0; !retval && index < elements.size(); index++)
        {
            if(index == currentFocusIndex)
                continue;
            retval = elements[index]->handlePause(event);
        }
        return retval;
    }
    virtual bool handleResume(ResumeEvent &event) override
    {
        bool retval = false;
        if(currentFocusIndex < elements.size())
            retval = elements[currentFocusIndex]->handleResume(event);
        for(std::size_t index = 0; !retval && index < elements.size(); index++)
        {
            if(index == currentFocusIndex)
                continue;
            retval = elements[index]->handleResume(event);
        }
        return retval;
    }
    virtual void handleFocusChange(bool gettingFocus) override
    {
        auto e = getFocusElement();
        if(e.get() != this)
            e->handleFocusChange(gettingFocus);
    }

protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override;

public:
    void setFocus(std::shared_ptr<Element> e)
    {
        if(e == nullptr)
            return;
        for(std::size_t i = 0; i < elements.size(); i++)
        {
            if(elements[i] == e)
            {
                if(e->canHaveKeyboardFocus())
                {
                    auto oldFocusElement = getFocusElement();
                    if(oldFocusElement == e)
                    {
                        if(getParent() != nullptr)
                        {
                            getParent()->setFocus(shared_from_this());
                        }
                        return;
                    }
                    currentFocusIndex = npos;
                    onSetFocus(shared_from_this(), oldFocusElement);
                    if(getParent() != nullptr)
                    {
                        getParent()->setFocus(shared_from_this());
                    }
                    currentFocusIndex = i;
                    onSetFocus(e, shared_from_this());
                }
            }
        }
    }
    using Element::setFocus;
    void removeFocus(std::shared_ptr<Element> e)
    {
        if(getFocusElement() != e)
            return;
        std::shared_ptr<Container> topLevelParent = getTopLevelParent();
        topLevelParent->nextFocusElement();
    }
    virtual void reset() override
    {
        for(const std::shared_ptr<Element> &e : elements)
            e->reset();
        firstFocusElement();
    }
    virtual std::shared_ptr<Container> getTopLevelParent() override final
    {
        auto retval = std::static_pointer_cast<Container>(shared_from_this());
        for(auto nextParent = retval->getParent(); nextParent != nullptr;
            nextParent = retval->getParent())
        {
            retval = nextParent;
        }
        return retval;
    }
    virtual void move(double deltaTime) override
    {
        for(const std::shared_ptr<Element> &e : elements)
        {
            e->move(deltaTime);
        }
    }
    virtual void layout() override
    {
        for(const std::shared_ptr<Element> &e : elements)
        {
            e->layout();
        }
    }
    virtual void moveBy(float dx, float dy)
    {
        minX += dx;
        maxX += dx;
        minY += dy;
        maxY += dy;
        for(const std::shared_ptr<Element> &e : elements)
        {
            e->moveBy(dx, dy);
        }
    }
};

inline std::shared_ptr<Container>
    Element::getTopLevelParent() // this is here so that Container is defined
{
    auto retval = getParent();
    if(retval == nullptr)
        return retval;
    for(auto nextParent = retval->getParent(); nextParent != nullptr;
        nextParent = retval->getParent())
    {
        retval = nextParent;
    }
    return retval;
}
}
}
}

#endif // CONTAINER_H_INCLUDED
