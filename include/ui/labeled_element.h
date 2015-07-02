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
#ifndef LABELED_ELEMENT_H_INCLUDED
#define LABELED_ELEMENT_H_INCLUDED

#include "ui/element.h"
#include "render/text.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class LabeledElement : public Element
{
public:
    std::wstring text;
    ColorF textColor;
    Text::TextProperties textProperties;
    LabeledElement(std::wstring text, float minX, float maxX, float minY, float maxY, ColorF textColor = GrayscaleF(0), Text::TextProperties textProperties = Text::defaultTextProperties)
        : Element(minX, maxX, minY, maxY), text(text), textColor(textColor), textProperties(textProperties)
    {
    }
protected:
    virtual float elementWidth() = 0;
    virtual float elementHeight() = 0;
    virtual void renderElement(Renderer &renderer, float minX, float maxX, float minY, float maxY, float minZ, float maxZ, bool isSelected) = 0;
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool isSelected) override
    {
        float newMinX = minX + elementWidth();
        float textWidth = Text::width(text, textProperties);
        float textHeight = Text::height(text, textProperties);
        if(textWidth == 0)
            textWidth = 1;
        if(textHeight == 0)
            textHeight = 1;
        float textScale = (maxY - minY) / textHeight;
        textScale = std::min<float>(textScale, (maxX - newMinX) / textWidth);
        float xOffset = -0.5f * textWidth, yOffset = -0.5f * textHeight;
        float centerY = 0.5f * (minY + maxY);
        xOffset = textScale * xOffset + 0.5f * (newMinX + maxX);
        yOffset = textScale * yOffset + centerY;
        renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(text, textColor, textProperties));
        float eHeight = elementHeight();
        renderElement(renderer, minX, newMinX, centerY - 0.5f * eHeight, centerY + 0.5f * eHeight, interpolate<float>(0.125f, minZ, maxZ), maxZ, isSelected);
    }
};
}
}
}

#endif // LABELED_ELEMENT_H_INCLUDED
