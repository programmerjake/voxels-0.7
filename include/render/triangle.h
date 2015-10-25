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
#include "util/util.h"
#include "stream/stream.h"
#include "util/math_constants.h"
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

constexpr TextureCoord interpolate(float t, TextureCoord a, TextureCoord b)
{
    return TextureCoord(interpolate(t, a.u, b.u), interpolate(t, a.v, b.v));
}

struct Vertex
{
    TextureCoord t; // do NOT change the order of the variables
    VectorF p;
    ColorF c;
    VectorF n;
    constexpr Vertex(TextureCoord t, VectorF p, ColorF c, VectorF n)
        : t(t), p(p), c(c), n(n)
    {
    }
    constexpr Vertex()
        : t(), p(), c(), n()
    {
    }
    constexpr bool operator ==(const Vertex &rt) const
    {
        return t == rt.t && p == rt.p && c == rt.c && n == rt.n;
    }
    constexpr bool operator !=(const Vertex &rt) const
    {
        return !operator ==(rt);
    }
};

inline Vertex interpolate(float t, Vertex a, Vertex b)
{
    return Vertex(interpolate(t, a.t, b.t), interpolate(t, a.p, b.p), interpolate(t, a.c, b.c), normalizeNoThrow(interpolate(t, a.n, b.n)));
}

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
    constexpr Triangle(Vertex v1, Vertex v2, Vertex v3)
        : t1(v1.t), p1(v1.p), c1(v1.c), n1(v1.n),
        t2(v2.t), p2(v2.p), c2(v2.c), n2(v2.n),
        t3(v3.t), p3(v3.p), c3(v3.c), n3(v3.n)
    {
    }
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
    constexpr Vertex v1() const
    {
        return Vertex(t1, p1, c1, n1);
    }
    constexpr Vertex v2() const
    {
        return Vertex(t2, p2, c2, n2);
    }
    constexpr Vertex v3() const
    {
        return Vertex(t3, p3, c3, n3);
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

struct CutTriangle final
{
    Triangle frontTriangles[2];
    size_t frontTriangleCount = 0;
    Triangle coplanarTriangles[1];
    size_t coplanarTriangleCount = 0;
    Triangle backTriangles[2];
    size_t backTriangleCount = 0;
private:
    static void triangulate(Triangle triangles[], size_t &triangleCount, const Vertex vertices[], size_t vertexCount)
    {
        size_t myTriangleCount = 0;
        if(vertexCount >= 3)
        {
            for(size_t i = 0, j = 1, k = 2; k < vertexCount; i++, j++, k++)
            {
                Triangle tri(vertices[0], vertices[j], vertices[k]);
                triangles[myTriangleCount++] = tri;
            }
        }
        triangleCount = myTriangleCount;
    }
public:
    CutTriangle(Triangle tri, VectorF planeNormal, float planeD)
    {
        const size_t triangleVertexCount = 3, resultMaxVertexCount = triangleVertexCount + 1;
        Vertex triVertices[triangleVertexCount] = {tri.v1(), tri.v2(), tri.v3()};
        float planeDistances[triangleVertexCount];
        bool isFront[triangleVertexCount];
        bool isBack[triangleVertexCount];
        bool isCoplanar = true, anyFront = false;
        for(size_t i = 0; i < triangleVertexCount; i++)
        {
            planeDistances[i] = dot(triVertices[i].p, planeNormal) + planeD;
            isFront[i] = planeDistances[i] > eps;
            isBack[i] = planeDistances[i] < -eps;
            if(isFront[i] || isBack[i])
                isCoplanar = false;
            if(isFront[i])
                anyFront = true;
        }
        if(isCoplanar)
        {
            coplanarTriangles[0] = tri;
            coplanarTriangleCount = 1;
            return;
        }
        if(anyFront)
        {
            for(size_t i = 0; i < triangleVertexCount; i++)
            {
                isFront[i] = !isBack[i];
            }
        }
        Vertex frontVertices[resultMaxVertexCount];
        size_t frontVertexCount = 0;
        Vertex backVertices[resultMaxVertexCount];
        size_t backVertexCount = 0;
        for(size_t i = 0, j = 1; i < triangleVertexCount; i++, j++, j %= triangleVertexCount)
        {
            if(isFront[i])
            {
                if(isFront[j])
                {
                    frontVertices[frontVertexCount++] = triVertices[i];
                }
                else
                {
                    frontVertices[frontVertexCount++] = triVertices[i];
                    float divisor = dot(triVertices[j].p - triVertices[i].p, planeNormal);
                    if(std::fabs(divisor) >= eps * eps)
                    {
                        float t = (-planeD - dot(triVertices[i].p, planeNormal)) / divisor;
                        Vertex v = interpolate(t, triVertices[i], triVertices[j]);
                        frontVertices[frontVertexCount++] = v;
                        backVertices[backVertexCount++] = v;
                    }
                }
            }
            else
            {
                if(isFront[j])
                {
                    backVertices[backVertexCount++] = triVertices[i];
                    float divisor = dot(triVertices[j].p - triVertices[i].p, planeNormal);
                    if(std::fabs(divisor) >= eps * eps)
                    {
                        float t = (-planeD - dot(triVertices[i].p, planeNormal)) / divisor;
                        Vertex v = interpolate(t, triVertices[i], triVertices[j]);
                        frontVertices[frontVertexCount++] = v;
                        backVertices[backVertexCount++] = v;
                    }
                }
                else
                {
                    backVertices[backVertexCount++] = triVertices[i];
                }
            }
        }
        triangulate(frontTriangles, frontTriangleCount, frontVertices, frontVertexCount);
        triangulate(backTriangles, backTriangleCount, backVertices, backVertexCount);
    }
};

inline CutTriangle cut(Triangle tri, VectorF planeNormal, float planeD)
{
    return CutTriangle(tri, planeNormal, planeD);
}

}
}

#endif // TRIANGLE_H_INCLUDED
