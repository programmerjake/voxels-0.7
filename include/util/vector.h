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
#ifndef VECTOR_H
#define VECTOR_H

#include "util/util.h"
#include "stream/stream.h"
#include <cmath>
#include <stdexcept>
#include <random>
#include <ostream>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
struct VectorF;

struct VectorI
{
    std::int32_t x, y, z;
    constexpr VectorI(std::int32_t x, std::int32_t y, std::int32_t z)
        : x(x), y(y), z(z)
    {
    }
public:
    constexpr VectorI(std::int32_t v = 0)
        : x(v), y(v), z(v)
    {
    }

    constexpr const VectorI operator -() const
    {
        return VectorI(-x, -y, -z);
    }

    constexpr const VectorI & operator +() const
    {
        return *this;
    }

    constexpr const VectorI operator +(const VectorI & r) const
    {
        return VectorI(x + r.x, y + r.y, z + r.z);
    }

    constexpr const VectorI operator -(const VectorI & r) const
    {
        return VectorI(x - r.x, y - r.y, z - r.z);
    }

    constexpr const VectorI operator *(const VectorI & r) const
    {
        return VectorI(x * r.x, y * r.y, z * r.z);
    }

    constexpr const VectorI operator *(std::int32_t r) const
    {
        return VectorI(x * r, y * r, z * r);
    }

    friend constexpr VectorI operator *(std::int32_t a, const VectorI & b)
    {
        return VectorI(a * b.x, a * b.y, a * b.z);
    }

    constexpr bool operator ==(const VectorI & r) const
    {
        return x == r.x && y == r.y && z == r.z;
    }

    constexpr bool operator !=(const VectorI & r) const
    {
        return x != r.x || y != r.y || z != r.z;
    }

    const VectorI & operator +=(const VectorI & r)
    {
        x += r.x;
        y += r.y;
        z += r.z;
        return *this;
    }
    const VectorI & operator -=(const VectorI & r)
    {
        x -= r.x;
        y -= r.y;
        z -= r.z;
        return *this;
    }
    const VectorI & operator *=(const VectorI & r)
    {
        x *= r.x;
        y *= r.y;
        z *= r.z;
        return *this;
    }
    const VectorI & operator *=(std::int32_t r)
    {
        x *= r;
        y *= r;
        z *= r;
        return *this;
    }

