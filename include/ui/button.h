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
    bool isPressed()
    {
        return !mouseButtons.empty() || !keysPressed.empty() || !touchs.empty();
    }
    void updatePressed(bool doClick)
    {
        bool v = isPressed();
        if(v != pressed.get())
        {
            pressed.set(v);
            if(doClick)
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
    GenericButton(float minX, float maxX, float minY, float maxY)
        : Element(minX, maxX, minY, maxY), pressed(false)
    {
    }
    virtual bool canHaveKeyboardFocus() const override
    {
        return true;
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
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override = 0;
};

class Button : public GenericButton
{
public:
    std::wstring text;
    Text::TextProperties textProperties;
    ColorF textColor, selectedTextColor, pressedTextColor, color, selectedColor, pressedColor;
    Button(std::wstring text, float minX, float maxX, float minY, float maxY, ColorF color, ColorF textColor, ColorF selectedTextColor, ColorF pressedTextColor, ColorF selectedColor, ColorF pressedColor, Text::TextProperties textProperties = Text::defaultTextProperties)
        : GenericButton(minX, maxX, minY, maxY), text(text), textProperties(textProperties), textColor(textColor), selectedTextColor(selectedTextColor), pressedTextColor(pressedTextColor), color(color), selectedColor(selectedColor), pressedColor(pressedColor)
    {
    }
    Button(std::wstring text, float minX, float maxX, float minY, float maxY, ColorF color = GrayscaleF(0.65), ColorF textColor = GrayscaleF(0), Text::TextProperties textProperties = Text::defaultTextProperties)
        : Button(text, minX, maxX, minY, maxY, color, textColor, colorize(GrayscaleF(0.75), textColor), colorize(GrayscaleF(0.5), textColor), interpolate(0.5, GrayscaleF(1), color), color, textProperties)
    {
    }
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        float backgroundZ = 0.5f * (minZ + maxZ);
        float spacing = std::min<float>(0.15 * (maxX - minX), 0.25 * (maxY - minY));
        ColorF currentTextColor = textColor;
        ColorF topColor = color;
        if(hasFocus)
        {
            currentTextColor = selectedTextColor;
            topColor = selectedColor;
        }
        ColorF bottomColor = colorize(GrayscaleF(0.7), topColor);
        float textWidth = Text::width(text, textProperties);
        float textHeight = Text::height(text, textProperties);
        if(textWidth == 0)
            textWidth = 1;
        if(textHeight == 0)
            textHeight = 1;
        float textScale = 0.5 * (maxY - minY - 2 * spacing) / textHeight;
        textScale = std::min<float>(textScale, (maxX - minX - 2 * spacing) / textWidth);
        if(pressed.get())
        {
            currentTextColor = pressedTextColor;
            bottomColor = pressedColor;
            topColor = colorize(GrayscaleF(0.7), pressedColor);
            textScale *= 0.9;
        }
        renderer << Generate::quadrilateral(whiteTexture(),
                                                  VectorF(minX * backgroundZ, minY * backgroundZ, -backgroundZ), bottomColor,
                                                  VectorF(maxX * backgroundZ, minY * backgroundZ, -backgroundZ), bottomColor,
                                                  VectorF(maxX * backgroundZ, maxY * backgroundZ, -backgroundZ), topColor,
                                                  VectorF(minX * backgroundZ, maxY * backgroundZ, -backgroundZ), topColor);
        float xOffset = -0.5 * textWidth, yOffset = -0.5 * textHeight;
        xOffset = textScale * xOffset + 0.5 * (minX + maxX);
        yOffset = textScale * yOffset + 0.5 * (minY + maxY);
        renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(text, currentTextColor, textProperties));
    }
};
}
}
}

#endif // BUTTON_H_INCLUDED
