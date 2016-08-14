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
#include "ui/edit.h"
#include "util/math_constants.h"
#include <cmath>
#include <utility>
#include "util/matrix.h"
#include "platform/platform.h"
#include "util/logging.h"
#include "texture/texture_atlas.h"
#include "render/generate.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
Edit::Edit(float minX, float maxX, float minY, float maxY)
    : Element(minX, maxX, minY, maxY),
      mouseButtons(),
      keysPressed(),
      touchs(),
      text(),
      enter(),
      cursorPosition(0),
      filterTextFunction(nullptr),
      textProperties()
{
    text.onChange.bind2v(
        [this]()
        {
            if(text.get().empty())
                cursorPosition = 0;
        },
        Event::Propagate);
}

bool Edit::handleTouchUp(TouchUpEvent &event)
{
    touchs.erase(event.touchId);
    return true;
}

bool Edit::handleTouchDown(TouchDownEvent &event)
{
    touchs.insert(event.touchId);
    setFocus();
    return true;
}

bool Edit::handleTouchMove(TouchMoveEvent &event)
{
    return true;
}

bool Edit::handleMouseUp(MouseUpEvent &event)
{
    mouseButtons.erase(event.button);
    return true;
}

bool Edit::handleMouseDown(MouseDownEvent &event)
{
    mouseButtons.insert(event.button);
    setFocus();
    return true;
}

bool Edit::handleMouseMove(MouseMoveEvent &event)
{
    return true;
}

bool Edit::handleKeyUp(KeyUpEvent &event)
{
    if(keysPressed.erase(event.key) > 0)
    {
        return true;
    }
    return false;
}

bool Edit::handleKeyDown(KeyDownEvent &event)
{
    if(currentEditingText != L"")
    {
        keysPressed.insert(event.key);
        return true;
    }
    switch(event.key)
    {
    case KeyboardKey::A:
    case KeyboardKey::B:
    case KeyboardKey::C:
    case KeyboardKey::D:
    case KeyboardKey::E:
    case KeyboardKey::F:
    case KeyboardKey::G:
    case KeyboardKey::H:
    case KeyboardKey::I:
    case KeyboardKey::J:
    case KeyboardKey::K:
    case KeyboardKey::L:
    case KeyboardKey::M:
    case KeyboardKey::N:
    case KeyboardKey::O:
    case KeyboardKey::P:
    case KeyboardKey::Q:
    case KeyboardKey::R:
    case KeyboardKey::S:
    case KeyboardKey::T:
    case KeyboardKey::U:
    case KeyboardKey::V:
    case KeyboardKey::W:
    case KeyboardKey::X:
    case KeyboardKey::Y:
    case KeyboardKey::Z:
    case KeyboardKey::Backspace:
    case KeyboardKey::Space:
    case KeyboardKey::SQuote:
    case KeyboardKey::Equals:
    case KeyboardKey::Comma:
    case KeyboardKey::Dash:
    case KeyboardKey::BQuote:
    case KeyboardKey::LBracket:
    case KeyboardKey::RBracket:
    case KeyboardKey::BSlash:
    case KeyboardKey::Delete:
    case KeyboardKey::Period:
    case KeyboardKey::FSlash:
    case KeyboardKey::Num0:
    case KeyboardKey::Num1:
    case KeyboardKey::Num2:
    case KeyboardKey::Num3:
    case KeyboardKey::Num4:
    case KeyboardKey::Num5:
    case KeyboardKey::Num6:
    case KeyboardKey::Num7:
    case KeyboardKey::Num8:
    case KeyboardKey::Num9:
    case KeyboardKey::Colon:
    case KeyboardKey::Left:
    case KeyboardKey::Right:
    case KeyboardKey::Home:
    case KeyboardKey::End:
    case KeyboardKey::KPad0:
    case KeyboardKey::KPad1:
    case KeyboardKey::KPad2:
    case KeyboardKey::KPad3:
    case KeyboardKey::KPad4:
    case KeyboardKey::KPad5:
    case KeyboardKey::KPad6:
    case KeyboardKey::KPad7:
    case KeyboardKey::KPad8:
    case KeyboardKey::KPad9:
    case KeyboardKey::KPadFSlash:
    case KeyboardKey::KPadStar:
    case KeyboardKey::KPadDash:
    case KeyboardKey::KPadPlus:
    case KeyboardKey::KPadPeriod:
    case KeyboardKey::KPadEquals:
    case KeyboardKey::LShift:
    case KeyboardKey::RShift:
    case KeyboardKey::NumLock:
    case KeyboardKey::CapsLock:
    case KeyboardKey::ScrollLock:
        keysPressed.insert(event.key);
        handleKey(event.key);
        return true;
    case KeyboardKey::Return:
    case KeyboardKey::KPadReturn:
        enter();
        return false;
    case KeyboardKey::Clear:
    case KeyboardKey::Tab:
    case KeyboardKey::Unknown:
    case KeyboardKey::LCtrl:
    case KeyboardKey::RCtrl:
    case KeyboardKey::Pause:
    case KeyboardKey::Escape:
    case KeyboardKey::LAlt:
    case KeyboardKey::RAlt:
    case KeyboardKey::LMeta:
    case KeyboardKey::RMeta:
    case KeyboardKey::LSuper:
    case KeyboardKey::RSuper:
    case KeyboardKey::AltGr:
    case KeyboardKey::Menu:
    case KeyboardKey::Mode:
    case KeyboardKey::PrintScreen:
    case KeyboardKey::F1:
    case KeyboardKey::F2:
    case KeyboardKey::F3:
    case KeyboardKey::F4:
    case KeyboardKey::F5:
    case KeyboardKey::F6:
    case KeyboardKey::F7:
    case KeyboardKey::F8:
    case KeyboardKey::F9:
    case KeyboardKey::F10:
    case KeyboardKey::F11:
    case KeyboardKey::F12:
    case KeyboardKey::Up:
    case KeyboardKey::Down:
    case KeyboardKey::Insert:
    case KeyboardKey::PageUp:
    case KeyboardKey::PageDown:
        break;
    }
    return false;
}

