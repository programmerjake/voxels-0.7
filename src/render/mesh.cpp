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
#include "render/mesh.h"
#include "render/renderer.h"
#include "platform/platform.h"
#include "render/generate.h"
#include <iostream>
#include <cassert>
#include "util/util.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
struct Renderer::Implementation final
{
    Mesh buffer = Mesh();
    RenderLayer bufferRenderLayer = RenderLayer();
    Matrix bufferTransform = Matrix::identity();
    bool isTransformIdentity = true;
    static bool isBufferSizeBig(std::size_t bufferSize)
    {
        return bufferSize >= 5000;
    }
    static bool isBufferSizeSmall(std::size_t bufferSize)
    {
        return bufferSize < 50;
    }
    void flush()
    {
        if(buffer.size() != 0)
        {
            Display::render(buffer, bufferTransform, bufferRenderLayer);
        }
        buffer.clear();
    }
    void render(const Mesh &m, Matrix tform, RenderLayer rl)
    {
        if(m.size() == 0)
            return;
        if(buffer.size() != 0)
        {
            if(!buffer.isAppendable(m) || bufferRenderLayer != rl)
            {
                flush();
            }
            else if(isBufferSizeBig(buffer.size()))
            {
                if(isTransformIdentity && isBufferSizeSmall(m.size()))
                {
                    buffer.append(m, tform);
                    flush();
                    return;
                }
                else if(bufferTransform == tform && isBufferSizeSmall(m.size()))
                {
                    buffer.append(m);
                    flush();
                    return;
                }
                else
                {
                    flush();
                }
            }
            else if(tform == bufferTransform)
            {
                buffer.append(m);
                return;
            }
            else if(isTransformIdentity)
            {
                buffer.append(m, tform);
                return;
            }
            else if(isBufferSizeSmall(buffer.size()))
            {
                buffer = Mesh(std::move(buffer), bufferTransform);
                bufferTransform = Matrix::identity();
                isTransformIdentity = true;
                buffer.append(m, tform);
                return;
            }
            else
            {
                flush();
            }
        }
        assert(buffer.size() == 0);
        if(isBufferSizeBig(m.size()))
        {
            Display::render(m, tform, rl);
            return;
        }
        buffer.append(m);
        bufferTransform = tform;
        bufferRenderLayer = rl;
        isTransformIdentity = (tform == Matrix::identity());
    }
};

void Renderer::render(const Mesh &m, Matrix tform)
{
    implementation->render(m, tform, currentRenderLayer);
}

void Renderer::render(const MeshBuffer &m)
{
    implementation->flush();
    Display::render(m, currentRenderLayer);
}

void Renderer::flush()
{
    implementation->flush();
}

Renderer Renderer::make()
{
    return Renderer(std::make_shared<Implementation>());
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
    float fMinX = iw * td.minU;
    float fMaxX = iw * td.maxU;
    float fMinY = ih * (1 - td.maxV);
    float fMaxY = ih * (1 - td.minV);
    int minX = ifloor(1e-5f + fMinX);
    int maxX = ifloor(-1e-5f + fMaxX);
    int minY = ifloor(1e-5f + fMinY);
    int maxY = ifloor(-1e-5f + fMaxY);
    assert(minX <= maxX && minY <= maxY);
    float scale = std::min<float>(1.0f / (1 + maxX - minX), 1.0f / (1 + maxY - minY));
    if(thickness <= 0)
        thickness = scale;
    TextureDescriptor backTD = td;
    backTD.maxU = td.minU;
    backTD.minU = td.maxU;
    float faceMinX = scale * (fMinX - minX);
    float faceMinY = scale * (maxY - fMaxY + 1);
    float faceMaxX = faceMinX + scale * (fMaxX - fMinX);
    float faceMaxY = faceMinY + scale * (fMaxY - fMinY);
    Mesh retval =
        transform(Matrix::translate(faceMinX, faceMinY, -0.5f)
                      .concat(Matrix::scale(faceMaxX - faceMinX, faceMaxY - faceMinY, thickness)),
                  unitBox(TextureDescriptor(),
                          TextureDescriptor(),
                          TextureDescriptor(),
                          TextureDescriptor(),
                          backTD,
                          td));
    for(int iy = minY; iy <= maxY; iy++)
    {
        for(int ix = minX; ix <= maxX; ix++)
        {
            bool transparent = isPixelTransparent(image.getPixel(ix, iy));
            if(transparent)
                continue;
            bool transparentNIX = true, transparentPIX = true, transparentNIY = true,
                 transparentPIY =
                     true; // image transparent for negative and positive image coordinates
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
            Matrix tform = Matrix::translate(static_cast<float>(ix - minX),
                                             static_cast<float>(maxY - iy),
                                             -0.5f).concat(Matrix::scale(scale, scale, thickness));
            retval.append(transform(
                tform, unitBox(nx, px, ny, py, TextureDescriptor(), TextureDescriptor())));
        }
    }
    return std::move(retval);
}

Mesh itemDamage(float damageValue)
{
    damageValue = limit<float>(damageValue, 0, 1);
    if(damageValue == 0)
        return Mesh();
    TextureDescriptor backgroundTexture = TextureAtlas::DamageBarGray.td();
    TextureDescriptor foregroundTexture = TextureAtlas::DamageBarGreen.td();
    if(damageValue > 1.0f / 3)
    {
        foregroundTexture = TextureAtlas::DamageBarYellow.td();
    }
    if(damageValue > 2.0f / 3)
    {
        foregroundTexture = TextureAtlas::DamageBarRed.td();
    }
    const float minX = 2 / 16.0f;
    const float maxX = 14 / 16.0f;
    float splitX = interpolate(damageValue, maxX, minX);
    const float minY = 2 / 16.0f;
    const float maxY = 4 / 16.0f;
    constexpr ColorF c = colorizeIdentity();
    const VectorF nxny = VectorF(minX, minY, 0);
    VectorF cxny = VectorF(splitX, minY, 0);
    const VectorF pxny = VectorF(maxX, minY, 0);
    const VectorF nxpy = VectorF(minX, maxY, 0);
    VectorF cxpy = VectorF(splitX, maxY, 0);
    const VectorF pxpy = VectorF(maxX, maxY, 0);
    Mesh retval = quadrilateral(foregroundTexture, nxny, c, cxny, c, cxpy, c, nxpy, c);
    retval.append(quadrilateral(backgroundTexture, cxny, c, pxny, c, pxpy, c, cxpy, c));
    return retval;
}
}
}
}
