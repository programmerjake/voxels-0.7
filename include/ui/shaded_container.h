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
#ifndef SHADED_CONTAINER_H_INCLUDED
#define SHADED_CONTAINER_H_INCLUDED

#include "ui/container.h"
#include "render/generate.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class ShadedContainer : public Container
{
public:
    ColorF background;
    ShadedContainer(float minX, float maxX, float minY, float maxY, ColorF background = GrayscaleAF(0.5f, 0.75f))
        : Container(minX, maxX, minY, maxY), background(background)
    {
    }
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        float backgroundZ = interpolate<float>(0.9, minZ, maxZ);
        Container::render(renderer, minZ, backgroundZ, hasFocus);
        renderer << Generate::quadrilateral(whiteTexture(),
                                                  VectorF(minX * backgroundZ, minY * backgroundZ, -backgroundZ), background,
                                                  VectorF(maxX * backgroundZ, minY * backgroundZ, -backgroundZ), background,
                                                  VectorF(maxX * backgroundZ, maxY * backgroundZ, -backgroundZ), background,
                                                  VectorF(minX * backgroundZ, maxY * backgroundZ, -backgroundZ), background);
    }
};
}
}
}

#endif // SHADED_CONTAINER_H_INCLUDED
