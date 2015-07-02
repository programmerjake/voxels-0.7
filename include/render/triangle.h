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
#ifndef TRIANGLE_H_INCLUDED
#define TRIANGLE_H_INCLUDED

#include "util/vector.h"
#include "util/matrix.h"
#include "util/color.h"
#include "stream/stream.h"
#include <algorithm>
#include <tuple>
#include <cstddef> // offsetof

namespace programmerjake
{
namespace voxels
{
struct TextureCoord
{
    float u, v;
    constexpr TextureCoord(float u, float v)
        : u(u), v(v)
    {
    }
    constexpr TextureCoord()
        : u(0), v(0)
    {
    }
    static TextureCoord read(stream::Reader &reader)
    {
        float u = stream::read_finite<float32_t>(reader);
        float v = stream::read_finite<float32_t>(reader);
        return TextureCoord(u, v);
    }
    void write(stream::Writer &writer) const
    {
        stream::write<float32_t>(writer, u);
        stream::write<float32_t>(writer, v);
    }
    constexpr bool operator ==(const TextureCoord &rt) const
    {
        return u == rt.u && v == rt.v;
    }
    constexpr bool operator !=(const TextureCoord &rt) const
    {
        return !operator ==(rt);
    }
};

struct Triangle
{
    TextureCoord t1; // do NOT change the order of the variables
    VectorF p1;
    ColorF c1;
    VectorF n1;
    TextureCoord t2;
    VectorF p2;
    ColorF c2;
    VectorF n2;
    TextureCoord t3;
    VectorF p3;
    ColorF c3;
    VectorF n3;
    Triangle(VectorF p1, VectorF p2, VectorF p3, ColorF color = colorizeIdentity())
        : t1(0, 0),
          p1(p1),
          c1(color),
          n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t2(0, 0),
          p2(p2),
          c2(color),
          n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t3(0, 0),
          p3(p3),
          c3(color),
          n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, TextureCoord t1, VectorF p2, TextureCoord t2, VectorF p3, TextureCoord t3, ColorF color = colorizeIdentity())
        : t1(t1),
          p1(p1),
          c1(color),
          n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t2(t2),
          p2(p2),
          c2(color),
          n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t3(t3),
          p3(p3),
          c3(color),
          n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, ColorF c1, VectorF p2, ColorF c2, VectorF p3, ColorF c3)
        : t1(0, 0),
          p1(p1),
          c1(c1),
          n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t2(0, 0),
          p2(p2),
          c2(c2),
          n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t3(0, 0),
          p3(p3),
          c3(c3),
          n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, TextureCoord t1, ColorF c1, VectorF p2, TextureCoord t2, ColorF c2, VectorF p3, TextureCoord t3, ColorF c3)
        : t1(t1),
          p1(p1),
          c1(c1),
          n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t2(t2),
          p2(p2),
          c2(c2),
          n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t3(t3),
          p3(p3),
          c3(c3),
          n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, ColorF c1, TextureCoord t1, VectorF p2, ColorF c2, TextureCoord t2, VectorF p3, ColorF c3, TextureCoord t3)
        : t1(t1),
          p1(p1),
          c1(c1),
          n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t2(t2),
          p2(p2),
          c2(c2),
          n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))),
          t3(t3),
          p3(p3),
          c3(c3),
          n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(VectorF p1, VectorF n1, VectorF p2, VectorF n2, VectorF p3, VectorF n3, ColorF color = colorizeIdentity())
        : t1(0, 0),
          p1(p1),
          c1(color),
          n1(n1),
          t2(0, 0),
          p2(p2),
          c2(color),
          n2(n2),
          t3(0, 0),
          p3(p3),
          c3(color),
          n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, VectorF n1, VectorF p2, TextureCoord t2, VectorF n2, VectorF p3, TextureCoord t3, VectorF n3, ColorF color = colorizeIdentity())
        : t1(t1),
          p1(p1),
          c1(color),
          n1(n1),
          t2(t2),
          p2(p2),
          c2(color),
          n2(n2),
          t3(t3),
          p3(p3),
          c3(color),
          n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, VectorF n1, VectorF p2, ColorF c2, VectorF n2, VectorF p3, ColorF c3, VectorF n3)
        : t1(0, 0),
          p1(p1),
          c1(c1),
          n1(n1),
          t2(0, 0),
          p2(p2),
          c2(c2),
          n2(n2),
          t3(0, 0),
          p3(p3),
          c3(c3),
          n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, ColorF c1, VectorF n1, VectorF p2, TextureCoord t2, ColorF c2, VectorF n2, VectorF p3, TextureCoord t3, ColorF c3, VectorF n3)
        : t1(t1),
          p1(p1),
          c1(c1),
          n1(n1),
          t2(t2),
          p2(p2),
          c2(c2),
          n2(n2),
          t3(t3),
          p3(p3),
          c3(c3),
          n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, TextureCoord t1, VectorF n1, VectorF p2, ColorF c2, TextureCoord t2, VectorF n2, VectorF p3, ColorF c3, TextureCoord t3, VectorF n3)
        : t1(t1),
          p1(p1),
          c1(c1),
          n1(n1),
          t2(t2),
          p2(p2),
          c2(c2),
          n2(n2),
          t3(t3),
          p3(p3),
          c3(c3),
          n3(n3)
    {
    }
    constexpr Triangle()
        : t1(0, 0),
          p1(0),
          c1(colorizeIdentity()),
          n1(0),
          t2(0, 0),
          p2(0),
          c2(colorizeIdentity()),
          n2(0),
          t3(0, 0),
          p3(0),
          c3(colorizeIdentity()),
          n3(0)
    {
    }
    static Triangle read(stream::Reader &reader)
    {
        Triangle retval;
        retval.p1 = stream::read<VectorF>(reader);
        retval.c1 = stream::read<ColorF>(reader);
        retval.t1 = stream::read<TextureCoord>(reader);
        retval.n1 = stream::read<VectorF>(reader);
        retval.p2 = stream::read<VectorF>(reader);
        retval.c2 = stream::read<ColorF>(reader);
        retval.t2 = stream::read<TextureCoord>(reader);
        retval.n2 = stream::read<VectorF>(reader);
        retval.p3 = stream::read<VectorF>(reader);
        retval.c3 = stream::read<ColorF>(reader);
        retval.t3 = stream::read<TextureCoord>(reader);
        retval.n3 = stream::read<VectorF>(reader);
        return retval;
    }
    void write(stream::Writer &writer) const
    {
        stream::write<VectorF>(writer, p1);
        stream::write<ColorF>(writer, c1);
        stream::write<TextureCoord>(writer, t1);
        stream::write<VectorF>(writer, n1);
        stream::write<VectorF>(writer, p2);
        stream::write<ColorF>(writer, c2);
        stream::write<TextureCoord>(writer, t2);
        stream::write<VectorF>(writer, n2);
        stream::write<VectorF>(writer, p3);
        stream::write<ColorF>(writer, c3);
        stream::write<TextureCoord>(writer, t3);
        stream::write<VectorF>(writer, n3);
    }
    VectorF getPlaneNormal() const
    {
        return normalizeNoThrow(cross(p1 - p2, p1 - p3));
    }
    constexpr bool operator ==(const Triangle &rt) const
    {
        return t1 == rt.t1 && p1 == rt.p1 && c1 == rt.c1 && n1 == rt.n1 &&
            t2 == rt.t2 && p2 == rt.p2 && c2 == rt.c2 && n2 == rt.n2 &&
            t3 == rt.t3 && p3 == rt.p3 && c3 == rt.c3 && n3 == rt.n3;
    }
    constexpr bool operator !=(const Triangle &rt) const
    {
        return !operator ==(rt);
    }
