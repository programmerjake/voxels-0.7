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
#include "render/text.h"
#include "texture/texture_atlas.h"
#include <unordered_map>
#include "util/util.h"
#include <iostream>

using namespace std;

namespace programmerjake
{
namespace voxels
{
const Text::TextProperties Text::defaultTextProperties = Text::TextProperties();

namespace Text
{
class FontDescriptor final
{
public:
    bool isVectorFont;
};

Font getBitmappedFont8x8()
{
    static const FontDescriptor staticDescriptor = {false};
    return Font(&staticDescriptor);
}

Font getVectorFont()
{
    static const FontDescriptor staticDescriptor = {true};
    return Font(&staticDescriptor);
}
}

namespace
{
const float charWidth = 1, charHeight = 1;
const TextureAtlas & FontTexture = TextureAtlas::Font8x8;
const int fontWidth = 8, fontHeight = 8;
const int textureXRes = 128, textureYRes = 128;
const float pixelOffset = TextureAtlas::pixelOffset;
constexpr size_t glyphCount = 256;

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
    0x2610
};

unordered_map<wchar_t, int> topPageTranslationMap;

initializer init2([]()
{
    for(int i = 0x80; i < 0x100; i++)
    {
        topPageTranslationMap[topPageTranslations[i - 0x80]] = i;
    }
});

int translateToFontIndex(wchar_t ch)
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
    case 0x2611:
        return 0x00;

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

    case 0xA0:
        return 0x20;

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
        catch(out_of_range &)
        {
            return (int)'?';
        }
    }
    }
}

struct VectorFontVertex final
{
    float x;
    float y;
    enum Kind
    {
        StartLoop,
        Vertex,
        EndLoop,
        EndGlyph,
        UnimplementedGlyph,
    };
    Kind kind;
};

