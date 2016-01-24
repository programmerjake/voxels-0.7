/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
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
#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include "ui/element.h"
#include "ui/container.h"
#include "platform/platform.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class Ui : public Container
{
    bool done;
public:
    ColorF background;
    explicit Ui(ColorF background = GrayscaleF(0.4))
        : Container(-Display::scaleX(), Display::scaleX(), -Display::scaleY(), Display::scaleY()), done(false), background(background)
    {
    }
    static std::shared_ptr<Ui> get(std::shared_ptr<Element> element)
    {
        if(element == nullptr)
            return nullptr;
        return std::dynamic_pointer_cast<Ui>(element->getTopLevelParent());
    }
    void run(Renderer &renderer);
    bool isDone() const
    {
        return done;
    }
    void quit()
    {
        done = true;
    }
    virtual void layout() override
    {
        minX = -Display::scaleX();
        maxX = Display::scaleX();
        minY = -Display::scaleY();
        maxY = Display::scaleY();
        Container::layout();
    }
    virtual void reset() override
    {
        done = false;
        minX = -Display::scaleX();
        maxX = Display::scaleX();
        minY = -Display::scaleY();
        maxY = Display::scaleY();
        Container::reset();
    }
    virtual bool handleQuit(QuitEvent &event) override
    {
        if(!Container::handleQuit(event))
            quit();
        return true;
    }
    virtual bool canHaveKeyboardFocus() const override
    {
        return true;
    }
    virtual void handleFinish()
    {
    }
protected:
    virtual void clear(Renderer &renderer);
};
}
}
}

#endif // UI_H_INCLUDED
