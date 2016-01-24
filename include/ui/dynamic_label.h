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
#ifndef DYNAMIC_LABEL_H_INCLUDED
#define DYNAMIC_LABEL_H_INCLUDED

#include "ui/label.h"
#include <cwchar>
#include <string>
#include <algorithm>
#include <functional>
#include "render/text.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class DynamicLabel : public Label
{
    std::function<std::wstring(double deltaTime)> textFn;
public:
    DynamicLabel(std::function<std::wstring(double deltaTime)> textFn, float minX, float maxX, float minY, float maxY, ColorF textColor = GrayscaleF(0), Text::TextProperties textProperties = Text::defaultTextProperties)
        : Label(L"uninitialized", minX, maxX, minY, maxY, textColor, textProperties), textFn(textFn)
    {
    }
    virtual void move(double deltaTime) override
    {
        if(textFn)
            text = textFn(deltaTime);
        else
            text = L"nullptr";
        Label::move(deltaTime);
    }
};
}
}
}

#endif // DYNAMIC_LABEL_H_INCLUDED
