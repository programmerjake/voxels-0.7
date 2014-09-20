#ifndef TRIANGLE_H_INCLUDED
#define TRIANGLE_H_INCLUDED

#include "util/vector.h"
#include "util/matrix.h"
#include "util/color.h"
#include "stream/stream.h"
#include <algorithm>

using namespace std;

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
};

struct Triangle
{
    TextureCoord t1, t2, t3;
    VectorF p1, p2, p3;
    ColorF c1, c2, c3;
    VectorF n1, n2, n3;
    constexpr Triangle(VectorF p1, VectorF p2, VectorF p3, ColorF color = colorizeIdentity())
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, VectorF p2, TextureCoord t2, VectorF p3, TextureCoord t3, ColorF color = colorizeIdentity())
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, VectorF p2, ColorF c2, VectorF p3, ColorF c3)
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, ColorF c1, VectorF p2, TextureCoord t2, ColorF c2, VectorF p3, TextureCoord t3, ColorF c3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, TextureCoord t1, VectorF p2, ColorF c2, TextureCoord t2, VectorF p3, ColorF c3, TextureCoord t3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(VectorF p1, VectorF n1, VectorF p2, VectorF n2, VectorF p3, VectorF n3, ColorF color = colorizeIdentity())
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, VectorF n1, VectorF p2, TextureCoord t2, VectorF n2, VectorF p3, TextureCoord t3, VectorF n3, ColorF color = colorizeIdentity())
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, VectorF n1, VectorF p2, ColorF c2, VectorF n2, VectorF p3, ColorF c3, VectorF n3)
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, ColorF c1, VectorF n1, VectorF p2, TextureCoord t2, ColorF c2, VectorF n2, VectorF p3, TextureCoord t3, ColorF c3, VectorF n3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, TextureCoord t1, VectorF n1, VectorF p2, ColorF c2, TextureCoord t2, VectorF n2, VectorF p3, ColorF c3, TextureCoord t3, VectorF n3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle()
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(0), p2(0), p3(0), c1(colorizeIdentity()), c2(colorizeIdentity()), c3(colorizeIdentity()), n1(0), n2(0), n3(0)
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
};

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

#endif // TRIANGLE_H_INCLUDED