    friend constexpr std::int32_t dot(const VectorI & a, const VectorI & b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    friend constexpr std::int32_t absSquared(const VectorI & v)
    {
        return dot(v, v);
    }

    friend float abs(const VectorI & v)
    {
        return std::sqrt(absSquared(v));
    }

    friend constexpr VectorI cross(const VectorI & a, const VectorI & b)
    {
        return VectorI(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    friend std::ostream & operator <<(std::ostream & os, const VectorI & v)
    {
        return os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
    }

    friend std::wostream & operator <<(std::wostream & os, const VectorI & v)
    {
        return os << L"<" << v.x << L", " << v.y << L", " << v.z << L">";
    }

    static VectorI read(stream::Reader &reader)
    {
        std::int32_t x = stream::read<std::int32_t>(reader);
        std::int32_t y = stream::read<std::int32_t>(reader);
        std::int32_t z = stream::read<std::int32_t>(reader);
        return VectorI(x, y, z);
    }

    void write(stream::Writer &writer) const
    {
        stream::write<std::int32_t>(writer, x);
        stream::write<std::int32_t>(writer, y);
        stream::write<std::int32_t>(writer, z);
    }
};

struct VectorF
{
    float x, y, z;

    constexpr VectorF(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }

    constexpr VectorF(float v = 0)
        : x(v), y(v), z(v)
    {
    }

    constexpr VectorF(const VectorI & v)
        : x(v.x), y(v.y), z(v.z)
    {
    }

    explicit operator VectorI() const
    {
        return VectorI(ifloor(x), ifloor(y), ifloor(z));
    }

    constexpr const VectorF operator -() const
    {
        return VectorF(-x, -y, -z);
    }

    constexpr const VectorF & operator +() const
    {
        return *this;
    }

    constexpr const VectorF operator +(const VectorF & r) const
    {
        return VectorF(x + r.x, y + r.y, z + r.z);
    }

    friend constexpr VectorF operator +(const VectorF & l, const VectorI & r)
    {
        return l + (VectorF)r;
    }

    friend constexpr VectorF operator +(const VectorI & l, const VectorF & r)
    {
        return (VectorF)l + r;
    }

    constexpr const VectorF operator -(const VectorF & r) const
    {
        return VectorF(x - r.x, y - r.y, z - r.z);
    }

    friend constexpr VectorF operator -(const VectorF & l, const VectorI & r)
    {
        return l - (VectorF)r;
    }

    friend constexpr VectorF operator -(const VectorI & l, const VectorF & r)
    {
        return (VectorF)l - r;
    }

    constexpr const VectorF operator *(const VectorF & r) const
    {
        return VectorF(x * r.x, y * r.y, z * r.z);
    }

    friend constexpr VectorF operator *(const VectorF & l, const VectorI & r)
    {
        return l * (VectorF)r;
    }

    friend constexpr VectorF operator *(const VectorI & l, const VectorF & r)
    {
        return (VectorF)l * r;
    }

    constexpr const VectorF operator /(const VectorF & r) const
    {
        return VectorF(x / r.x, y / r.y, z / r.z);
    }

    constexpr const VectorF operator *(float r) const
    {
        return VectorF(x * r, y * r, z * r);
    }

    friend constexpr VectorF operator *(float a, const VectorF & b)
    {
        return VectorF(a * b.x, a * b.y, a * b.z);
    }

    constexpr const VectorF operator /(float r) const
    {
        return VectorF(x / r, y / r, z / r);
    }

    constexpr bool operator ==(const VectorF & r) const
    {
        return x == r.x && y == r.y && z == r.z;
    }

    constexpr bool operator !=(const VectorF & r) const
    {
        return x != r.x || y != r.y || z != r.z;
    }

    const VectorF & operator +=(const VectorF & r)
    {
        x += r.x;
        y += r.y;
        z += r.z;
        return *this;
    }
    const VectorF & operator -=(const VectorF & r)
    {
        x -= r.x;
        y -= r.y;
        z -= r.z;
        return *this;
    }
    const VectorF & operator *=(const VectorF & r)
    {
        x *= r.x;
        y *= r.y;
        z *= r.z;
        return *this;
    }
    const VectorF & operator /=(const VectorF & r)
    {
        x /= r.x;
        y /= r.y;
        z /= r.z;
        return *this;
    }
    const VectorF & operator *=(float r)
    {
        x *= r;
        y *= r;
        z *= r;
        return *this;
    }
    const VectorF & operator /=(float r)
    {
        x /= r;
        y /= r;
        z /= r;
        return *this;
    }

    friend constexpr float dot(const VectorF & a, const VectorF & b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    friend constexpr float dot(const VectorI & a, const VectorF & b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    friend constexpr float dot(const VectorF & a, const VectorI & b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    friend constexpr float absSquared(const VectorF & v)
    {
        return dot(v, v);
    }

    friend float abs(const VectorF & v)
    {
        return std::sqrt(absSquared(v));
    }

private:
    static float normalizeNoThrowHelper(float v)
    {
        return v == 0 ? 1 : v;
    }
public:

    friend VectorF normalizeNoThrow(VectorF v)
    {
        return v / normalizeNoThrowHelper(abs(v));
    }

    friend const VectorF normalize(const VectorF v)
    {
        float r = abs(v);
        if(v == 0)
        {
            throw std::domain_error("can't normalize <0, 0, 0>");
        }
        return v / r;
    }

    const VectorF static normalize(float x, float y, float z)
    {
        VectorF v(x, y, z);
        float r = abs(v);
        if(v == 0)
        {
            throw std::domain_error("can't normalize <0, 0, 0>");
        }
        return v / r;
    }

    float phi() const
    {
        float r = abs(*this);
        if(r == 0)
        {
            return 0;
        }
        float v = y / r;
        v = limit(v, -1.0f, 1.0f);
        return std::asin(v);
    }

    float theta() const
    {
        return std::atan2(x, z);
    }

    float rSpherical() const
    {
        return abs(*this);
    }

    friend constexpr VectorF cross(const VectorF & a, const VectorF & b)
    {
        return VectorF(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    template<typename RE>
    static VectorF random(RE & re)
    {
        VectorF retval;
        do
        {
            retval.x = std::uniform_real_distribution<float>(-1, 1)(re);
            retval.y = std::uniform_real_distribution<float>(-1, 1)(re);
            retval.z = std::uniform_real_distribution<float>(-1, 1)(re);
        }
        while(absSquared(retval) > 1 || absSquared(retval) < eps);
        return retval;
    }

    friend std::ostream & operator <<(std::ostream & os, const VectorF & v)
    {
        return os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
    }

    friend std::wostream & operator <<(std::wostream & os, const VectorF & v)
    {
        return os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
    }

    static VectorF read(stream::Reader &reader)
    {
        float x = stream::read_finite<float32_t>(reader);
        float y = stream::read_finite<float32_t>(reader);
        float z = stream::read_finite<float32_t>(reader);
        return VectorF(x, y, z);
    }

    void write(stream::Writer &writer) const
    {
        stream::write<float32_t>(writer, x);
        stream::write<float32_t>(writer, y);
        stream::write<float32_t>(writer, z);
    }
};

constexpr inline bool operator ==(const VectorF & a, const VectorI & b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

constexpr inline bool operator ==(const VectorI & a, const VectorF & b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

constexpr inline bool operator !=(const VectorF & a, const VectorI & b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

constexpr inline bool operator !=(const VectorI & a, const VectorF & b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

inline VectorF normalize(const VectorI & v)
{
    return normalize((VectorF)v);
}

inline VectorF normalizeNoThrow(const VectorI & v)
{
    return normalizeNoThrow((VectorF)v);
}

constexpr VectorF defaultGravityVector = VectorF(0, -9.8, 0);
}
}

#endif // VECTOR_H
