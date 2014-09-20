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
#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED

#include <cstdint>
#include <ostream>
#include "util/util.h"
#include "stream/stream.h"

using namespace std;

struct ColorI final
{
    uint8_t b, g, r, a;
    friend constexpr ColorI RGBAI(int r, int g, int b, int a);
    friend struct ColorF;
private:
    constexpr ColorI(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : b(b), g(g), r(r), a(a)
    {
    }
    union ConvertWithUInt32
    {
        uint32_t v;
        struct
        {
            uint8_t b, g, r, a;
        };
        constexpr ConvertWithUInt32(uint32_t v)
            : v(v)
        {
        }
        constexpr ConvertWithUInt32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
            : b(b), g(g), r(r), a(a)
        {
        }
    };
public:
    constexpr ColorI()
        : b(0), g(0), r(0), a(0)
    {
    }
    explicit constexpr ColorI(uint32_t v)
        : b(ConvertWithUInt32(v).b), g(ConvertWithUInt32(v).g), r(ConvertWithUInt32(v).r), a(ConvertWithUInt32(v).a)
    {

    }
    explicit constexpr operator uint32_t() const
    {
        return ConvertWithUInt32(r, g, b, a).v;
    }
    static ColorI read(stream::Reader &reader);
    void write(stream::Writer &writer) const
    {
        stream::write<uint8_t>(writer, b);
        stream::write<uint8_t>(writer, g);
        stream::write<uint8_t>(writer, r);
        stream::write<uint8_t>(writer, a);
    }
};

constexpr ColorI RGBAI(int r, int g, int b, int a)
{
    return ColorI((uint8_t)limit<int>(r, 0, 0xFF), (uint8_t)limit<int>(g, 0, 0xFF), (uint8_t)limit<int>(b, 0, 0xFF), (uint8_t)limit<int>(a, 0, 0xFF));
}

inline ColorI ColorI::read(stream::Reader &reader)
{
    uint8_t b = stream::read<uint8_t>(reader);
    uint8_t g = stream::read<uint8_t>(reader);
    uint8_t r = stream::read<uint8_t>(reader);
    uint8_t a = stream::read<uint8_t>(reader);
    return RGBAI(r, g, b, a);
}

constexpr ColorI RGBI(int r, int g, int b)
{
    return RGBAI(r, g, b, 0xFF);
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
    constexpr ColorF(float r, float g, float b, float a)
        : r(r), g(g), b(b), a(a)
    {
    }
public:
    constexpr ColorF()
        : r(0), g(0), b(0), a(0)
    {
    }
    explicit constexpr ColorF(ColorI c)
        : r((int)c.r * ((float)1 / 0xFF)), g((int)c.g * ((float)1 / 0xFF)), b((int)c.b * ((float)1 / 0xFF)), a((int)c.a * ((float)1 / 0xFF))
    {
    }
    explicit constexpr operator ColorI() const
    {
        return ColorI((int)limit<float>(r * 0xFF, 0, 0xFF), (int)limit<float>(g * 0xFF, 0, 0xFF), (int)limit<float>(b * 0xFF, 0, 0xFF), (int)limit<float>(a * 0xFF, 0, 0xFF));
    }
    friend ostream & operator <<(ostream & os, const ColorF & c)
    {
        return os << "RGBA(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ")";
    }
    static ColorF read(stream::Reader &reader)
    {
        return (ColorF)(ColorI)stream::read<ColorI>(reader);
    }
    void write(stream::Writer &writer) const
    {
        stream::write<ColorI>(writer, (ColorI)*this);
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
    return RGBAI((int)((int)v.r * color.r), (int)((int)v.g * color.g), (int)((int)v.b * color.b), (int)((int)v.a * color.a));
}

constexpr ColorF colorizeIdentity()
{
    return RGBAF(1, 1, 1, 1);
}

template <>
constexpr ColorF interpolate<ColorF>(const float t, const ColorF a, const ColorF b)
{
    return RGBAF(interpolate(t, a.r, b.r), interpolate(t, a.g, b.g), interpolate(t, a.b, b.b), interpolate(t, a.a, b.a));
}

#endif // COLOR_H_INCLUDED
