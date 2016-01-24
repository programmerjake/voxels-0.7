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
#ifndef BUTTON_H_INCLUDED
#define BUTTON_H_INCLUDED

#include "ui/element.h"
#include "util/monitored_variable.h"
#include <unordered_set>
#include "render/text.h"
#include <algorithm>
#include "render/generate.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class GenericButton : public Element
{
private:
    std::unordered_set<MouseButton> mouseButtons;
    std::unordered_set<KeyboardKey> keysPressed;
    std::unordered_set<int> touchs;
    bool disabled;
    bool isPressed()
    {
        if(disabled)
            return false;
        return !mouseButtons.empty() || !keysPressed.empty() || !touchs.empty();
    }
    void updatePressed(bool doClick)
    {
        bool v = isPressed();
        if(v != pressed.get())
        {
            pressed.set(v);
            if(doClick && !disabled)
            {
                setFocus();
                EventArguments args;
                click(args);
            }
        }
    }
public:
    MonitoredBool pressed;
    Event click;
    bool isDisabled() const
    {
        return disabled;
    }
    void setDisabled(bool disabled)
    {
        this->disabled = disabled;
        if(disabled)
        {
            mouseButtons.clear();
            keysPressed.clear();
            touchs.clear();
        }
        updatePressed(false);
        if(disabled)
        {
            std::shared_ptr<Container> parent = getParent();
            if(parent != nullptr)
            {
                parent->removeFocus(shared_from_this());
            }
        }
    }
    GenericButton(float minX, float maxX, float minY, float maxY, bool disabled = false)
        : Element(minX, maxX, minY, maxY),
        mouseButtons(),
        keysPressed(),
        touchs(),
        disabled(disabled),
        pressed(false),
        click()
    {
    }
    virtual bool canHaveKeyboardFocus() const override
    {
        return !disabled;
    }
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        touchs.erase(event.touchId);
        updatePressed(true);
        return true;
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        touchs.insert(event.touchId);
        updatePressed(false);
        return true;
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        return true;
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        mouseButtons.erase(event.button);
        updatePressed(false);
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        mouseButtons.insert(event.button);
        updatePressed(true);
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent &event) override
    {
        if(keysPressed.erase(event.key) > 0)
        {
            updatePressed(false);
            return true;
        }
        updatePressed(false);
        return false;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        if(event.key == KeyboardKey::Space || event.key == KeyboardKey::Return)
        {
            keysPressed.insert(event.key);
            updatePressed(true);
            return true;
        }
        updatePressed(false);
        return false;
    }
    virtual bool handleMouseMoveOut(MouseEvent &event) override
    {
        mouseButtons.clear();
        updatePressed(false);
        return true;
    }
    virtual bool handleMouseMoveIn(MouseEvent &event) override
    {
        updatePressed(false);
        return true;
    }
    virtual bool handleTouchMoveOut(TouchEvent &event) override
    {
        touchs.erase(event.touchId);
        updatePressed(false);
        return true;
    }
    virtual bool handleTouchMoveIn(TouchEvent &event) override
    {
        updatePressed(false);
        return true;
    }
    virtual void reset() override
    {
        mouseButtons.clear();
        keysPressed.clear();
        touchs.clear();
        updatePressed(false);
    }
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override = 0;
};

class InvisibleButton : public GenericButton
{
public:
    InvisibleButton(float minX, float maxX, float minY, float maxY)
        : GenericButton(minX, maxX, minY, maxY)
    {
    }
    virtual bool canHaveKeyboardFocus() const override
    {
        return false;
    }
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
    }
};

class Button : public GenericButton
{
public:
    std::wstring text;
    Text::TextProperties textProperties;
    ColorF textColor, selectedTextColor, pressedTextColor;
    ColorF disabledTextColor;
    ColorF color, selectedColor, pressedColor;
    Button(std::wstring text, float minX, float maxX, float minY, float maxY, ColorF color, ColorF textColor, ColorF selectedTextColor, ColorF pressedTextColor, ColorF disabledTextColor, ColorF selectedColor, ColorF pressedColor, Text::TextProperties textProperties = Text::defaultTextProperties)
        : GenericButton(minX, maxX, minY, maxY),
        text(text),
        textProperties(textProperties),
        textColor(textColor),
        selectedTextColor(selectedTextColor),
        pressedTextColor(pressedTextColor),
        disabledTextColor(disabledTextColor),
        color(color),
        selectedColor(selectedColor),
        pressedColor(pressedColor)
    {
    }
    Button(std::wstring text, float minX, float maxX, float minY, float maxY, ColorF color = GrayscaleF(0.65), ColorF textColor = GrayscaleF(0), Text::TextProperties textProperties = Text::defaultTextProperties)
        : Button(text,
                 minX, maxX, minY, maxY,
                 color, textColor,
                 colorize(GrayscaleF(0.75), textColor), colorize(GrayscaleF(0.5), textColor), interpolate(0.5, GrayscaleF(0.5f), textColor),
                 interpolate(0.5, GrayscaleF(1), color), color, textProperties)
    {
    }
protected:
    void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus, bool isPressed)
    {
        float backgroundZ = 0.5f * (minZ + maxZ);
        float spacing = std::min<float>(0.15f * (maxX - minX), 0.25f * (maxY - minY));
        ColorF currentTextColor = textColor;
        ColorF topColor = color;
        if(isDisabled())
        {
            currentTextColor = disabledTextColor;
        }
        else if(hasFocus)
        {
            currentTextColor = selectedTextColor;
            topColor = selectedColor;
        }
        ColorF bottomColor = colorize(GrayscaleF(0.7f), topColor);
        float textWidth = Text::width(text, textProperties);
        float textHeight = Text::height(text, textProperties);
        if(textWidth == 0)
            textWidth = 1;
        if(textHeight == 0)
            textHeight = 1;
        float textScale = (maxY - minY - 2 * spacing) / textHeight;
        textScale = std::min<float>(textScale, (maxX - minX - 2 * spacing) / textWidth);
        if(isPressed && !isDisabled())
        {
            currentTextColor = pressedTextColor;
            bottomColor = pressedColor;
            topColor = colorize(GrayscaleF(0.7f), pressedColor);
            textScale *= 0.9f;
        }
        renderer << Generate::quadrilateral(whiteTexture(),
                                                  VectorF(minX * backgroundZ, minY * backgroundZ, -backgroundZ), bottomColor,
                                                  VectorF(maxX * backgroundZ, minY * backgroundZ, -backgroundZ), bottomColor,
                                                  VectorF(maxX * backgroundZ, maxY * backgroundZ, -backgroundZ), topColor,
                                                  VectorF(minX * backgroundZ, maxY * backgroundZ, -backgroundZ), topColor);
        float xOffset = -0.5f * textWidth, yOffset = -0.5f * textHeight;
        xOffset = textScale * xOffset + 0.5f * (minX + maxX);
        yOffset = textScale * yOffset + 0.5f * (minY + maxY);
        renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(text, currentTextColor, textProperties));
    }
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        render(renderer, minZ, maxZ, hasFocus, pressed.get());
    }
};
}
}
}

#endif // BUTTON_H_INCLUDED