bool Edit::handleTextInput(TextInputEvent &event)
{
    std::wstring str = text.get();
    if(cursorPosition > str.size())
    {
        cursorPosition = str.size();
    }
    str.insert(cursorPosition, event.text);
    cursorPosition += event.text.size();
    text.set(str);
    cursorBlinkPhase = 0.0f;
    return true;
}

bool Edit::handleTextEdit(TextEditEvent &event)
{
    cursorBlinkPhase = 0.0f;
    currentEditingText = event.text;
    currentEditingCursorPosition = event.start;
    currentEditingSelectionLength = event.length;
    Display::Text::start(minX, maxX, minY, maxY);
    return true;
}

bool Edit::handleMouseMoveOut(MouseEvent &event)
{
    mouseButtons.clear();
    return true;
}

bool Edit::handleMouseMoveIn(MouseEvent &event)
{
    return true;
}

bool Edit::handleTouchMoveOut(TouchEvent &event)
{
    touchs.erase(event.touchId);
    return true;
}

bool Edit::handleTouchMoveIn(TouchEvent &event)
{
    return true;
}

void Edit::handleFocusChange(bool gettingFocus)
{
    if(gettingFocus)
    {
        Display::Text::start(minX, maxX, minY, maxY);
        cursorBlinkPhase = 0.0f;
    }
    else
    {
        Display::Text::stop();
    }
}

void Edit::reset()
{
    cursorPosition = 0;
    cursorBlinkPhase = 0.0f;
    Display::Text::stop();
}

void Edit::move(double deltaTime)
{
    Element::move(deltaTime);
    if(cursorBlinkPeriod > doNotBlink)
    {
        double additionalPhase = deltaTime / cursorBlinkPeriod;
        additionalPhase -= std::floor(additionalPhase);
        cursorBlinkPhase += static_cast<float>(additionalPhase);
        cursorBlinkPhase -= std::floor(cursorBlinkPhase);
        if(std::isnan(cursorBlinkPhase) || !std::isfinite(cursorBlinkPhase))
            cursorBlinkPhase = 0.0f;
    }
    else
    {
        cursorBlinkPhase = 0.0f;
    }
}

