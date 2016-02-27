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
#include <limits>

namespace programmerjake
{
namespace voxels
{
struct TextureCoord
{
    float u, v;
    constexpr TextureCoord(float u, float v) : u(u), v(v)
    {
    }
    constexpr TextureCoord() : u(0), v(0)
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
    constexpr bool operator==(const TextureCoord &rt) const
    {
        return u == rt.u && v == rt.v;
    }
    constexpr bool operator!=(const TextureCoord &rt) const
    {
        return !operator==(rt);
    }
};

constexpr TextureCoord interpolate(float t, TextureCoord a, TextureCoord b)
{
    return TextureCoord(interpolate(t, a.u, b.u), interpolate(t, a.v, b.v));
}

struct Vertex
{
    // do NOT change the order of the variables
    TextureCoord t; /// texture coordinate
    VectorF p; /// position
    ColorF c; /// color
    VectorF n; /// normal
    constexpr Vertex(TextureCoord t, VectorF p, ColorF c, VectorF n) : t(t), p(p), c(c), n(n)
    {
    }
    constexpr Vertex() : t(), p(), c(), n()
    {
    }
    constexpr bool operator==(const Vertex &rt) const
    {
        return t == rt.t && p == rt.p && c == rt.c && n == rt.n;
    }
    constexpr bool operator!=(const Vertex &rt) const
    {
        return !operator==(rt);
    }
    static Vertex read(stream::Reader &reader)
    {
        Vertex retval;
        retval.p = stream::read<VectorF>(reader);
        retval.c = stream::read<ColorF>(reader);
        retval.t = stream::read<TextureCoord>(reader);
        retval.n = stream::read<VectorF>(reader);
        return retval;
    }
    void write(stream::Writer &writer) const
    {
        stream::write<VectorF>(writer, p);
        stream::write<ColorF>(writer, c);
        stream::write<TextureCoord>(writer, t);
        stream::write<VectorF>(writer, n);
    }
    friend Vertex reverse(const Vertex &v)
    {
        return Vertex(v.t, v.p, v.c, -v.n);
    }
};

inline Vertex interpolate(float t, Vertex a, Vertex b)
{
    return Vertex(interpolate(t, a.t, b.t),
                  interpolate(t, a.p, b.p),
                  interpolate(t, a.c, b.c),
                  normalizeNoThrow(interpolate(t, a.n, b.n)));
}

struct IndexedTriangle final
{
    typedef std::uint32_t IndexType;
    static_assert(std::numeric_limits<IndexType>::max() <= std::numeric_limits<std::size_t>::max(),
                  "index type is too big");
    IndexType v[3];
    constexpr IndexedTriangle(IndexType v0, IndexType v1, IndexType v2) : v{v0, v1, v2}
    {
    }
    constexpr IndexedTriangle() : v{}
    {
    }
    static IndexedTriangle read(stream::Reader &reader)
    {
        IndexedTriangle retval;
        retval.v[0] = stream::read<IndexType>(reader);
        retval.v[1] = stream::read<IndexType>(reader);
        retval.v[2] = stream::read<IndexType>(reader);
        return retval;
    }
    void write(stream::Writer &writer) const
    {
        stream::write<IndexType>(writer, v[0]);
        stream::write<IndexType>(writer, v[1]);
        stream::write<IndexType>(writer, v[2]);
    }
    static constexpr std::size_t indexMaxValue()
    {
        return std::numeric_limits<IndexType>::max();
    }
    constexpr IndexedTriangle offsettedBy(IndexType offset) const
    {
        return IndexedTriangle(v[0] + offset, v[1] + offset, v[2] + offset);
    }
    constexpr bool operator==(const IndexedTriangle &rt) const
    {
        return v[0] == rt.v[0] && v[1] == rt.v[1] && v[2] == rt.v[2];
    }
    constexpr bool operator!=(const IndexedTriangle &rt) const
    {
        return !operator==(rt);
    }
};

static_assert(sizeof(IndexedTriangle) == sizeof(IndexedTriangle::IndexType) * 3,
              "IndexedTriangle is wrong size");

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
        : t1(v1.t),
          p1(v1.p),
          c1(v1.c),
          n1(v1.n),
          t2(v2.t),
          p2(v2.p),
          c2(v2.c),
          n2(v2.n),
          t3(v3.t),
          p3(v3.p),
          c3(v3.c),
          n3(v3.n)
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
    Triangle(VectorF p1,
             TextureCoord t1,
             VectorF p2,
             TextureCoord t2,
             VectorF p3,
             TextureCoord t3,
             ColorF color = colorizeIdentity())
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
    Triangle(VectorF p1,
             TextureCoord t1,
             ColorF c1,
             VectorF p2,
             TextureCoord t2,
             ColorF c2,
             VectorF p3,
             TextureCoord t3,
             ColorF c3)
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
    Triangle(VectorF p1,
             ColorF c1,
             TextureCoord t1,
             VectorF p2,
             ColorF c2,
             TextureCoord t2,
             VectorF p3,
             ColorF c3,
             TextureCoord t3)
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
    constexpr Triangle(VectorF p1,
                       VectorF n1,
                       VectorF p2,
                       VectorF n2,
                       VectorF p3,
                       VectorF n3,
                       ColorF color = colorizeIdentity())
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
    constexpr Triangle(VectorF p1,
                       TextureCoord t1,
                       VectorF n1,
                       VectorF p2,
                       TextureCoord t2,
                       VectorF n2,
                       VectorF p3,
                       TextureCoord t3,
                       VectorF n3,
                       ColorF color = colorizeIdentity())
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
    constexpr Triangle(VectorF p1,
                       ColorF c1,
                       VectorF n1,
                       VectorF p2,
                       ColorF c2,
                       VectorF n2,
                       VectorF p3,
                       ColorF c3,
                       VectorF n3)
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
    constexpr Triangle(VectorF p1,
                       TextureCoord t1,
                       ColorF c1,
                       VectorF n1,
                       VectorF p2,
                       TextureCoord t2,
                       ColorF c2,
                       VectorF n2,
                       VectorF p3,
                       TextureCoord t3,
                       ColorF c3,
                       VectorF n3)
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
    constexpr Triangle(VectorF p1,
                       ColorF c1,
                       TextureCoord t1,
                       VectorF n1,
                       VectorF p2,
                       ColorF c2,
                       TextureCoord t2,
                       VectorF n2,
                       VectorF p3,
                       ColorF c3,
                       TextureCoord t3,
                       VectorF n3)
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
    constexpr bool operator==(const Triangle &rt) const
    {
        return t1 == rt.t1 && p1 == rt.p1 && c1 == rt.c1 && n1 == rt.n1 && t2 == rt.t2
               && p2 == rt.p2 && c2 == rt.c2 && n2 == rt.n2 && t3 == rt.t3 && p3 == rt.p3
               && c3 == rt.c3 && n3 == rt.n3;
    }
    constexpr bool operator!=(const Triangle &rt) const
    {
        return !operator==(rt);
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
static_assert(Triangle_vertex_stride == offsetof(Triangle, t2) - offsetof(Triangle, t1),
              "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, t3) - offsetof(Triangle, t2),
              "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, p2) - offsetof(Triangle, p1),
              "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, p3) - offsetof(Triangle, p2),
              "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, c2) - offsetof(Triangle, c1),
              "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, c3) - offsetof(Triangle, c2),
              "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, n2) - offsetof(Triangle, n1),
              "triangle not packed correctly");
static_assert(Triangle_vertex_stride == offsetof(Triangle, n3) - offsetof(Triangle, n2),
              "triangle not packed correctly");
static_assert(sizeof(Triangle::p1) == sizeof(float[3]), "triangle not packed properly");
static_assert(sizeof(Triangle::t1) == sizeof(float[2]), "triangle not packed properly");
static_assert(sizeof(Triangle::c1) == sizeof(float[4]), "triangle not packed properly");
static_assert(sizeof(Vertex::p) == sizeof(float[3]), "vertex not packed properly");
static_assert(sizeof(Vertex::t) == sizeof(float[2]), "vertex not packed properly");
static_assert(sizeof(Vertex::c) == sizeof(float[4]), "vertex not packed properly");

inline Triangle transform(const Transform &m, const Triangle &t)
{
    return Triangle(transform(m, t.p1),
                    t.t1,
                    t.c1,
                    transformNormal(m, t.n1),
                    transform(m, t.p2),
                    t.t2,
                    t.c2,
                    transformNormal(m, t.n2),
                    transform(m, t.p3),
                    t.t3,
                    t.c3,
                    transformNormal(m, t.n3));
}

inline Triangle transform(const Matrix &m, const Triangle &t)
{
    return transform(Transform(m), t);
}

constexpr Triangle colorize(ColorF color, const Triangle &t)
{
    return Triangle(t.p1,
                    t.t1,
                    colorize(color, t.c1),
                    t.n1,
                    t.p2,
                    t.t2,
                    colorize(color, t.c2),
                    t.n2,
                    t.p3,
                    t.t3,
                    colorize(color, t.c3),
                    t.n3);
}

inline Vertex transform(const Transform &m, const Vertex &v)
{
    return Vertex(v.t, transform(m, v.p), v.c, transformNormal(m, v.n));
}

constexpr Vertex colorize(ColorF color, const Vertex &v)
{
    return Vertex(v.t, v.p, colorize(color, v.c), v.n);
}

constexpr Triangle reverse(const Triangle &t)
{
    return Triangle(t.p1, t.t1, t.c1, -t.n1, t.p3, t.t3, t.c3, -t.n3, t.p2, t.t2, t.c2, -t.n2);
}

constexpr IndexedTriangle reverse(const IndexedTriangle &t)
{
    return IndexedTriangle(t.v[2], t.v[1], t.v[0]);
}

struct CutTriangle final
{
    static constexpr std::size_t triangleVertexCount = 3;
    static constexpr std::size_t resultMaxVertexCount = triangleVertexCount + 1;
    static constexpr std::size_t resultMaxTriangleCount = resultMaxVertexCount - triangleVertexCount + 1;
    Triangle frontTriangles[resultMaxTriangleCount];
    std::size_t frontTriangleCount = 0;
    Triangle coplanarTriangles[1];
    std::size_t coplanarTriangleCount = 0;
    Triangle backTriangles[resultMaxTriangleCount];
    std::size_t backTriangleCount = 0;

    template <typename TriangleType, typename VertexType>
    static void triangulate(TriangleType triangles[],
                            std::size_t &triangleCount,
                            const VertexType vertices[],
                            std::size_t vertexCount)
    {
        std::size_t myTriangleCount = 0;
        if(vertexCount >= 3)
        {
            for(std::size_t i = 0, j = 1, k = 2; k < vertexCount; i++, j++, k++)
            {
                TriangleType tri(vertices[0], vertices[j], vertices[k]);
                triangles[myTriangleCount++] = tri;
            }
        }
        triangleCount = myTriangleCount;
    }

    static constexpr bool isInFront(float planeDistance)
    {
        return planeDistance > eps;
    }

    static constexpr bool isBehind(float planeDistance)
    {
        return planeDistance < -eps;
    }

    static constexpr bool isCoplanar(float planeDistance)
    {
        return !isInFront(planeDistance) && !isBehind(planeDistance);
    }

    static constexpr float getPlaneDistance(VectorF point, VectorF planeNormal, float planeD)
    {
        return dot(point, planeNormal) + planeD;
    }

    /**
     *  @return true if the triangle is coplanar
     */
    template <typename VertexType,
              typename GetVertexPositionFn,
              typename AddBackVertexFn,
              typename AddFrontVertexFn,
              typename InterpolateVertexFn>
    static bool cutTriangleAlgorithm(const VectorF planeNormal,
                                     const float planeD,
                                     const VertexType &vertex0,
                                     const VertexType &vertex1,
                                     const VertexType &vertex2,
                                     GetVertexPositionFn getVertexPositionFn,
                                     AddBackVertexFn addBackVertexFn,
                                     AddFrontVertexFn addFrontVertexFn,
                                     InterpolateVertexFn interpolateVertexFn)
    {
        VertexType triVertices[triangleVertexCount] = {vertex0, vertex1, vertex2};
        float planeDistances[triangleVertexCount];
        bool isFront[triangleVertexCount];
        bool isBack[triangleVertexCount];
        bool isCoplanar = true, anyFront = false;
        for(std::size_t i = 0; i < triangleVertexCount; i++)
        {
            planeDistances[i] =
                dot(getVertexPositionFn(static_cast<const VertexType &>(triVertices[i])),
                    planeNormal) + planeD;
            isFront[i] = planeDistances[i] > eps;
            isBack[i] = planeDistances[i] < -eps;
            if(isFront[i] || isBack[i])
                isCoplanar = false;
            if(isFront[i])
                anyFront = true;
        }
        if(isCoplanar)
        {
            return true;
        }
        if(anyFront)
        {
            for(std::size_t i = 0; i < triangleVertexCount; i++)
            {
                isFront[i] = !isBack[i];
            }
        }
        for(std::size_t i = 0, j = 1; i < triangleVertexCount; i++, j++, j %= triangleVertexCount)
        {
            const VertexType &vertexI = triVertices[i];
            const VertexType &vertexJ = triVertices[j];
            if(isFront[i])
            {
                if(isFront[j])
                {
                    addFrontVertexFn(vertexI);
                }
                else
                {
                    addFrontVertexFn(vertexI);
                    VectorF positionI = getVertexPositionFn(vertexI);
                    VectorF positionJ = getVertexPositionFn(vertexJ);
                    float divisor = dot(positionJ - positionI, planeNormal);
                    if(std::fabs(divisor) >= eps * eps)
                    {
                        float t = (-planeD - dot(positionI, planeNormal)) / divisor;
                        const VertexType newVertex = interpolateVertexFn(t, vertexI, vertexJ);
                        addFrontVertexFn(newVertex);
                        addBackVertexFn(newVertex);
                    }
                }
            }
            else
            {
                if(isFront[j])
                {
                    addBackVertexFn(vertexI);
                    VectorF positionI = getVertexPositionFn(vertexI);
                    VectorF positionJ = getVertexPositionFn(vertexJ);
                    float divisor = dot(positionJ - positionI, planeNormal);
                    if(std::fabs(divisor) >= eps * eps)
                    {
                        float t = (-planeD - dot(positionI, planeNormal)) / divisor;
                        const VertexType newVertex = interpolateVertexFn(t, vertexI, vertexJ);
                        addFrontVertexFn(newVertex);
                        addBackVertexFn(newVertex);
                    }
                }
                else
                {
                    addBackVertexFn(vertexI);
                }
            }
        }
        return false;
    }

private:
    static VectorF getVertexPosition(const Vertex &v)
    {
        return v.p;
    }
    static Vertex interpolateVertex(float t, const Vertex &a, const Vertex &b)
    {
        return interpolate(t, a, b);
    }
    struct VertexAccumulator final
    {
        Vertex vertices[resultMaxVertexCount];
        std::size_t vertexCount = 0;
        void add(const Vertex &vertex)
        {
            assert(vertexCount < resultMaxVertexCount);
            vertices[vertexCount++] = vertex;
        }
    };
    struct VertexAccumulatorAddFunction final
    {
        VertexAccumulator &vertexAccumulator;
        VertexAccumulatorAddFunction(VertexAccumulator &vertexAccumulator)
            : vertexAccumulator(vertexAccumulator)
        {
        }
        void operator()(const Vertex &vertex)
        {
            vertexAccumulator.add(vertex);
        }
    };

public:
    CutTriangle(Triangle tri, VectorF planeNormal, float planeD)
    {
        VertexAccumulator backVertices, frontVertices;
        bool isCoplanar = cutTriangleAlgorithm(planeNormal,
                                               planeD,
                                               tri.v1(),
                                               tri.v2(),
                                               tri.v3(),
                                               getVertexPosition,
                                               VertexAccumulatorAddFunction(backVertices),
                                               VertexAccumulatorAddFunction(frontVertices),
                                               interpolateVertex);
        if(isCoplanar)
        {
            coplanarTriangles[coplanarTriangleCount++] = tri;
            return;
        }
        triangulate(frontTriangles, frontTriangleCount, frontVertices.vertices, frontVertices.vertexCount);
        triangulate(backTriangles, backTriangleCount, backVertices.vertices, backVertices.vertexCount);
    }
};

inline CutTriangle cut(Triangle tri, VectorF planeNormal, float planeD)
{
    return CutTriangle(tri, planeNormal, planeD);
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::TextureCoord> final
{
    hash<float> floatHasher;
    std::size_t operator()(const programmerjake::voxels::TextureCoord &v) const
    {
        return floatHasher(v.u) + 3 * floatHasher(v.v);
    }
};

template <>
struct hash<programmerjake::voxels::Vertex> final
{
    hash<programmerjake::voxels::VectorF> vectorHasher;
    hash<programmerjake::voxels::ColorF> colorHasher;
    hash<programmerjake::voxels::TextureCoord> textureCoordHasher;
    std::size_t operator()(const programmerjake::voxels::Vertex &v) const
    {
        return vectorHasher(v.p) + 3 * vectorHasher(v.n) + 5 * colorHasher(v.c)
               + 7 * textureCoordHasher(v.t);
    }
};

template <>
struct hash<programmerjake::voxels::IndexedTriangle> final
{
    hash<programmerjake::voxels::IndexedTriangle::IndexType> indexHasher;
    std::size_t operator()(const programmerjake::voxels::IndexedTriangle &v) const
    {
        return indexHasher(v.v[0]) + 8191 * indexHasher(v.v[1]) + 1021 * indexHasher(v.v[2]);
    }
};

template <>
struct hash<programmerjake::voxels::Triangle> final
{
    hash<programmerjake::voxels::Vertex> vertexHasher;
    std::size_t operator()(const programmerjake::voxels::Triangle &v) const
    {
        return vertexHasher(v.v1()) + 8191 * vertexHasher(v.v2()) + 1021 * vertexHasher(v.v3());
    }
};
}

#endif // TRIANGLE_H_INCLUDED
