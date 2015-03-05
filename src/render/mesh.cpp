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
#include "render/mesh.h"
#include "render/renderer.h"
#include "platform/platform.h"
#include "render/generate.h"
#include <iostream>
#include <cassert>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
void programmerjake::voxels::Renderer::render(const Mesh & m, Matrix tform)
{
    Display::render(m, tform, depthBufferEnabled);
}

namespace Generate
{
namespace
{
bool isPixelTransparent(ColorI c)
{
    return c.a < 0x80;
}
}

Mesh item3DImage(TextureDescriptor td, float thickness)
{
    assert(td);
    assert(td.maxU <= 1 && td.minU >= 0 && td.maxV <= 1 && td.minV >= 0);
    Image image = td.image;
    unsigned iw = image.width();
    unsigned ih = image.height();
    int minX = ifloor(1e-5 + iw * td.minU);
    int maxX = ifloor(-1e-5 + iw * td.maxU);
    int minY = ifloor(1e-5 + ih * (1 - td.maxV));
    int maxY = ifloor(-1e-5 + ih * (1 - td.minV));
    assert(minX <= maxX && minY <= maxY);
    float scale = std::min<float>(1.0f / (1 + maxX - minX), 1.0f / (1 + maxY - minY));
    if(thickness <= 0)
        thickness = scale;
    TextureDescriptor backTD = td;
    backTD.maxU = td.minU;
    backTD.minU = td.maxU;
    Mesh retval = (Mesh)transform(Matrix::translate(0, 0, -0.5f).concat(Matrix::scale(1, 1, thickness)), unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), backTD, td));
    for(int iy = minY; iy <= maxY; iy++)
    {
        for(int ix = minX; ix <= maxX; ix++)
        {
            bool transparent = isPixelTransparent(image.getPixel(ix, iy));
            if(transparent)
                continue;
            bool transparentNIX = true, transparentPIX = true, transparentNIY = true, transparentPIY = true; // image transparent for negative and positive image coordinates
            if(ix > minX)
                transparentNIX = isPixelTransparent(image.getPixel(ix - 1, iy));
            if(ix < maxX)
                transparentPIX = isPixelTransparent(image.getPixel(ix + 1, iy));
            if(iy > minY)
                transparentNIY = isPixelTransparent(image.getPixel(ix, iy - 1));
            if(iy < maxY)
                transparentPIY = isPixelTransparent(image.getPixel(ix, iy + 1));
            float pixelU = (ix + 0.5f) / iw;
            float pixelV = 1.0f - (iy + 0.5f) / ih;
            TextureDescriptor pixelTD = TextureDescriptor(image, pixelU, pixelU, pixelV, pixelV);
            TextureDescriptor nx, px, ny, py;
            if(transparentNIX)
                nx = pixelTD;
            if(transparentPIX)
                px = pixelTD;
            if(transparentNIY)
                py = pixelTD;
            if(transparentPIY)
                ny = pixelTD;
            Matrix tform = Matrix::translate(ix - minX, maxY - iy, -0.5f).concat(Matrix::scale(scale, scale, thickness));
            retval.append(transform(tform, unitBox(nx, px, ny, py, TextureDescriptor(), TextureDescriptor())));
        }
    }
    return std::move(retval);
}
}
}
}
