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
    std::unordered_map<int, std::size_t> touchElementMap;
    std::size_t &findOrGetTouch(int touchId)
    {
        auto iter = touchElementMap.find(touchId);
        if(iter == touchElementMap.end())
            iter = std::get<0>(touchElementMap.insert(std::make_pair(touchId, npos)));
        return std::get<1>(*iter);
    }
protected:
    static constexpr std::size_t npos = ~(std::size_t)0;
    virtual void onSetFocus()
    {
    }
    std::shared_ptr<Element> operator [](std::size_t index)
    {
        return elements[index];
    }
    typedef typename decltype(elements)::iterator iterator;
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
        : Element(minX, maxX, minY, maxY), elements(), touchElementMap()
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
                for(auto &v : touchElementMap)
                {
                    removeHelper(i, std::get<1>(v));
                }
                elements.erase(elements.begin() + i);
                return true;
            }
        }
        return false;
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
        for(currentFocusIndex = 0; currentFocusIndex < elements.size(); currentFocusIndex++)
        {
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->firstFocusElement();
                onSetFocus();
                return;
            }
        }
        currentFocusIndex = 0;
        onSetFocus();
    }

    virtual void lastFocusElement() override final
    {
        currentFocusIndex = elements.size() - 1;
        for(size_t i = 0; i < elements.size(); currentFocusIndex--, i++)
        {
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->lastFocusElement();
                onSetFocus();
                return;
            }
        }
        currentFocusIndex = 0;
        onSetFocus();
    }

    virtual bool prevFocusElement() override final /// returns true when reached container boundary
    {
        if(elements.size() == 0)
            return true;
        if(currentFocusIndex >= elements.size())
        {
            currentFocusIndex = 0;
            return true;
        }
        if(!elements[currentFocusIndex]->prevFocusElement())
            return false;
        do
        {
            currentFocusIndex += elements.size() - 1;
            currentFocusIndex %= elements.size();
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->lastFocusElement();
                onSetFocus();
                return currentFocusIndex == elements.size() - 1;
            }
        }
        while(currentFocusIndex != elements.size() - 1);
        lastFocusElement();
        return true;
    }

    virtual bool nextFocusElement() override final /// returns true when reached container boundary
    {
        if(elements.size() == 0)
            return true;
        if(currentFocusIndex >= elements.size())
        {
            currentFocusIndex = 0;
            return true;
        }
        if(!elements[currentFocusIndex]->nextFocusElement())
            return false;
        do
        {
            currentFocusIndex++;
            currentFocusIndex %= elements.size();
            if(elements[currentFocusIndex]->canHaveKeyboardFocus())
            {
                elements[currentFocusIndex]->firstFocusElement();
                onSetFocus();
                return currentFocusIndex == 0;
            }
        }
        while(currentFocusIndex != 0);
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
        auto iter = touchElementMap.find(event.touchId);
        if(iter == touchElementMap.end())
            return Element::handleTouchUp(event);
        std::size_t lastTouchElement = std::get<1>(*iter);
        touchElementMap.erase(iter);
        if(lastTouchElement != index)
        {
            if(lastTouchElement != npos)
            {
                elements[lastTouchElement]->handleTouchMoveOut(event);
            }
            lastTouchElement = index;
            if(index != npos)
            {
                elements[index]->handleTouchMoveIn(event);
            }
        }
        if(index != npos)
        {
            return elements[index]->handleTouchUp(event);
        }
        return Element::handleTouchUp(event);
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformTouchTo3D(event.x, event.y));
        std::size_t &lastTouchElement = findOrGetTouch(event.touchId);
        if(index != lastTouchElement)
        {
            if(lastTouchElement != npos)
            {
                elements[lastTouchElement]->handleTouchMoveOut(event);
            }
            lastTouchElement = index;
            if(index != npos)
            {
                elements[index]->handleTouchMoveIn(event);
            }
        }
        if(index != npos)
        {
            return elements[index]->handleTouchDown(event);
        }
        return Element::handleTouchDown(event);
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformTouchTo3D(event.x, event.y));
        std::size_t &lastTouchElement = findOrGetTouch(event.touchId);
        if(index != lastTouchElement)
        {
            if(lastTouchElement != npos)
            {
                elements[lastTouchElement]->handleTouchMoveOut(event);
            }
            lastTouchElement = index;
            if(index != npos)
            {
                elements[index]->handleTouchMoveIn(event);
            }
        }
        if(index != npos)
        {
            return elements[index]->handleTouchMove(event);
        }
        return Element::handleTouchMove(event);
    }
    virtual bool handleTouchMoveOut(TouchEvent &event) override
    {
        auto iter = touchElementMap.find(event.touchId);
        if(iter == touchElementMap.end())
            return Element::handleTouchMoveOut(event);
        std::size_t lastTouchElement = std::get<1>(*iter);
        touchElementMap.erase(iter);
        if(lastTouchElement != npos)
        {
            const std::shared_ptr<Element> &e = elements[lastTouchElement];
            lastTouchElement = npos;
            return e->handleTouchMoveOut(event);
        }
        return Element::handleTouchMoveOut(event);
    }
    virtual bool handleTouchMoveIn(TouchEvent &event) override
    {
        std::size_t index = getIndexFromPosition(Display::transformTouchTo3D(event.x, event.y));
        std::size_t &lastTouchElement = findOrGetTouch(event.touchId);
        lastTouchElement = index;
        if(index != npos)
        {
            return elements[index]->handleTouchMoveIn(event);
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
        if(getParent() != nullptr || (event.mods & (KeyboardModifiers_Ctrl | KeyboardModifiers_Alt | KeyboardModifiers_Meta | KeyboardModifiers_Mode)) != 0)
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
    virtual bool handleKeyPress(KeyPressEvent &event) override
    {
        auto e = getFocusElement();
        if(e.get() == this)
            return Element::handleKeyPress(event);
        return e->handleKeyPress(event);
    }
    virtual bool handleQuit(QuitEvent &event) override
    {
        auto e = getFocusElement();
        if(e.get() == this)
            return Element::handleQuit(event);
        return e->handleQuit(event);
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
                    currentFocusIndex = i;
                    if(getParent() != nullptr)
                    {
                        getParent()->setFocus(shared_from_this());
                    }
                    onSetFocus();
                }
            }
        }
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
        for(auto nextParent = retval->getParent(); nextParent != nullptr; nextParent = retval->getParent())
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

inline std::shared_ptr<Container> Element::getTopLevelParent() // this is here so that Container is defined
{
    auto retval = getParent();
    if(retval == nullptr)
        return retval;
    for(auto nextParent = retval->getParent(); nextParent != nullptr; nextParent = retval->getParent())
    {
        retval = nextParent;
    }
    return retval;
}
}
}
}

#endif // CONTAINER_H_INCLUDED