static const VectorFontVertex vectorFontVertices[] =
{
    // Glyph 0 : '☑'
    {0, 0, VectorFontVertex::StartLoop},
    {0, 7, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {1, 1, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::EndLoop},
    {7, 7, VectorFontVertex::StartLoop},
    {7, 1, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {1, 6, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::EndLoop},
    {5, 2, VectorFontVertex::StartLoop},
    {2, 5, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::EndLoop},
    {2, 4, VectorFontVertex::StartLoop},
    {1, 5, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 1 : '☺'
    {1, 0, VectorFontVertex::StartLoop},
    {0, 1, VectorFontVertex::Vertex},
    {0, 7, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {1, 1, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::EndLoop},
    {7, 8, VectorFontVertex::StartLoop},
    {8, 7, VectorFontVertex::Vertex},
    {8, 1, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::Vertex},
    {7, 7, VectorFontVertex::Vertex},
    {0, 7, VectorFontVertex::Vertex},
    {1, 8, VectorFontVertex::EndLoop},
    {2, 2, VectorFontVertex::StartLoop},
    {2, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::EndLoop},
    {5, 2, VectorFontVertex::StartLoop},
    {5, 3, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::EndLoop},
    {6, 4, VectorFontVertex::StartLoop},
    {2, 4, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::Vertex},
    {3, 6, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 2 : '☻'
    {1, 0, VectorFontVertex::StartLoop},
    {0, 1, VectorFontVertex::Vertex},
    {0, 7, VectorFontVertex::Vertex},
    {1, 8, VectorFontVertex::Vertex},
    {2, 8, VectorFontVertex::Vertex},
    {2, 2, VectorFontVertex::Vertex},
    {8, 2, VectorFontVertex::Vertex},
    {8, 1, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::EndLoop},
    {8, 7, VectorFontVertex::StartLoop},
    {8, 2, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {2, 8, VectorFontVertex::Vertex},
    {7, 8, VectorFontVertex::EndLoop},
    {2, 5, VectorFontVertex::StartLoop},
    {2, 6, VectorFontVertex::Vertex},
    {3, 6, VectorFontVertex::EndLoop},
    {3, 3, VectorFontVertex::StartLoop},
    {2, 3, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 3 : '♥'
    {4.5f, 6, VectorFontVertex::StartLoop},
    {7, 3.5f, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::Vertex},
    {6, 0, VectorFontVertex::Vertex},
    {5, 0, VectorFontVertex::Vertex},
    {4, 1, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::Vertex},
    {2, 0, VectorFontVertex::Vertex},
    {1, 1, VectorFontVertex::Vertex},
    {1, 3.5f, VectorFontVertex::Vertex},
    {3.5f, 6, VectorFontVertex::EndLoop},
    {3.5f, 6, VectorFontVertex::StartLoop},
    {4, 7, VectorFontVertex::Vertex},
    {4.5f, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 4 : '♦'
    {3.5f, 0, VectorFontVertex::StartLoop},
    {0, 3.5f, VectorFontVertex::Vertex},
    {3.5f, 7, VectorFontVertex::Vertex},
    {7, 3.5f, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 5 : '♣'
    {3, 0, VectorFontVertex::StartLoop},
    {2, 1, VectorFontVertex::Vertex},
    {2, 2, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::EndLoop},
    {3, 3, VectorFontVertex::StartLoop},
    {3, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::EndLoop},
    {3, 3.5f, VectorFontVertex::StartLoop},
    {2.5f, 3, VectorFontVertex::Vertex},
    {1, 3, VectorFontVertex::Vertex},
    {0, 4, VectorFontVertex::Vertex},
    {0, 5, VectorFontVertex::Vertex},
    {1, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {3, 4.5f, VectorFontVertex::EndLoop},
    {4, 3.5f, VectorFontVertex::StartLoop},
    {4, 4.5f, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {7, 5, VectorFontVertex::Vertex},
    {7, 4, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {4.5f, 3, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 6 : '♠'
    {3, 0, VectorFontVertex::StartLoop},
    {0.5f, 4, VectorFontVertex::Vertex},
    {0.5f, 5, VectorFontVertex::Vertex},
    {1.5f, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {5.5f, 6, VectorFontVertex::Vertex},
    {6.5f, 5, VectorFontVertex::Vertex},
    {6.5f, 4, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 7 : '•'
    {3, 2, VectorFontVertex::StartLoop},
    {2, 3, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::Vertex},
    {3, 6, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 8 : '◘'
    {0, 0, VectorFontVertex::StartLoop},
    {0, 8, VectorFontVertex::Vertex},
    {2, 8, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::Vertex},
    {8, 2, VectorFontVertex::Vertex},
    {8, 0, VectorFontVertex::EndLoop},
    {8, 8, VectorFontVertex::StartLoop},
    {8, 2, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {2, 8, VectorFontVertex::EndLoop},
    {6, 2, VectorFontVertex::StartLoop},
    {5, 2, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::EndLoop},
    {2, 6, VectorFontVertex::StartLoop},
    {3, 6, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 9 : '○'
    {2, 1, VectorFontVertex::StartLoop},
    {1, 2, VectorFontVertex::Vertex},
    {1, 6, VectorFontVertex::Vertex},
    {2, 7, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::Vertex},
    {7, 2, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::EndLoop},
    {7, 2, VectorFontVertex::StartLoop},
    {5, 2, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {6, 7, VectorFontVertex::Vertex},
    {7, 6, VectorFontVertex::EndLoop},
    {6, 7, VectorFontVertex::StartLoop},
    {6, 5, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {2, 7, VectorFontVertex::EndLoop},
    {2, 6, VectorFontVertex::StartLoop},
    {3, 6, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 10 : '◙'
    {3, 2, VectorFontVertex::StartLoop},
    {2, 3, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::Vertex},
    {3, 6, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::StartLoop},
    {0, 8, VectorFontVertex::Vertex},
    {1, 8, VectorFontVertex::Vertex},
    {1, 2, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {8, 1, VectorFontVertex::Vertex},
    {8, 0, VectorFontVertex::EndLoop},
    {8, 8, VectorFontVertex::StartLoop},
    {8, 1, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::Vertex},
    {7, 6, VectorFontVertex::Vertex},
    {6, 7, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {1, 8, VectorFontVertex::EndLoop},
    {7, 1, VectorFontVertex::StartLoop},
    {6, 1, VectorFontVertex::Vertex},
    {7, 2, VectorFontVertex::EndLoop},
    {1, 7, VectorFontVertex::StartLoop},
    {2, 7, VectorFontVertex::Vertex},
    {1, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 11 : '♂'
    {7, 1, VectorFontVertex::StartLoop},
    {4, 1, VectorFontVertex::Vertex},
    {4, 2, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::Vertex},
    {5, 4, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::Vertex},
    {7, 4, VectorFontVertex::EndLoop},
    {5, 4, VectorFontVertex::StartLoop},
    {4, 3, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {1, 4, VectorFontVertex::Vertex},
    {4, 4, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::EndLoop},
    {2, 7, VectorFontVertex::StartLoop},
    {4, 7, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {1, 4, VectorFontVertex::Vertex},
    {1, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 12 : '♀'
    {6, 1, VectorFontVertex::StartLoop},
    {5, 0, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::Vertex},
    {5, 4, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::EndLoop},
    {3, 4, VectorFontVertex::StartLoop},
    {5, 4, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {3, 1, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::EndLoop},
    {3.5f, 5, VectorFontVertex::StartLoop},
    {2, 5, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {3.5f, 6, VectorFontVertex::Vertex},
    {3.5f, 7, VectorFontVertex::Vertex},
    {4.5f, 7, VectorFontVertex::Vertex},
    {4.5f, 6, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {4.5f, 5, VectorFontVertex::Vertex},
    {4.5f, 4, VectorFontVertex::Vertex},
    {3.5f, 4, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 13 : '♪'
    {3, 4, VectorFontVertex::StartLoop},
    {2, 4, VectorFontVertex::Vertex},
    {1, 5, VectorFontVertex::Vertex},
    {1, 6, VectorFontVertex::Vertex},
    {2, 7, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::EndLoop},
    {4, 0, VectorFontVertex::StartLoop},
    {4, 1, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 14 : '♫'
    {2, 4, VectorFontVertex::StartLoop},
    {1, 4, VectorFontVertex::Vertex},
    {0, 5, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {2, 7, VectorFontVertex::Vertex},
    {3, 6, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::Vertex},
    {2, 0, VectorFontVertex::EndLoop},
    {6, 5, VectorFontVertex::StartLoop},
    {5, 5, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {5, 8, VectorFontVertex::Vertex},
    {6, 8, VectorFontVertex::Vertex},
    {7, 7, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::EndLoop},
    {3, 0, VectorFontVertex::StartLoop},
    {3, 1, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 15 : '☼'
    {6, 3, VectorFontVertex::StartLoop},
    {5, 2, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::EndLoop},
    {3, 6, VectorFontVertex::StartLoop},
    {5, 6, VectorFontVertex::Vertex},
    {5, 5, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::EndLoop},
    {3.5f, 0, VectorFontVertex::StartLoop},
    {3.5f, 2, VectorFontVertex::Vertex},
    {4.5f, 2, VectorFontVertex::Vertex},
    {4.5f, 0, VectorFontVertex::EndLoop},
    {3.5f, 6, VectorFontVertex::StartLoop},
    {3.5f, 8, VectorFontVertex::Vertex},
    {4.5f, 8, VectorFontVertex::Vertex},
    {4.5f, 6, VectorFontVertex::EndLoop},
    {2, 3.5f, VectorFontVertex::StartLoop},
    {0, 3.5f, VectorFontVertex::Vertex},
    {0, 4.5f, VectorFontVertex::Vertex},
    {2, 4.5f, VectorFontVertex::EndLoop},
    {8, 3.5f, VectorFontVertex::StartLoop},
    {6, 3.5f, VectorFontVertex::Vertex},
    {6, 4.5f, VectorFontVertex::Vertex},
    {8, 4.5f, VectorFontVertex::EndLoop},
    {3, 2, VectorFontVertex::StartLoop},
    {1.5f, 0.5f, VectorFontVertex::Vertex},
    {0.5f, 1.5f, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::EndLoop},
    {5, 6, VectorFontVertex::StartLoop},
    {6.5f, 7.5f, VectorFontVertex::Vertex},
    {7.5f, 6.5f, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::EndLoop},
    {6, 3, VectorFontVertex::StartLoop},
    {7.5f, 1.5f, VectorFontVertex::Vertex},
    {6.5f, 0.5f, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::EndLoop},
    {2, 5, VectorFontVertex::StartLoop},
    {0.5f, 6.5f, VectorFontVertex::Vertex},
    {1.5f, 7.5f, VectorFontVertex::Vertex},
    {3, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 16 : '►'
    {0, 7, VectorFontVertex::StartLoop},
    {7, 4, VectorFontVertex::Vertex},
    {0, 1, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 17 : '◄'
    {7, 1, VectorFontVertex::StartLoop},
    {0, 4, VectorFontVertex::Vertex},
    {7, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 18 : '↕'
    {4, 0, VectorFontVertex::StartLoop},
    {1, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {5, 5, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {7, 3, VectorFontVertex::EndLoop},
    {7, 5, VectorFontVertex::StartLoop},
    {1, 5, VectorFontVertex::Vertex},
    {4, 8, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 19 : '‼'
    {1, 0, VectorFontVertex::StartLoop},
    {1, 5, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::EndLoop},
    {5, 0, VectorFontVertex::StartLoop},
    {5, 5, VectorFontVertex::Vertex},
    {7, 5, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::EndLoop},
    {1, 6, VectorFontVertex::StartLoop},
    {1, 7, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {3, 6, VectorFontVertex::EndLoop},
    {5, 6, VectorFontVertex::StartLoop},
    {5, 7, VectorFontVertex::Vertex},
    {7, 7, VectorFontVertex::Vertex},
    {7, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 20 : '¶'
    {4, 0, VectorFontVertex::StartLoop},
    {2, 0, VectorFontVertex::Vertex},
    {1, 1, VectorFontVertex::Vertex},
    {1, 3, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {3, 4, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::EndLoop},
    {6, 0, VectorFontVertex::StartLoop},
    {4, 0, VectorFontVertex::Vertex},
    {4, 1, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {6, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 21 : '§'
    {2, 2, VectorFontVertex::StartLoop},
    {1, 3, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {2, 0, VectorFontVertex::Vertex},
    {1, 1, VectorFontVertex::EndLoop},
    {6, 0, VectorFontVertex::StartLoop},
    {2, 0, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::Vertex},
    {7, 2, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::EndLoop},
    {1, 3, VectorFontVertex::StartLoop},
    {1, 5, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::EndLoop},
    {5, 6, VectorFontVertex::StartLoop},
    {6, 5, VectorFontVertex::Vertex},
    {1, 5, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {5, 8, VectorFontVertex::Vertex},
    {6, 7, VectorFontVertex::EndLoop},
    {1, 8, VectorFontVertex::StartLoop},
    {5, 8, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {0, 7, VectorFontVertex::EndLoop},
    {6, 5, VectorFontVertex::StartLoop},
    {6, 3, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 22 : '▬'
    {1, 4, VectorFontVertex::StartLoop},
    {1, 7, VectorFontVertex::Vertex},
    {7, 7, VectorFontVertex::Vertex},
    {7, 4, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 23 : '↨'
    {4, 0, VectorFontVertex::StartLoop},
    {1, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {3, 4, VectorFontVertex::Vertex},
    {5, 4, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {7, 3, VectorFontVertex::EndLoop},
    {7, 4, VectorFontVertex::StartLoop},
    {1, 4, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::EndLoop},
    {0, 7, VectorFontVertex::StartLoop},
    {0, 8, VectorFontVertex::Vertex},
    {8, 8, VectorFontVertex::Vertex},
    {8, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 24 : '↑'
    {4, 0, VectorFontVertex::StartLoop},
    {1, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {7, 3, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 25 : '↓'
    {4, 7, VectorFontVertex::StartLoop},
    {7, 4, VectorFontVertex::Vertex},
    {5, 4, VectorFontVertex::Vertex},
    {5, 0, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::Vertex},
    {3, 4, VectorFontVertex::Vertex},
    {1, 4, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 26 : '→'
    {7, 3, VectorFontVertex::StartLoop},
    {4, 0, VectorFontVertex::Vertex},
    {4, 2, VectorFontVertex::Vertex},
    {0, 2, VectorFontVertex::Vertex},
    {0, 4, VectorFontVertex::Vertex},
    {4, 4, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 27 : '←'
    {0, 4, VectorFontVertex::StartLoop},
    {3, 7, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {7, 5, VectorFontVertex::Vertex},
    {7, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {3, 1, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 28 : '∟'
    {0, 6, VectorFontVertex::StartLoop},
    {7, 6, VectorFontVertex::Vertex},
    {7, 5, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::Vertex},
    {2, 2, VectorFontVertex::Vertex},
    {0, 2, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 29 : '↔'
    {0, 4, VectorFontVertex::StartLoop},
    {3, 7, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {5, 5, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {3, 1, VectorFontVertex::EndLoop},
    {5, 1, VectorFontVertex::StartLoop},
    {5, 7, VectorFontVertex::Vertex},
    {8, 4, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 30 : '▲'
    {1, 6, VectorFontVertex::StartLoop},
    {7, 6, VectorFontVertex::Vertex},
    {4, 1, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 31 : '▼'
    {7, 1, VectorFontVertex::StartLoop},
    {1, 1, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 32 : ' '
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 33 : '!'
    {2, 0, VectorFontVertex::StartLoop},
    {1, 1, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::EndLoop},
    {2, 6, VectorFontVertex::StartLoop},
    {2, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 34 : '"'
    {2, 0, VectorFontVertex::StartLoop},
    {1, 1, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {3, 1, VectorFontVertex::EndLoop},
    {5, 0, VectorFontVertex::StartLoop},
    {4, 1, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 35 : '#'
    {1, 2, VectorFontVertex::StartLoop},
    {0, 2, VectorFontVertex::Vertex},
    {0, 3, VectorFontVertex::Vertex},
    {1, 3, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {7, 3, VectorFontVertex::Vertex},
    {7, 2, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::Vertex},
    {1, 0, VectorFontVertex::EndLoop},
    {4, 4, VectorFontVertex::StartLoop},
    {3, 4, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {6, 7, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {7, 5, VectorFontVertex::Vertex},
    {7, 4, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::EndLoop},
    {0, 4, VectorFontVertex::StartLoop},
    {0, 5, VectorFontVertex::Vertex},
    {1, 5, VectorFontVertex::Vertex},
    {1, 4, VectorFontVertex::EndLoop},
    {4, 0, VectorFontVertex::StartLoop},
    {4, 2, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::Vertex},
    {6, 0, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 36 : '$'
    {2, 2, VectorFontVertex::StartLoop},
    {6, 2, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::Vertex},
    {4, 1, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::Vertex},
    {2, 0, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {1, 1, VectorFontVertex::Vertex},
    {0, 2, VectorFontVertex::Vertex},
    {0, 3, VectorFontVertex::Vertex},
    {1, 4, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::EndLoop},
    {4, 5, VectorFontVertex::StartLoop},
    {0, 5, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {2, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::EndLoop},
    {2, 3, VectorFontVertex::StartLoop},
    {2, 4, VectorFontVertex::Vertex},
    {4, 4, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 37 : '%'
    {0.5f, 1.5f, VectorFontVertex::StartLoop},
    {2, 1.5f, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {0, 1, VectorFontVertex::Vertex},
    {0, 3, VectorFontVertex::Vertex},
    {0.5f, 3, VectorFontVertex::EndLoop},
    {1.5f, 2.5f, VectorFontVertex::StartLoop},
    {0.5f, 2.5f, VectorFontVertex::Vertex},
    {0.5f, 3, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {2, 1.5f, VectorFontVertex::Vertex},
    {1.5f, 1.5f, VectorFontVertex::EndLoop},
    {5.5f, 5.5f, VectorFontVertex::StartLoop},
    {7, 5.5f, VectorFontVertex::Vertex},
    {7, 5, VectorFontVertex::Vertex},
    {5, 5, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {5.5f, 7, VectorFontVertex::EndLoop},
    {6.5f, 6.5f, VectorFontVertex::StartLoop},
    {5.5f, 6.5f, VectorFontVertex::Vertex},
    {5.5f, 7, VectorFontVertex::Vertex},
    {7, 7, VectorFontVertex::Vertex},
    {7, 5.5f, VectorFontVertex::Vertex},
    {6.5f, 5.5f, VectorFontVertex::EndLoop},
    {7, 1, VectorFontVertex::StartLoop},
    {6, 1, VectorFontVertex::Vertex},
    {0, 7, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 38 : '&'
    {0.6f, 3.4f, VectorFontVertex::StartLoop},
    {0.3f, 3.9f, VectorFontVertex::Vertex},
    {0, 4.8f, VectorFontVertex::Vertex},
    {0.9f, 4.8f, VectorFontVertex::Vertex},
    {1, 4, VectorFontVertex::Vertex},
    {1.7f, 3.3f, VectorFontVertex::Vertex},
    {1.3f, 2.7f, VectorFontVertex::EndLoop},
    {0.6f, 6.2f, VectorFontVertex::StartLoop},
    {1.3f, 6.8f, VectorFontVertex::Vertex},
    {2.5f, 7, VectorFontVertex::Vertex},
    {2.5f, 6.3f, VectorFontVertex::Vertex},
    {1.5f, 6, VectorFontVertex::Vertex},
    {1, 5.5f, VectorFontVertex::Vertex},
    {0.9f, 4.8f, VectorFontVertex::Vertex},
    {0, 4.8f, VectorFontVertex::EndLoop},
    {3.9f, 6.7f, VectorFontVertex::StartLoop},
    {4.6f, 6.2f, VectorFontVertex::Vertex},
    {4.2f, 5.7f, VectorFontVertex::Vertex},
    {3.6f, 6.1f, VectorFontVertex::Vertex},
    {2.5f, 6.3f, VectorFontVertex::Vertex},
    {2.5f, 7, VectorFontVertex::EndLoop},
    {5.7f, 4.8f, VectorFontVertex::StartLoop},
    {6.1f, 3.3f, VectorFontVertex::Vertex},
    {5.3f, 3.3f, VectorFontVertex::Vertex},
    {5.1f, 4.3f, VectorFontVertex::Vertex},
    {4.7f, 5.1f, VectorFontVertex::Vertex},
    {5.2f, 5.6f, VectorFontVertex::EndLoop},
    {3, 4, VectorFontVertex::StartLoop},
    {4.2f, 5.7f, VectorFontVertex::Vertex},
    {4.6f, 6.2f, VectorFontVertex::Vertex},
    {5.4f, 7, VectorFontVertex::Vertex},
    {6.4f, 7, VectorFontVertex::Vertex},
    {5.2f, 5.6f, VectorFontVertex::Vertex},
    {4.7f, 5.1f, VectorFontVertex::Vertex},
    {2, 2.4f, VectorFontVertex::EndLoop},
    {1, 2.2f, VectorFontVertex::StartLoop},
    {1.3f, 2.7f, VectorFontVertex::Vertex},
    {1.7f, 3.3f, VectorFontVertex::Vertex},
    {4.2f, 5.7f, VectorFontVertex::Vertex},
    {3, 4, VectorFontVertex::Vertex},
    {2, 2.4f, VectorFontVertex::Vertex},
    {1.7f, 1.6f, VectorFontVertex::Vertex},
    {0.8f, 1.7f, VectorFontVertex::EndLoop},
    {1.3f, 0.5f, VectorFontVertex::StartLoop},
    {0.9f, 0.9f, VectorFontVertex::Vertex},
    {0.8f, 1.7f, VectorFontVertex::Vertex},
    {1.7f, 1.6f, VectorFontVertex::Vertex},
    {1.9f, 1, VectorFontVertex::Vertex},
    {2.8f, 0.7f, VectorFontVertex::Vertex},
    {2.6f, 0, VectorFontVertex::Vertex},
    {1.7f, 0.2f, VectorFontVertex::EndLoop},
    {3.5f, 0.1f, VectorFontVertex::StartLoop},
    {2.6f, 0, VectorFontVertex::Vertex},
    {2.8f, 0.7f, VectorFontVertex::Vertex},
    {3.5f, 0.8f, VectorFontVertex::Vertex},
    {4.2f, 1.1f, VectorFontVertex::Vertex},
    {4.2f, 0.3f, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 39 : '''
    {2, 0, VectorFontVertex::StartLoop},
    {1, 1, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {3, 1, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 40 : '('
    {1, 2, VectorFontVertex::StartLoop},
    {1, 5, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::Vertex},
    {5, 0, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::EndLoop},
    {5, 7, VectorFontVertex::StartLoop},
    {3, 5, VectorFontVertex::Vertex},
    {1, 5, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 41 : ')'
    {5, 5, VectorFontVertex::StartLoop},
    {5, 2, VectorFontVertex::Vertex},
    {3, 2, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::EndLoop},
    {1, 0, VectorFontVertex::StartLoop},
    {3, 2, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 42 : '*'
    {3, 2, VectorFontVertex::StartLoop},
    {2, 3, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {3, 5, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {5, 4, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {4, 2, VectorFontVertex::EndLoop},
    {3, 2, VectorFontVertex::StartLoop},
    {2, 1, VectorFontVertex::Vertex},
    {1, 2, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::EndLoop},
    {4, 5, VectorFontVertex::StartLoop},
    {5, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {5, 4, VectorFontVertex::EndLoop},
    {2, 3, VectorFontVertex::StartLoop},
    {0.5f, 3, VectorFontVertex::Vertex},
    {0.5f, 4, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::EndLoop},
    {5, 4, VectorFontVertex::StartLoop},
    {6.5f, 4, VectorFontVertex::Vertex},
    {6.5f, 3, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::EndLoop},
    {3, 5, VectorFontVertex::StartLoop},
    {2, 4, VectorFontVertex::Vertex},
    {1, 5, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::EndLoop},
    {4, 2, VectorFontVertex::StartLoop},
    {5, 3, VectorFontVertex::Vertex},
    {6, 2, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 43 : '+'
    {6, 3, VectorFontVertex::StartLoop},
    {0, 3, VectorFontVertex::Vertex},
    {0, 4, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::EndLoop},
    {4, 3, VectorFontVertex::StartLoop},
    {4, 1, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::EndLoop},
    {4, 6, VectorFontVertex::StartLoop},
    {4, 4, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 44 : ','
    {2, 6, VectorFontVertex::StartLoop},
    {1, 8, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 45 : '-'
    {6, 3, VectorFontVertex::StartLoop},
    {0, 3, VectorFontVertex::Vertex},
    {0, 4, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 46 : '.'
    {2, 7, VectorFontVertex::StartLoop},
    {4, 7, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 47 : '/'
    {1, 7, VectorFontVertex::StartLoop},
    {3, 7, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::Vertex},
    {5, 0, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 48 : '0'
    {1, 7, VectorFontVertex::StartLoop},
    {6, 7, VectorFontVertex::Vertex},
    {7, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {2, 0, VectorFontVertex::Vertex},
    {1, 0, VectorFontVertex::Vertex},
    {0, 1, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::EndLoop},
    {7, 1, VectorFontVertex::StartLoop},
    {6, 0, VectorFontVertex::Vertex},
    {2, 0, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::Vertex},
    {5, 6, VectorFontVertex::Vertex},
    {7, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 49 : '1'
    {4, 0, VectorFontVertex::StartLoop},
    {2.5f, 0, VectorFontVertex::Vertex},
    {0.5f, 2, VectorFontVertex::Vertex},
    {2, 2, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::EndLoop},
    {6, 6, VectorFontVertex::StartLoop},
    {0, 6, VectorFontVertex::Vertex},
    {0, 7, VectorFontVertex::Vertex},
    {6, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 50 : '2'
    {6, 7, VectorFontVertex::StartLoop},
    {6, 5, VectorFontVertex::Vertex},
    {5, 5, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {0, 7, VectorFontVertex::EndLoop},
    {4, 4, VectorFontVertex::StartLoop},
    {5, 4, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::Vertex},
    {5, 0, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::Vertex},
    {3, 3, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {2, 6, VectorFontVertex::EndLoop},
    {2, 1, VectorFontVertex::StartLoop},
    {4, 1, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::Vertex},
    {1, 0, VectorFontVertex::Vertex},
    {0, 1, VectorFontVertex::Vertex},
    {0, 2, VectorFontVertex::Vertex},
    {1, 2, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 51 : '3'
    {5, 3.5f, VectorFontVertex::StartLoop},
    {6, 3, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::Vertex},
    {5, 0, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::Vertex},
    {4, 3, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {4, 4, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::EndLoop},
    {2, 1, VectorFontVertex::StartLoop},
    {4, 1, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::Vertex},
    {1, 0, VectorFontVertex::Vertex},
    {0, 1, VectorFontVertex::Vertex},
    {0, 2, VectorFontVertex::Vertex},
    {1, 2, VectorFontVertex::EndLoop},
    {2, 6, VectorFontVertex::StartLoop},
    {1, 5, VectorFontVertex::Vertex},
    {0, 5, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 52 : '4'
    {4, 6, VectorFontVertex::StartLoop},
    {3, 6, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {7, 7, VectorFontVertex::Vertex},
    {7, 6, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {6, 5, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::EndLoop},
    {4, 4, VectorFontVertex::StartLoop},
    {0, 4, VectorFontVertex::Vertex},
    {0, 5, VectorFontVertex::Vertex},
    {7, 5, VectorFontVertex::Vertex},
    {7, 4, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::Vertex},
    {6, 0, VectorFontVertex::Vertex},
    {4, 0, VectorFontVertex::EndLoop},
    {4, 0, VectorFontVertex::StartLoop},
    {0, 4, VectorFontVertex::Vertex},
    {2, 4, VectorFontVertex::Vertex},
    {4, 2, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 53 : '5'
    {2, 1, VectorFontVertex::StartLoop},
    {6, 1, VectorFontVertex::Vertex},
    {6, 0, VectorFontVertex::Vertex},
    {0, 0, VectorFontVertex::Vertex},
    {0, 2, VectorFontVertex::Vertex},
    {2, 2, VectorFontVertex::EndLoop},
    {4, 3, VectorFontVertex::StartLoop},
    {4, 7, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::Vertex},
    {0, 2, VectorFontVertex::Vertex},
    {0, 3, VectorFontVertex::EndLoop},
    {2, 6, VectorFontVertex::StartLoop},
    {1, 5, VectorFontVertex::Vertex},
    {0, 5, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 54 : '6'
    {1, 1, VectorFontVertex::StartLoop},
    {0, 3, VectorFontVertex::Vertex},
    {2, 3, VectorFontVertex::Vertex},
    {2, 2, VectorFontVertex::Vertex},
    {3, 1, VectorFontVertex::Vertex},
    {5, 1, VectorFontVertex::Vertex},
    {5, 0, VectorFontVertex::Vertex},
    {2, 0, VectorFontVertex::EndLoop},
    {2, 4, VectorFontVertex::StartLoop},
    {6, 4, VectorFontVertex::Vertex},
    {5, 3, VectorFontVertex::Vertex},
    {0, 3, VectorFontVertex::Vertex},
    {0, 6, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {2, 7, VectorFontVertex::EndLoop},
    {4, 6, VectorFontVertex::StartLoop},
    {2, 6, VectorFontVertex::Vertex},
    {2, 7, VectorFontVertex::Vertex},
    {5, 7, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {6, 4, VectorFontVertex::Vertex},
    {4, 4, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 55 : '7'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 56 : '8'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 57 : '9'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 58 : ':'
    {2, 3, VectorFontVertex::StartLoop},
    {4, 3, VectorFontVertex::Vertex},
    {4, 1, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::EndLoop},
    {2, 7, VectorFontVertex::StartLoop},
    {4, 7, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 59 : ';'
    {2, 3, VectorFontVertex::StartLoop},
    {4, 3, VectorFontVertex::Vertex},
    {4, 1, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::EndLoop},
    {2, 6, VectorFontVertex::StartLoop},
    {1, 8, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {2, 5, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 60 : '<'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 61 : '='
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 62 : '>'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 63 : '?'
    {6, 1, VectorFontVertex::StartLoop},
    {5, 0, VectorFontVertex::Vertex},
    {1, 0, VectorFontVertex::Vertex},
    {0, 1, VectorFontVertex::EndLoop},
    {0, 1, VectorFontVertex::StartLoop},
    {0, 2, VectorFontVertex::Vertex},
    {1, 2, VectorFontVertex::Vertex},
    {2, 1, VectorFontVertex::EndLoop},
    {4, 1, VectorFontVertex::StartLoop},
    {4, 3, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::EndLoop},
    {4, 3, VectorFontVertex::StartLoop},
    {2, 5, VectorFontVertex::Vertex},
    {4, 5, VectorFontVertex::Vertex},
    {6, 3, VectorFontVertex::EndLoop},
    {2, 6, VectorFontVertex::StartLoop},
    {2, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::Vertex},
    {4, 6, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 64 : '@'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 65 : 'A'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 66 : 'B'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 67 : 'C'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 68 : 'D'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 69 : 'E'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 70 : 'F'
    {0, 0, VectorFontVertex::StartLoop},
    {1, 1, VectorFontVertex::Vertex},
    {1, 0, VectorFontVertex::EndLoop},
    {1, 0, VectorFontVertex::StartLoop},
    {1, 7, VectorFontVertex::Vertex},
    {3, 7, VectorFontVertex::Vertex},
    {3, 1, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::EndLoop},
    {7, 1, VectorFontVertex::StartLoop},
    {6, 1, VectorFontVertex::Vertex},
    {7, 2, VectorFontVertex::EndLoop},
    {4, 3, VectorFontVertex::StartLoop},
    {3, 3, VectorFontVertex::Vertex},
    {3, 4, VectorFontVertex::Vertex},
    {4, 4, VectorFontVertex::Vertex},
    {5, 5, VectorFontVertex::Vertex},
    {5, 2, VectorFontVertex::EndLoop},
    {1, 6, VectorFontVertex::StartLoop},
    {0, 7, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::EndLoop},
    {3, 6, VectorFontVertex::StartLoop},
    {3, 7, VectorFontVertex::Vertex},
    {4, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 71 : 'G'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 72 : 'H'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 73 : 'I'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 74 : 'J'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 75 : 'K'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 76 : 'L'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 77 : 'M'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 78 : 'N'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 79 : 'O'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 80 : 'P'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 81 : 'Q'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 82 : 'R'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 83 : 'S'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 84 : 'T'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 85 : 'U'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 86 : 'V'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 87 : 'W'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 88 : 'X'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 89 : 'Y'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 90 : 'Z'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 91 : '['
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 92 : '\'
    {5, 7, VectorFontVertex::StartLoop},
    {7, 7, VectorFontVertex::Vertex},
    {3, 0, VectorFontVertex::Vertex},
    {1, 0, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

    // Glyph 93 : ']'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 94 : '^'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 95 : '_'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 96 : '`'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 97 : 'a'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 98 : 'b'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 99 : 'c'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 100 : 'd'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 101 : 'e'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 102 : 'f'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 103 : 'g'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 104 : 'h'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 105 : 'i'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 106 : 'j'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 107 : 'k'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 108 : 'l'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 109 : 'm'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 110 : 'n'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 111 : 'o'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 112 : 'p'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 113 : 'q'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 114 : 'r'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 115 : 's'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 116 : 't'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 117 : 'u'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 118 : 'v'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 119 : 'w'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 120 : 'x'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 121 : 'y'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 122 : 'z'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 123 : '{'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 124 : '|'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 125 : '}'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 126 : '~'
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 127 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 128 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 129 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 130 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 131 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 132 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 133 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 134 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 135 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 136 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 137 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 138 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 139 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 140 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 141 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 142 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 143 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 144 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 145 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 146 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 147 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 148 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 149 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 150 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 151 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 152 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 153 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 154 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 155 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 156 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 157 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 158 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 159 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 160 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 161 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 162 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 163 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 164 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 165 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 166 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 167 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 168 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 169 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 170 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 171 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 172 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 173 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 174 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 175 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 176 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 177 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 178 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 179 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 180 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 181 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 182 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 183 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 184 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 185 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 186 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 187 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 188 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 189 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 190 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 191 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 192 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 193 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 194 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 195 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 196 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 197 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 198 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 199 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 200 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 201 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 202 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 203 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 204 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 205 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 206 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 207 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 208 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 209 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 210 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 211 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 212 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 213 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 214 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 215 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 216 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 217 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 218 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 219 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 220 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 221 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 222 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 223 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 224 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 225 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 226 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 227 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 228 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 229 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 230 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 231 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 232 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 233 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 234 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 235 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 236 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 237 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 238 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 239 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 240 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 241 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 242 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 243 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 244 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 245 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 246 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 247 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 248 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 249 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 250 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 251 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 252 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 253 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 254 : ''
    FIXME_MESSAGE(finish creating glyph)
    {0, 0, VectorFontVertex::UnimplementedGlyph},

    // Glyph 255 : '☐'
    {0, 0, VectorFontVertex::StartLoop},
    {0, 7, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::Vertex},
    {1, 1, VectorFontVertex::Vertex},
    {7, 1, VectorFontVertex::Vertex},
    {7, 0, VectorFontVertex::EndLoop},
    {7, 7, VectorFontVertex::StartLoop},
    {7, 1, VectorFontVertex::Vertex},
    {6, 1, VectorFontVertex::Vertex},
    {6, 6, VectorFontVertex::Vertex},
    {1, 6, VectorFontVertex::Vertex},
    {1, 7, VectorFontVertex::EndLoop},
    {0, 0, VectorFontVertex::EndGlyph},

};

checked_array<Mesh, glyphCount> bitmappedCharMesh;
checked_array<Mesh, glyphCount> vectorCharMesh;
bool didInit = false;

void init()
{
    if(didInit)
    {
        return;
    }

    didInit = true;
    for(size_t i = 0; i < bitmappedCharMesh.size(); i++)
    {
        int left = (i % (textureXRes / fontWidth)) * fontWidth;
        int top = (i / (textureXRes / fontWidth)) * fontHeight;
        const int width = fontWidth;
        const int height = fontHeight;
        float minU = (left + pixelOffset) / textureXRes;
        float maxU = (left + width - pixelOffset) / textureXRes;
        float minV = 1 - (top + height - pixelOffset) / textureYRes;
        float maxV = 1 - (top + pixelOffset) / textureYRes;
        TextureDescriptor texture = FontTexture.tdNoOffset();
        texture = texture.subTexture(minU, maxU, minV, maxV);
        bitmappedCharMesh[i] = Generate::quadrilateral(texture, VectorF(0, 0, 0), colorizeIdentity(), VectorF(1, 0, 0), colorizeIdentity(), VectorF(1, 1, 0), colorizeIdentity(), VectorF(0, 1, 0), colorizeIdentity());
    }
    checked_array<bool, glyphCount> isUnimplementedGlyph;
    for(bool &v : isUnimplementedGlyph)
        v = false;
    size_t glyphNumber = 0;
    TextureDescriptor td = TextureAtlas::Blank.td();
    td = td.subTexture(0.5f, 0.5f, 0.5f, 0.5f);
    Mesh glyphMesh;
    glyphMesh.image = td.image;
    float startX = 0;
    float startY = 0;
    bool haveStart = false;
    bool haveLastPoint = false;
    float lastX = 0;
    float lastY = 0;
    TextureCoord tc(td.minU, td.minV);
    for(const VectorFontVertex &v : vectorFontVertices)
    {
        assert(glyphNumber < vectorCharMesh.size());
        float x = v.x / 8.0f;
        float y = 1.0f - v.y / 8.0f;
        switch(v.kind)
        {
        case VectorFontVertex::UnimplementedGlyph:
            isUnimplementedGlyph[glyphNumber] = true;
            assert(!haveStart);
            assert(!haveLastPoint);
            vectorCharMesh[glyphNumber++] = Mesh();
            glyphMesh = Mesh();
            glyphMesh.image = td.image;
            break;
        case VectorFontVertex::EndGlyph:
            assert(!haveStart);
            assert(!haveLastPoint);
            vectorCharMesh[glyphNumber++] = std::move(glyphMesh);
            glyphMesh = Mesh();
            glyphMesh.image = td.image;
            break;
        case VectorFontVertex::EndLoop:
            assert(haveStart);
            assert(haveLastPoint);
            glyphMesh.append(Triangle(VectorF(startX, startY, 0.0f), tc,
                                      VectorF(lastX, lastY, 0.0f), tc,
                                      VectorF(x, y, 0.0f), tc));
            haveStart = false;
            haveLastPoint = false;
            break;
        case VectorFontVertex::StartLoop:
            assert(!haveStart);
            assert(!haveLastPoint);
            haveStart = true;
            startX = x;
            startY = y;
            break;
        case VectorFontVertex::Vertex:
            assert(haveStart);
            if(haveLastPoint)
            {
                glyphMesh.append(Triangle(VectorF(startX, startY, 0.0f), tc,
                                          VectorF(lastX, lastY, 0.0f), tc,
                                          VectorF(x, y, 0.0f), tc));
            }
            haveLastPoint = true;
            lastX = x;
            lastY = y;
            break;
        }
    }
    assert(glyphNumber == vectorCharMesh.size());
    for(size_t i = 0; i < vectorCharMesh.size(); i++)
    {
        if(isUnimplementedGlyph[i])
            vectorCharMesh[i] = vectorCharMesh[(int)'?'];
    }
}

void renderChar(Mesh &dest, Matrix m, ColorF color, wchar_t ch, const Text::TextProperties &properties)
{
    init();
    Text::Font font = properties.font;
    if(font.descriptor == nullptr)
        font = Text::getBitmappedFont8x8();
    if(font.descriptor->isVectorFont)
    {
        dest.append(colorize(color, transform(m, vectorCharMesh[translateToFontIndex(ch)])));
    }
    else
    {
        dest.append(colorize(color, transform(m, bitmappedCharMesh[translateToFontIndex(ch)])));
    }
}

bool updateFromChar(float &x, float &y, float &w, float &h, wchar_t ch, const Text::TextProperties &properties)
{
    if(ch == L'\n')
    {
        x = 0;
        y += charHeight;
        if(y + charHeight > h)
        {
            h = y + charHeight;
        }
    }
    else if(ch == L'\r')
    {
        x = 0;
    }
    else if(ch == L'\t')
    {
        x = floor(x / properties.tabWidth + 1) * properties.tabWidth;

        if(x > w)
        {
            w = x;
        }
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
        return ch != L' ';
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

float Text::xPos(wstring str, std::size_t cursorPosition, const TextProperties &properties)
{
    float x = 0, y = 0, w = 0, h = 0;

    for(wchar_t ch : str)
    {
        if(cursorPosition == 0)
            return x;
        cursorPosition--;
        updateFromChar(x, y, w, h, ch, properties);
    }

    return x;
}

float Text::yPos(wstring str, std::size_t cursorPosition, const TextProperties &properties)
{
    float totalHeight = height(str, properties);
    float x = 0, y = 0, w = 0, h = 0;

    for(wchar_t ch : str)
    {
        if(cursorPosition == 0)
            return totalHeight - y - 1;
        cursorPosition--;
        updateFromChar(x, y, w, h, ch, properties);
    }

    return totalHeight - y - 1;
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
            renderChar(retval, mat, color, ch, properties);
        }
    }

    return retval;
}

}
}

#if 0
#define MAKE_VECTOR_FONT
#endif // 1

#ifdef MAKE_VECTOR_FONT
#include <iostream>
#include <cstdlib>
namespace programmerjake
{
namespace voxels
{
namespace Text
{
namespace
{
ColorI getGlyphPixel(std::size_t glyphIndex, int x, int y)
{
    if(x < 0 || x >= fontWidth || y < 0 || y >= fontHeight || glyphIndex >= 0x100)
    {
        return GrayscaleAI(0, 0);
    }
    int i = glyphIndex;
    int left = (i % (textureXRes / fontWidth)) * fontWidth;
    int top = (i / (textureXRes / fontWidth)) * fontHeight;
    const int width = fontWidth;
    const int height = fontHeight;
    float minU = (left + pixelOffset) / textureXRes;
    float maxU = (left + width - pixelOffset) / textureXRes;
    float minV = 1 - (top + height - pixelOffset) / textureYRes;
    float maxV = 1 - (top + pixelOffset) / textureYRes;
    TextureDescriptor texture = FontTexture.tdNoOffset();
    texture = texture.subTexture(minU, maxU, minV, maxV);
    float u = interpolate<float>((float)x / width, texture.minU, texture.maxU);
    float v = interpolate<float>((float)y / width, texture.maxV, texture.minV);
    int ix = (int)(u * texture.image.width());
    int iy = (int)((1 - v) * texture.image.width());
    return texture.image.getPixel(ix, iy)
}

bool testGlyphPixel(std::size_t glyphIndex, int x, int y) // returns true if the glyph is non-transparent at the selected pixel
{
    return getGlyphPixel(glyphIndex, x, y).a != 0;
}

int steppedSin(int angle)
{
    angle &= 7;
    switch(angle)
    {
    case 1:
    case 2:
    case 3:
        return 1;
    case 5:
    case 6:
    case 7:
        return -1;
    default:
        return 0;
    }
}

int steppedCos(int angle)
{
    angle &= 7;
    switch(angle)
    {
    case 0:
    case 1:
    case 7:
        return 1;
    case 3:
    case 4:
    case 5:
        return -1;
    default:
        return 0;
    }
}

Initializer init1([]()
{
    using namespace std;
    for(std::size_t glyphIndex = 0; glyphIndex < 0x100; glyphIndex++)
    {
        constexpr int pixelsXOffset = 1, pixelsYOffset = 1;
        constexpr int pixelsXSize = 2 * pixelsXOffset + fontWidth, pixelsYSize = 2 * pixelsYOffset + fontHeight;
        checked_array<checked_array<bool, pixelsYSize>, pixelsXSize> pixels;
        checked_array<checked_array<bool, pixelsYSize>, pixelsXSize> borderInsidePixels;
        for(int y = 0; y < pixelsYSize; y++)
        {
            for(int x = 0; x < pixelsXSize; x++)
            {
                pixels[x][y] = testGlyphPixel(glyphIndex, x - pixelsXOffset, y - pixelsYOffset);
                borderInsidePixels[x][y] = false;
            }
        }
        for(int y = 0; y < fontHeight; y++)
        {
            for(int x = 0; x < fontWidth; x++)
            {
                bool borderInsidePixel = false;
                if(pixels[x + pixelsXOffset][y + pixelsYOffset])
                {
                    borderInsidePixel = !pixels[x + 1 + pixelsXOffset][y + 0 + pixelsYOffset] || borderInsidePixel;
                    borderInsidePixel = !pixels[x + 1 + pixelsXOffset][y + 1 + pixelsYOffset] || borderInsidePixel;
                    borderInsidePixel = !pixels[x + 0 + pixelsXOffset][y + 1 + pixelsYOffset] || borderInsidePixel;
                    borderInsidePixel = !pixels[x - 1 + pixelsXOffset][y + 1 + pixelsYOffset] || borderInsidePixel;
                    borderInsidePixel = !pixels[x - 1 + pixelsXOffset][y + 0 + pixelsYOffset] || borderInsidePixel;
                    borderInsidePixel = !pixels[x - 1 + pixelsXOffset][y - 1 + pixelsYOffset] || borderInsidePixel;
                    borderInsidePixel = !pixels[x + 0 + pixelsXOffset][y - 1 + pixelsYOffset] || borderInsidePixel;
                    borderInsidePixel = !pixels[x + 1 + pixelsXOffset][y - 1 + pixelsYOffset] || borderInsidePixel;
                }
                borderInsidePixels[x + pixelsXOffset][y + pixelsYOffset] = borderInsidePixel;
            }
        }
        checked_array<checked_array<bool, fontHeight>, fontWidth> pixelHandled;
        for(auto &i : pixelHandled)
            for(bool &v : i)
                v = false;
        for(int y = 0; y < fontHeight; y++)
        {
            for(int x = 0; x < fontWidth; x++)
            {
                if(pixelHandled[x][y])
                    continue;
                #error finish
            }
        }
    }
    exit(0);
});
}
}
}
}
#endif // MAKE_VECTOR_FONT
