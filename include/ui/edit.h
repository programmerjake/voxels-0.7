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
#ifndef EDIT_H_INCLUDED
#define EDIT_H_INCLUDED

#include "ui/element.h"
#include "util/monitored_variable.h"
#include "render/text.h"
#include <unordered_set>
#include "util/color.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class Edit : public Element
{
private:
    float cursorBlinkPhase = 0.0f;
    std::unordered_set<MouseButton> mouseButtons;
    std::unordered_set<KeyboardKey> keysPressed;
    std::unordered_set<int> touchs;
    void handleKey(KeyboardKey key);
    std::wstring currentEditingText = L"";
    std::size_t currentEditingCursorPosition = 0;
    std::size_t currentEditingSelectionLength = 0;
public:
    MonitoredString text;
    Event enter;
    std::size_t cursorPosition;
    std::function<std::wstring (std::wstring text)> filterTextFunction;
    Text::TextProperties textProperties;
    float cursorBlinkPeriod = 1.0f;
    /** the cursorBlinkPeriod value that means do not blink.
     */
    static constexpr float doNotBlink = 0.0f;
    ColorF textColor = GrayscaleF(0.0f);
    ColorF cursorColor = GrayscaleF(0.0f);
    ColorF editUnderlineColor = GrayscaleF(0.0f);
    ColorF backgroundColor = GrayscaleF(1.0f);
    float cursorWidth = 0.0625f;
    float textHeightFraction = 0.6f;
    explicit Edit(float minX, float maxX, float minY, float maxY);
    virtual bool canHaveKeyboardFocus() const override
    {
        return true;
    }
    virtual bool handleTouchUp(TouchUpEvent &event) override;
    virtual bool handleTouchDown(TouchDownEvent &event) override;
    virtual bool handleTouchMove(TouchMoveEvent &event) override;
    virtual bool handleMouseUp(MouseUpEvent &event) override;
    virtual bool handleMouseDown(MouseDownEvent &event) override;
    virtual bool handleMouseMove(MouseMoveEvent &event) override;
    virtual bool handleKeyUp(KeyUpEvent &event) override;
    virtual bool handleKeyDown(KeyDownEvent &event) override;
    virtual bool handleTextInput(TextInputEvent &event) override;
    virtual bool handleTextEdit(TextEditEvent &event) override;
    virtual bool handleMouseMoveOut(MouseEvent &event) override;
    virtual bool handleMouseMoveIn(MouseEvent &event) override;
    virtual bool handleTouchMoveOut(TouchEvent &event) override;
    virtual bool handleTouchMoveIn(TouchEvent &event) override;
    virtual void reset() override;
    virtual void move(double deltaTime) override;
    virtual void handleFocusChange(bool gettingFocus) override;
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override;
};
}
}
}

#endif // EDIT_H_INCLUDED