void Edit::render(Renderer &renderer, float minZ, float maxZ, bool hasFocus)
{
    if(minX >= maxX || minY >= maxY)
        return;
    if(textHeightFraction < 0.05f)
        textHeightFraction = 0.05f;
    if(textHeightFraction > 1.0f)
        textHeightFraction = 1.0f;
    if(std::isnan(textHeightFraction) || !std::isfinite(textHeightFraction))
        textHeightFraction = 0.8f;
    float backgroundZ = interpolate<float>(0.75f, minZ, maxZ);
    float textZ = interpolate<float>(0.5f, minZ, maxZ);
    float underlineZ = interpolate<float>(0.25f, minZ, maxZ);
    float cursorZ = minZ;
    bool cursorOn = hasFocus && (cursorBlinkPhase < 0.5f);
    std::wstring filteredText = text.get();
    if(filterTextFunction)
        filteredText = filterTextFunction(static_cast<std::wstring>(std::move(filteredText)));
    std::size_t currentCursorPosition = cursorPosition;
    if(currentCursorPosition > filteredText.size())
        currentCursorPosition = filteredText.size();
    filteredText.insert(currentCursorPosition, currentEditingText);
    filteredText += L" "; // for space for cursor
    float editStartX = Text::xPos(filteredText, currentCursorPosition, textProperties);
    float editEndX =
        Text::xPos(filteredText, currentCursorPosition + currentEditingText.size(), textProperties);
    float cursorX = Text::xPos(
        filteredText, currentCursorPosition + currentEditingCursorPosition, textProperties);
    float cursorY = Text::yPos(
        filteredText, currentCursorPosition + currentEditingCursorPosition, textProperties);
    float textWidth = Text::width(filteredText, textProperties);
    float textHeight = Text::height(filteredText, textProperties);
    if(textWidth <= 0.0f || textHeight <= 0.0f)
    {
        textWidth = 1.0f;
        textHeight = 1.0f;
        cursorX = 0.0f;
        cursorY = 0.0f;
    }
    float controlWidth = maxX - minX;
    float controlHeight = maxY - minY;
    float controlCenterY = 0.5f * (minY + maxY);
    float adjustedCursorWidth = cursorWidth;
    float underlineHeight = 0.05f;
    float scale = controlHeight * textHeightFraction / textHeight;
    float pixelWidth = (Display::scaleX() * 2.0f) / static_cast<float>(Display::width()) / scale;
    if(adjustedCursorWidth < pixelWidth)
        adjustedCursorWidth = pixelWidth;
    float cursorMinX = cursorX;
    float cursorMidY = cursorY + 0.5f;
    float cursorHeight = interpolate(0.5f, 1.0f / textHeightFraction, 1.0f);
    float cursorMinY = cursorMidY - 0.5f * cursorHeight;
    float cursorMaxX = cursorMinX + adjustedCursorWidth;
    float cursorMaxY = cursorMidY + 0.5f * cursorHeight;
    float pixelHeight = (Display::scaleY() * 2.0f) / static_cast<float>(Display::height()) / scale;
    if(underlineHeight < pixelHeight)
        underlineHeight = pixelHeight;
    float underlineMinX = editStartX;
    float underlineMaxX = editEndX;
    float underlineMinY = cursorY;
    float underlineMaxY = underlineMinY + underlineHeight;
    float visibleTextWidth = controlWidth / scale;
    float unitWidth = Text::width(L"A", textProperties);
    float visibleTextLeft = cursorX - visibleTextWidth;
    float jumpWidth = (10.0f / 3.0f) * unitWidth;
    visibleTextLeft /= jumpWidth;
    visibleTextLeft = std::floor(visibleTextLeft);
    visibleTextLeft *= jumpWidth;
    visibleTextLeft += unitWidth * (1.0f / 3.0f) + jumpWidth;
    if(visibleTextLeft < 0)
        visibleTextLeft = 0;
    float translatedUnderlineMinX = underlineMinX - visibleTextLeft;
    float translatedUnderlineMaxX = underlineMaxX - visibleTextLeft;
    float translatedUnderlineMinY = underlineMinY;
    float translatedUnderlineMaxY = underlineMaxY;
    if(translatedUnderlineMinX < 0)
        translatedUnderlineMinX = 0; // clip to text box bounds
    if(translatedUnderlineMaxX > visibleTextWidth)
        translatedUnderlineMaxX = visibleTextWidth; // clip to text box bounds
    Transform textTransform = Transform::translate(0, -0.5f * textHeight, 0.0f)
                                  .concat(Transform::scale(scale))
                                  .concat(Transform::translate(minX, controlCenterY, -1.0f));
    Mesh mesh =
        Generate::quadrilateral(TextureAtlas::Blank.td(),
                                VectorF(minX * backgroundZ, minY * backgroundZ, -backgroundZ),
                                backgroundColor,
                                VectorF(maxX * backgroundZ, minY * backgroundZ, -backgroundZ),
                                backgroundColor,
                                VectorF(maxX * backgroundZ, maxY * backgroundZ, -backgroundZ),
                                backgroundColor,
                                VectorF(minX * backgroundZ, maxY * backgroundZ, -backgroundZ),
                                backgroundColor);
    Mesh textMesh = transform(Transform::translate(-visibleTextLeft, 0, 0),
                              Text::mesh(filteredText, textColor, textProperties));
    textMesh = cutAndGetBack(textMesh, VectorF(1, 0, 0), -visibleTextWidth);
    textMesh = cutAndGetBack(textMesh, VectorF(-1, 0, 0), 0);
    mesh.append(transform(textTransform.concat(Transform::scale(textZ)), textMesh));
    if(currentEditingText != L"")
    {
        mesh.append(transform(
            textTransform.concat(Transform::scale(underlineZ)),
            Generate::quadrilateral(TextureAtlas::Blank.td(),
                                    VectorF(translatedUnderlineMinX, translatedUnderlineMinY, 0.0f),
                                    editUnderlineColor,
                                    VectorF(translatedUnderlineMaxX, translatedUnderlineMinY, 0.0f),
                                    editUnderlineColor,
                                    VectorF(translatedUnderlineMaxX, translatedUnderlineMaxY, 0.0f),
                                    editUnderlineColor,
                                    VectorF(translatedUnderlineMinX, translatedUnderlineMaxY, 0.0f),
                                    editUnderlineColor)));
    }
    if(cursorOn)
    {
        mesh.append(transform(
            textTransform.concat(Transform::scale(cursorZ)),
            Generate::quadrilateral(TextureAtlas::Blank.td(),
                                    VectorF(cursorMinX - visibleTextLeft, cursorMinY, 0.0f),
                                    cursorColor,
                                    VectorF(cursorMaxX - visibleTextLeft, cursorMinY, 0.0f),
                                    cursorColor,
                                    VectorF(cursorMaxX - visibleTextLeft, cursorMaxY, 0.0f),
                                    cursorColor,
                                    VectorF(cursorMinX - visibleTextLeft, cursorMaxY, 0.0f),
                                    cursorColor)));
    }
    renderer << mesh;
}

