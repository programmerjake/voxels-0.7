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
#ifndef LABEL_H_INCLUDED
#define LABEL_H_INCLUDED

#include "ui/element.h"
#include <cwchar>
#include <string>
#include <algorithm>
#include "render/text.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class Label : public Element
{
public:
    std::wstring text;
    Text::TextProperties textProperties;
    ColorF textColor;
    Label(std::wstring text, float minX, float maxX, float minY, float maxY, ColorF textColor = GrayscaleF(0), Text::TextProperties textProperties = Text::defaultTextProperties)
        : Element(minX, maxX, minY, maxY), text(text), textProperties(textProperties), textColor(textColor)
    {
    }
    virtual bool canHaveKeyboardFocus() const override
    {
        return false;
    }
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        float textWidth = Text::width(text, textProperties);
        float textHeight = Text::height(text, textProperties);
        if(textWidth == 0)
            textWidth = 1;
        if(textHeight == 0)
            textHeight = 1;
        float textScale = (maxY - minY) / textHeight;
        textScale = std::min<float>(textScale, (maxX - minX) / textWidth);
        float xOffset = -0.5f * textWidth, yOffset = -0.5f * textHeight;
        xOffset = textScale * xOffset + 0.5f * (minX + maxX);
        yOffset = textScale * yOffset + 0.5f * (minY + maxY);
        renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(text, textColor, textProperties));
    }
};
}
}
}

#endif // LABEL_H_INCLUDED