private:
    std::pair<VectorF, float> getPlaneEquationHelper(VectorF normal) const
    {
        return std::pair<VectorF, float>(normal, -dot(p1, normal));
    }
public:
    std::pair<VectorF, float> getPlaneEquation() const
    {
        return getPlaneEquationHelper(getPlaneNormal());
    }
};

constexpr std::size_t Triangle_vertex_stride = offsetof(Triangle, t2) - offsetof(Triangle, t1);
constexpr std::size_t Triangle_texture_coord_start = offsetof(Triangle, t1);
constexpr std::size_t Triangle_position_start = offsetof(Triangle, p1);
constexpr std::size_t Triangle_color_start = offsetof(Triangle, c1);
constexpr std::size_t Triangle_normal_start = offsetof(Triangle, n1);

static_assert(sizeof(Triangle) == 3 * Triangle_vertex_stride, "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, t2) - offsetof(Triangle, t1), "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, t3) - offsetof(Triangle, t2), "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, p2) - offsetof(Triangle, p1), "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, p3) - offsetof(Triangle, p2), "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, c2) - offsetof(Triangle, c1), "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, c3) - offsetof(Triangle, c2), "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, n2) - offsetof(Triangle, n1), "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, n3) - offsetof(Triangle, n2), "triangle not packed correctly");
static_assert(sizeof(Triangle::p1) == sizeof(float[3]), "triangle not packed properly");
static_assert(sizeof(Triangle::t1) == sizeof(float[2]), "triangle not packed properly");
static_assert(sizeof(Triangle::c1) == sizeof(float[4]), "triangle not packed properly");

inline Triangle transform(const Matrix & m, const Triangle & t)
{
    return Triangle(transform(m, t.p1), t.t1, t.c1, transformNormal(m, t.n1),
                     transform(m, t.p2), t.t2, t.c2, transformNormal(m, t.n2),
                     transform(m, t.p3), t.t3, t.c3, transformNormal(m, t.n3));
}

constexpr Triangle colorize(ColorF color, const Triangle & t)
{
    return Triangle(t.p1, t.t1, colorize(color, t.c1), t.n1,
                     t.p2, t.t2, colorize(color, t.c2), t.n2,
                     t.p3, t.t3, colorize(color, t.c3), t.n3);
}

constexpr Triangle reverse(const Triangle & t)
{
    return Triangle(t.p1, t.t1, t.c1, -t.n1, t.p3, t.t3, t.c3, -t.n3, t.p2, t.t2, t.c2, -t.n2);
}
}
}

#endif // TRIANGLE_H_INCLUDED
