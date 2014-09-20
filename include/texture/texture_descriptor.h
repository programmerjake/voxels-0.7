/*
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
#ifndef TEXTURE_DESCRIPTOR_H_INCLUDED
#define TEXTURE_DESCRIPTOR_H_INCLUDED

#include "texture/image.h"
#include "util/util.h"

struct TextureDescriptor
{
    Image image;
    float minU, maxU, minV, maxV;
    TextureDescriptor(Image image = Image())
        : image(image), minU(0), maxU(1), minV(0), maxV(1)
    {
    }
    TextureDescriptor(Image image, float minU, float maxU, float minV, float maxV)
        : image(image), minU(minU), maxU(maxU), minV(minV), maxV(maxV)
    {
    }
    operator bool() const
    {
        return (bool)image;
    }
    bool operator !() const
    {
        return !image;
    }
    TextureDescriptor subTexture(const float minU, const float maxU, const float minV, const float maxV) const
    {
        return TextureDescriptor(image,
                                 interpolate(minU, this->minU, this->maxU),
                                 interpolate(maxU, this->minU, this->maxU),
                                 interpolate(minV, this->minV, this->maxV),
                                 interpolate(maxV, this->minV, this->maxV));
    }
};

#endif // TEXTURE_DESCRIPTOR_H_INCLUDED
