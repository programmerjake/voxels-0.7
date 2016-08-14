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
#ifndef IMAGE_ELEMENT_H_INCLUDED
#define IMAGE_ELEMENT_H_INCLUDED

#include "ui/element.h"
#include <cwchar>
#include <string>
#include <algorithm>
#include "texture/texture_descriptor.h"
#include "render/generate.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class ImageElement : public Element
{
public:
    TextureDescriptor image;
    ColorF colorizeColor;
    ImageElement(TextureDescriptor image,
                 float minX,
                 float maxX,
                 float minY,
                 float maxY,
                 ColorF colorizeColor = colorizeIdentity())
        : Element(minX, maxX, minY, maxY), image(image), colorizeColor(colorizeColor)
    {
    }
    virtual bool canHaveKeyboardFocus() const override
    {
        return false;
    }

protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        if(image)
        {
            renderer << Generate::quadrilateral(image,
                                                VectorF(minX * minZ, minY * minZ, -minZ),
                                                colorizeColor,
                                                VectorF(maxX * minZ, minY * minZ, -minZ),
                                                colorizeColor,
                                                VectorF(maxX * minZ, maxY * minZ, -minZ),
                                                colorizeColor,
                                                VectorF(minX * minZ, maxY * minZ, -minZ),
                                                colorizeColor);
        }
    }
};
}
}
}

#endif // IMAGE_ELEMENT_H_INCLUDED
