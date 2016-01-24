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
#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED

#include <cstdint>
#include <ostream>
#include "util/util.h"
#include "stream/stream.h"
#include <cmath>

namespace programmerjake
{
namespace voxels
{
struct ColorI final
{
    std::uint8_t b, g, r, a;
    friend constexpr ColorI RGBAI(int r, int g, int b, int a);
    friend struct ColorF;

private:
    constexpr ColorI(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
        : b(b), g(g), r(r), a(a)
    {
    }
    union ConvertWithUInt32
    {
        std::uint32_t v;
        struct
        {
            std::uint8_t b, g, r, a;
        };
        constexpr ConvertWithUInt32(std::uint32_t v) : v(v)
        {
        }
        constexpr ConvertWithUInt32(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
            : b(b), g(g), r(r), a(a)
        {
        }
    };

public:
    constexpr ColorI() : b(0), g(0), r(0), a(0)
    {
    }
    explicit constexpr ColorI(std::uint32_t v)
        : b(ConvertWithUInt32(v).b),
          g(ConvertWithUInt32(v).g),
          r(ConvertWithUInt32(v).r),
          a(ConvertWithUInt32(v).a)
    {
    }
    explicit constexpr operator std::uint32_t() const
    {
        return ConvertWithUInt32(r, g, b, a).v;
    }
    static ColorI read(stream::Reader &reader);
    void write(stream::Writer &writer) const
    {
        stream::write<std::uint8_t>(writer, b);
        stream::write<std::uint8_t>(writer, g);
        stream::write<std::uint8_t>(writer, r);
        stream::write<std::uint8_t>(writer, a);
    }
    constexpr bool operator==(const ColorI &rt) const
    {
        return b == rt.b && g == rt.g && r == rt.r && a == rt.a;
    }
    constexpr bool operator!=(const ColorI &rt) const
    {
        return !operator==(rt);
    }
};

constexpr ColorI RGBAI(int r, int g, int b, int a)
{
    return ColorI(static_cast<std::uint8_t>(limit<int>(r, 0, 0xFF)),
                  static_cast<std::uint8_t>(limit<int>(g, 0, 0xFF)),
                  static_cast<std::uint8_t>(limit<int>(b, 0, 0xFF)),
                  static_cast<std::uint8_t>(limit<int>(a, 0, 0xFF)));
}

inline ColorI ColorI::read(stream::Reader &reader)
{
    std::uint8_t b = stream::read<std::uint8_t>(reader);
    std::uint8_t g = stream::read<std::uint8_t>(reader);
    std::uint8_t r = stream::read<std::uint8_t>(reader);
    std::uint8_t a = stream::read<std::uint8_t>(reader);
    return RGBAI(r, g, b, a);
}

constexpr ColorI RGBI(int r, int g, int b)
{
    return RGBAI(r, g, b, 0xFF);
}

constexpr ColorI GrayscaleI(int v)
{
    return RGBI(v, v, v);
}

constexpr ColorI GrayscaleAI(int v, int a)
{
    return RGBAI(v, v, v, a);
}

constexpr ColorI scaleI(ColorI l, ColorI r)
{
    return RGBAI(l.r * r.r / 0xFF, l.g * r.g / 0xFF, l.b * r.b / 0xFF, l.a * r.a / 0xFF);
}

constexpr ColorI scaleI(int l, ColorI r)
{
    return RGBAI(l * r.r / 0xFF, l * r.g / 0xFF, l * r.b / 0xFF, r.a);
}

constexpr ColorI scaleI(ColorI l, int r)
{
    return RGBAI(l.r * r / 0xFF, l.g * r / 0xFF, l.b * r / 0xFF, l.a);
}

struct ColorF final
{
    friend constexpr ColorF RGBAF(float r, float g, float b, float a);
    float r, g, b, a; /// a is opacity -- 0 is transparent and 1 is opaque
private:
    constexpr ColorF(float r, float g, float b, float a) : r(r), g(g), b(b), a(a)
    {
    }

public:
    constexpr ColorF() : r(0), g(0), b(0), a(0)
    {
    }
    explicit constexpr ColorF(ColorI c)
        : r(static_cast<int>(c.r) * static_cast<float>(1.0 / 0xFF)),
          g(static_cast<int>(c.g) * static_cast<float>(1.0 / 0xFF)),
          b(static_cast<int>(c.b) * static_cast<float>(1.0 / 0xFF)),
          a(static_cast<int>(c.a) * static_cast<float>(1.0 / 0xFF))
    {
    }
    explicit constexpr operator ColorI() const
    {
        return ColorI(static_cast<int>(limit<float>(r * 0xFF, 0, 0xFF)),
                      static_cast<int>(limit<float>(g * 0xFF, 0, 0xFF)),
                      static_cast<int>(limit<float>(b * 0xFF, 0, 0xFF)),
                      static_cast<int>(limit<float>(a * 0xFF, 0, 0xFF)));
    }
    friend std::ostream &operator<<(std::ostream &os, const ColorF &c)
    {
        return os << "RGBA(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ")";
    }
    static ColorF read(stream::Reader &reader)
    {
        return static_cast<ColorF>(static_cast<ColorI>(stream::read<ColorI>(reader)));
    }
    void write(stream::Writer &writer) const
    {
        stream::write<ColorI>(writer, static_cast<ColorI>(*this));
    }
    constexpr bool operator==(const ColorF &rt) const
    {
        return r == rt.r && g == rt.g && b == rt.b && a == rt.a;
    }
    constexpr bool operator!=(const ColorF &rt) const
    {
        return !operator==(rt);
    }
};

constexpr ColorF RGBAF(float r, float g, float b, float a)
{
    return ColorF(r, g, b, a);
}

constexpr ColorF RGBF(float r, float g, float b)
{
    return RGBAF(r, g, b, 1);
}

constexpr ColorF GrayscaleF(float v)
{
    return RGBF(v, v, v);
}

constexpr ColorF GrayscaleAF(float v, float a)
{
    return RGBAF(v, v, v, a);
}

constexpr ColorF scaleF(ColorF l, ColorF r)
{
    return RGBAF(l.r * r.r, l.g * r.g, l.b * r.b, l.a * r.a);
}

constexpr ColorF scaleF(float l, ColorF r)
{
    return RGBAF(l * r.r, l * r.g, l * r.b, r.a);
}

constexpr ColorF scaleF(ColorF l, float r)
{
    return RGBAF(l.r * r, l.g * r, l.b * r, l.a);
}

constexpr ColorF colorize(ColorF color, ColorF v)
{
    return scaleF(color, v);
}

constexpr ColorI colorize(ColorF color, ColorI v)
{
    return RGBAI(static_cast<int>(static_cast<int>(v.r) * color.r),
                 static_cast<int>(static_cast<int>(v.g) * color.g),
                 static_cast<int>(static_cast<int>(v.b) * color.b),
                 static_cast<int>(static_cast<int>(v.a) * color.a));
}

constexpr ColorF colorizeIdentity()
{
    return RGBAF(1, 1, 1, 1);
}

template <>
constexpr ColorF interpolate<ColorF>(const float t, const ColorF a, const ColorF b)
{
    return RGBAF(interpolate(t, a.r, b.r),
                 interpolate(t, a.g, b.g),
                 interpolate(t, a.b, b.b),
                 interpolate(t, a.a, b.a));
}

inline ColorF HueF(float h)
{
    h -= std::floor(h);
    h *= 6;
    int index = static_cast<int>(std::floor(h));
    ColorF colors[6] = {
        RGBF(1, 0, 0), RGBF(1, 1, 0), RGBF(0, 1, 0), RGBF(0, 1, 1), RGBF(0, 0, 1), RGBF(1, 0, 1)};
    return interpolate(
        h - index, colors[index], colors[index + 1 >= 6 ? index + 1 - 6 : index + 1]);
}

inline ColorF HSLAF(float h, float s, float l, float a) /// hue saturation lightness
{
    ColorF hs = interpolate(s, GrayscaleF(0.5), HueF(h));
    ColorF hsl = (l < 0.5) ? interpolate(2 * l, GrayscaleF(0), hs) :
                             interpolate(2 * l - 1, hs, GrayscaleF(1));
    hsl.a = a;
    return hsl;
}

inline ColorF HSVAF(float h, float s, float v, float a) /// hue saturation value
{
    ColorF hs = interpolate(s, GrayscaleF(1), HueF(h));
    ColorF hsv = interpolate(v, GrayscaleF(0), hs);
    hsv.a = a;
    return hsv;
}

inline ColorF HSLF(float h, float s, float l)
{
    return HSLAF(h, s, l, 1);
}

inline ColorF HSVF(float h, float s, float v)
{
    return HSVAF(h, s, v, 1);
}
}
}

#endif // COLOR_H_INCLUDED
