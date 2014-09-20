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
#include "render/text.h"
#include "texture/texture_atlas.h"
#include <unordered_map>
#include "util/util.h"
#include <iostream>

const Text::TextProperties Text::defaultTextProperties = Text::TextProperties();

namespace
{
const float charWidth = 1, charHeight = 1;
const TextureAtlas & Font = TextureAtlas::Font8x8;
const int fontWidth = 8, fontHeight = 8;
const int textureXRes = 128, textureYRes = 128;
const float pixelOffset = TextureAtlas::pixelOffset;

const wchar_t topPageTranslations[128] =
{
    0x00C7,
    0x00FC,
    0x00E9,
    0x00E2,
    0x00E4,
    0x00E0,
    0x00E5,
    0x00E7,
    0x00EA,
    0x00EB,
    0x00E8,
    0x00EF,
    0x00EE,
    0x00EC,
    0x00C4,
    0x00C5,
    0x00C9,
    0x00E6,
    0x00C6,
    0x00F4,
    0x00F6,
    0x00F2,
    0x00FB,
    0x00F9,
    0x00FF,
    0x00D6,
    0x00DC,
    0x00A2,
    0x00A3,
    0x00A5,
    0x20A7,
    0x0192,
    0x00E1,
    0x00ED,
    0x00F3,
    0x00FA,
    0x00F1,
    0x00D1,
    0x00AA,
    0x00BA,
    0x00BF,
    0x2310,
    0x00AC,
    0x00BD,
    0x00BC,
    0x00A1,
    0x00AB,
    0x00BB,
    0x2591,
    0x2592,
    0x2593,
    0x2502,
    0x2524,
    0x2561,
    0x2562,
    0x2556,
    0x2555,
    0x2563,
    0x2551,
    0x2557,
    0x255D,
    0x255C,
    0x255B,
    0x2510,
    0x2514,
    0x2534,
    0x252C,
    0x251C,
    0x2500,
    0x253C,
    0x255E,
    0x255F,
    0x255A,
    0x2554,
    0x2569,
    0x2566,
    0x2560,
    0x2550,
    0x256C,
    0x2567,
    0x2568,
    0x2564,
    0x2565,
    0x2559,
    0x2558,
    0x2552,
    0x2553,
    0x256B,
    0x256A,
    0x2518,
    0x250C,
    0x2588,
    0x2584,
    0x258C,
    0x2590,
    0x2580,
    0x03B1,
    0x00DF,
    0x0393,
    0x03C0,
    0x03A3,
    0x03C3,
    0x00B5,
    0x03C4,
    0x03A6,
    0x0398,
    0x03A9,
    0x03B4,
    0x221E,
    0x03C6,
    0x03B5,
    0x2229,
    0x2261,
    0x00B1,
    0x2265,
    0x2264,
    0x2320,
    0x2321,
    0x00F7,
    0x2248,
    0x00B0,
    0x2219,
    0x00B7,
    0x221A,
    0x207F,
    0x00B2,
    0x25A0,
    0x00A0
};

unordered_map<wchar_t, int> topPageTranslationMap;

initializer init2([]()
{
    for(int i = 0x80; i < 0x100; i++)
    {
        topPageTranslationMap[topPageTranslations[i - 0x80]] = i;
    }
});

int translateToCodePage437(wchar_t ch)
{
    int character = (int)ch;

    if(character == 0)
    {
        return character;
    }

    if(character >= 0x20 && character <= 0x7E)
    {
        return character;
    }

    switch(character)
    {
    case 0x263A:
        return 0x01;

    case 0x263B:
        return 0x02;

    case 0x2665:
        return 0x03;

    case 0x2666:
        return 0x04;

    case 0x2663:
        return 0x05;

    case 0x2660:
        return 0x06;

    case 0x2022:
        return 0x07;

    case 0x25D8:
        return 0x08;

    case 0x25CB:
        return 0x09;

    case 0x25D9:
        return 0x0A;

    case 0x2642:
        return 0x0B;

    case 0x2640:
        return 0x0C;

    case 0x266A:
        return 0x0D;

    case 0x266B:
        return 0x0E;

    case 0x263C:
        return 0x0F;

    case 0x25BA:
        return 0x10;

    case 0x25C4:
        return 0x11;

    case 0x2195:
        return 0x12;

    case 0x203C:
        return 0x13;

    case 0x00B6:
        return 0x14;

    case 0x00A7:
        return 0x15;

    case 0x25AC:
        return 0x16;

    case 0x21A8:
        return 0x17;

    case 0x2191:
        return 0x18;

    case 0x2193:
        return 0x19;

    case 0x2192:
        return 0x1A;

    case 0x2190:
        return 0x1B;

    case 0x221F:
        return 0x1C;

    case 0x2194:
        return 0x1D;

    case 0x25B2:
        return 0x1E;

    case 0x25BC:
        return 0x1F;

    case 0x2302:
        return 0x7F;

    case 0x03B2:
        return 0xE1;

    case 0x03A0:
    case 0x220F:
        return 0xE3;

    case 0x2211:
        return 0xE4;

    case 0x03BC:
        return 0xE6;

    case 0x2126:
        return 0xEA;

    case 0x00F0:
    case 0x2202:
        return 0xEB;

    case 0x2205:
    case 0x03D5:
    case 0x2300:
    case 0x00F8:
        return 0xED;

    case 0x2208:
    case 0x20AC:
        return 0xEE;

    default:
    {
        try
        {
            return topPageTranslationMap.at(character);
        }
        catch(out_of_range &e)
        {
            return (int)'?';
        }
    }
    }
}

Mesh charMesh[256];
bool didInit = false;

void init()
{
    if(didInit)
    {
        return;
    }

    didInit = true;
    for(size_t i = 0; i < sizeof(charMesh) / sizeof(charMesh[0]); i++)
    {
        int left = (i % (textureXRes / fontWidth)) * fontWidth;
        int top = (i / (textureXRes / fontWidth)) * fontHeight;
        const int width = fontWidth;
        const int height = fontHeight;
        float minU = (left + pixelOffset) / textureXRes;
        float maxU = (left + width - pixelOffset) / textureXRes;
        float minV = 1 - (top + height - pixelOffset) / textureYRes;
        float maxV = 1 - (top + pixelOffset) / textureYRes;
        TextureDescriptor texture = Font.tdNoOffset();
        texture = texture.subTexture(minU, maxU, minV, maxV);
        charMesh[i] = Generate::quadrilateral(texture, VectorF(0, 0, 0), colorizeIdentity(), VectorF(1, 0, 0), colorizeIdentity(), VectorF(1, 1, 0), colorizeIdentity(), VectorF(0, 1, 0), colorizeIdentity());
    }
}

void renderChar(Mesh &dest, Matrix m, ColorF color, wchar_t ch)
{
    init();
    //cout << "char:" << (char)ch << " pt:" << transform(m, VectorF(0, 0, 0)) << endl;
    dest.append(colorize(color, transform(m, charMesh[translateToCodePage437(ch)])));
}

bool updateFromChar(float &x, float &y, float &w, float &h, wchar_t ch, const Text::TextProperties &properties)
{
    if(ch == L'\n')
    {
        x = 0;
        y += charHeight;
    }
    else if(ch == L'\r')
    {
        x = 0;
    }
    else if(ch == L'\t')
    {
        x = floor(x / properties.tabWidth + 1) * properties.tabWidth;
    }
    else if(ch == L' ')
    {
        x += charWidth;
    }
    else
    {
        x += charWidth;

        if(x > w)
        {
            w = x;
        }

        if(y + charHeight > h)
        {
            h = y + charHeight;
        }
        return true;
    }
    return false;
}
}

