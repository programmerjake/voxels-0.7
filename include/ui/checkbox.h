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
#ifndef CHECKBOX_H_INCLUDED
#define CHECKBOX_H_INCLUDED

#include "ui/labeled_element.h"
#include "util/monitored_variable.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class CheckBox : public LabeledElement
{
private:
    std::unordered_set<MouseButton> mouseButtons;
    std::unordered_set<KeyboardKey> keysPressed;
    std::unordered_set<int> touchs;
    bool isPressed()
    {
        return !mouseButtons.empty() || !keysPressed.empty() || !touchs.empty();
    }
    void updatePressed(bool doToggle)
    {
        bool v = isPressed();
        if(v != pressed)
        {
            pressed = v;
            if(doToggle)
            {
                setFocus();
                checked.set(!checked.get());
            }
        }
    }
    bool pressed;
public:
    MonitoredBool checked;
    ColorF checkboxColor, selectedCheckboxColor;
    CheckBox(std::wstring text, float minX, float maxX, float minY, float maxY, bool checked = false, ColorF textColor = GrayscaleF(0), ColorF checkboxColor = GrayscaleF(0), ColorF selectedCheckboxColor = RGBF(0, 0, 1), Text::TextProperties textProperties = Text::defaultTextProperties)
        : LabeledElement(text, minX, maxX, minY, maxY, textColor, textProperties),
        mouseButtons(),
        keysPressed(),
        touchs(),
        pressed(false),
        checked(checked),
        checkboxColor(checkboxColor),
        selectedCheckboxColor(selectedCheckboxColor)
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
    virtual float elementWidth() override
    {
        return elementHeight();
    }
    virtual float elementHeight() override
    {
        return std::min<float>(maxY - minY, 0.25f * (maxX - minX));
    }
    virtual void renderElement(Renderer &renderer, float minX, float maxX, float minY, float maxY, float minZ, float maxZ, bool isSelected) override
    {
        std::wstring str = L"\u2610";
        if(checked.get())
            str = L"\u2611";
        float textWidth = Text::width(str);
        float textHeight = Text::height(str);
        if(textWidth == 0)
            textWidth = 1;
        if(textHeight == 0)
            textHeight = 1;
        float textScale = (maxY - minY) / textHeight;
        textScale = std::min<float>(textScale, (maxX - minX) / textWidth);
        float xOffset = -0.5f * textWidth, yOffset = -0.5f * textHeight;
        xOffset = textScale * xOffset + 0.5f * (minX + maxX);
        yOffset = textScale * yOffset + 0.5f * (minY + maxY);
        renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(str, isSelected ? selectedCheckboxColor : checkboxColor));
    }
};
}
}
}


#endif // CHECKBOX_H_INCLUDED
