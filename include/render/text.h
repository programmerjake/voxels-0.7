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
#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

#include "render/generate.h"
#include <cwchar>
#include <string>

namespace programmerjake
{
namespace voxels
{
namespace Text
{
class FontDescriptor;
typedef const FontDescriptor *FontDescriptorPointer;
struct Font final
{
    Font(const Font &) = default;
    Font &operator =(const Font &) = default;
    FontDescriptorPointer descriptor;
    explicit Font(FontDescriptorPointer descriptor = nullptr)
        : descriptor(descriptor)
    {
    }
};
inline Font getDefaultFont()
{
    return Font();
}
Font getBitmappedFont8x8();
Font getVectorFont();
struct TextProperties final
{
    float tabWidth = 8;
    Font font = getDefaultFont();
};
extern const TextProperties defaultTextProperties;
float width(std::wstring str, const TextProperties & properties = defaultTextProperties);
float height(std::wstring str, const TextProperties & properties = defaultTextProperties);
float xPos(std::wstring str, const TextProperties & properties = defaultTextProperties);
float yPos(std::wstring str, const TextProperties & properties = defaultTextProperties);
Mesh mesh(std::wstring str, ColorF color = colorizeIdentity(), const TextProperties & properties = defaultTextProperties);
}
}
}

#endif // TEXT_H_INCLUDED