float Text::width(wstring str, const TextProperties &properties)
{
    float x = 0, y = 0, w = 0, h = 0;

    for(wchar_t ch : str)
    {
        updateFromChar(x, y, w, h, ch, properties);
    }

    return w;
}

float Text::height(wstring str, const TextProperties &properties)
{
    float x = 0, y = 0, w = 0, h = 0;

    for(wchar_t ch : str)
    {
        updateFromChar(x, y, w, h, ch, properties);
    }

    return h;
}

float Text::xPos(wstring str, const TextProperties &properties)
{
    float x = 0, y = 0, w = 0, h = 0;

    for(wchar_t ch : str)
    {
        updateFromChar(x, y, w, h, ch, properties);
    }

    return x;
}

float Text::yPos(wstring str, const TextProperties &properties)
{
    float x = 0, y = 0, w = 0, h = 0;

    for(wchar_t ch : str)
    {
        updateFromChar(x, y, w, h, ch, properties);
    }

    return h - y - 1;
}

Mesh Text::mesh(wstring str, ColorF color, const TextProperties &properties)
{
    float x = 0, y = 0, w = 0, h = 0;
    float totalHeight = height(str, properties);
    Mesh retval;

    for(wchar_t ch : str)
    {
        Matrix mat = Matrix::translate(x, totalHeight - y - 1, 0);
        if(updateFromChar(x, y, w, h, ch, properties))
        {
            renderChar(retval, mat, color, ch);
        }
    }

    return retval;
}