void Edit::handleKey(KeyboardKey key)
{
    switch(key)
    {
    case KeyboardKey::A:
    case KeyboardKey::B:
    case KeyboardKey::C:
    case KeyboardKey::D:
    case KeyboardKey::E:
    case KeyboardKey::F:
    case KeyboardKey::G:
    case KeyboardKey::H:
    case KeyboardKey::I:
    case KeyboardKey::J:
    case KeyboardKey::K:
    case KeyboardKey::L:
    case KeyboardKey::M:
    case KeyboardKey::N:
    case KeyboardKey::O:
    case KeyboardKey::P:
    case KeyboardKey::Q:
    case KeyboardKey::R:
    case KeyboardKey::S:
    case KeyboardKey::T:
    case KeyboardKey::U:
    case KeyboardKey::V:
    case KeyboardKey::W:
    case KeyboardKey::X:
    case KeyboardKey::Y:
    case KeyboardKey::Z:
    case KeyboardKey::Space:
    case KeyboardKey::SQuote:
    case KeyboardKey::Equals:
    case KeyboardKey::Comma:
    case KeyboardKey::Dash:
    case KeyboardKey::BQuote:
    case KeyboardKey::LBracket:
    case KeyboardKey::RBracket:
    case KeyboardKey::BSlash:
    case KeyboardKey::Period:
    case KeyboardKey::FSlash:
    case KeyboardKey::Num0:
    case KeyboardKey::Num1:
    case KeyboardKey::Num2:
    case KeyboardKey::Num3:
    case KeyboardKey::Num4:
    case KeyboardKey::Num5:
    case KeyboardKey::Num6:
    case KeyboardKey::Num7:
    case KeyboardKey::Num8:
    case KeyboardKey::Num9:
    case KeyboardKey::Colon:
    case KeyboardKey::KPad0:
    case KeyboardKey::KPad1:
    case KeyboardKey::KPad2:
    case KeyboardKey::KPad3:
    case KeyboardKey::KPad4:
    case KeyboardKey::KPad5:
    case KeyboardKey::KPad6:
    case KeyboardKey::KPad7:
    case KeyboardKey::KPad8:
    case KeyboardKey::KPad9:
    case KeyboardKey::KPadFSlash:
    case KeyboardKey::KPadStar:
    case KeyboardKey::KPadDash:
    case KeyboardKey::KPadPlus:
    case KeyboardKey::KPadPeriod:
    case KeyboardKey::KPadEquals:
    case KeyboardKey::LShift:
    case KeyboardKey::RShift:
    case KeyboardKey::NumLock:
    case KeyboardKey::CapsLock:
    case KeyboardKey::ScrollLock:
    case KeyboardKey::Clear:
    case KeyboardKey::Tab:
    case KeyboardKey::Return:
    case KeyboardKey::KPadReturn:
    case KeyboardKey::Unknown:
    case KeyboardKey::LCtrl:
    case KeyboardKey::RCtrl:
    case KeyboardKey::Pause:
    case KeyboardKey::Escape:
    case KeyboardKey::LAlt:
    case KeyboardKey::RAlt:
    case KeyboardKey::LMeta:
    case KeyboardKey::RMeta:
    case KeyboardKey::LSuper:
    case KeyboardKey::RSuper:
    case KeyboardKey::AltGr:
    case KeyboardKey::Menu:
    case KeyboardKey::Mode:
    case KeyboardKey::PrintScreen:
    case KeyboardKey::F1:
    case KeyboardKey::F2:
    case KeyboardKey::F3:
    case KeyboardKey::F4:
    case KeyboardKey::F5:
    case KeyboardKey::F6:
    case KeyboardKey::F7:
    case KeyboardKey::F8:
    case KeyboardKey::F9:
    case KeyboardKey::F10:
    case KeyboardKey::F11:
    case KeyboardKey::F12:
    case KeyboardKey::Up:
    case KeyboardKey::Down:
    case KeyboardKey::Insert:
    case KeyboardKey::PageUp:
    case KeyboardKey::PageDown:
        return;
    case KeyboardKey::Backspace:
    {
        if(cursorPosition == 0)
            return;
        cursorPosition--;
        std::wstring str = text.get();
        str.erase(cursorPosition, 1);
        text.set(str);
        cursorBlinkPhase = 0.0f;
        return;
    }
    case KeyboardKey::Delete:
    {
        std::wstring str = text.get();
        if(cursorPosition >= str.size())
            return;
        str.erase(cursorPosition, 1);
        text.set(str);
        cursorBlinkPhase = 0.0f;
        return;
    }
    case KeyboardKey::Left:
    {
        if(cursorPosition == 0)
            return;
        cursorPosition--;
        cursorBlinkPhase = 0.0f;
        return;
    }
    case KeyboardKey::Right:
    {
        if(cursorPosition >= text.get().size())
            return;
        cursorPosition++;
        cursorBlinkPhase = 0.0f;
        return;
    }
    case KeyboardKey::Home:
        if(cursorPosition != 0)
            cursorBlinkPhase = 0.0f;
        cursorPosition = 0;
        return;
    case KeyboardKey::End:
    {
        std::size_t pos = text.get().size();
        if(pos != cursorPosition)
            cursorBlinkPhase = 0.0f;
        cursorPosition = pos;
        return;
    }
    }
}
}
}
}
