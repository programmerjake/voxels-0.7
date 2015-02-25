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
#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include "util/dimension.h"
#include "util/vector.h"
#include "stream/stream.h"
#include "util/matrix.h"
#include <unordered_set>
#include <vector>
#include <list>

namespace programmerjake
{
namespace voxels
{
class PositionF;

struct PositionI : public VectorI
{
    Dimension d;
    constexpr PositionI()
        : d(Dimension::Overworld)
    {
    }
    constexpr PositionI(std::int32_t x, std::int32_t y, std::int32_t z, Dimension d)
        : VectorI(x, y, z), d(d)
    {
    }
    PositionI(VectorF p, Dimension d)
        : VectorI(p), d(d)
    {
    }
    constexpr PositionI(VectorI p, Dimension d)
        : VectorI(p), d(d)
    {
    }
    friend constexpr bool operator ==(const PositionI & a, const PositionI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend constexpr bool operator !=(const PositionI & a, const PositionI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
    friend constexpr bool operator ==(const VectorI & a, const PositionI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend constexpr bool operator !=(const VectorI & a, const PositionI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend constexpr bool operator ==(const PositionI & a, const VectorI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend constexpr bool operator !=(const PositionI & a, const VectorI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend constexpr PositionI operator +(PositionI a, VectorI b)
    {
        return PositionI((VectorI)a + b, a.d);
    }
    friend constexpr PositionI operator +(VectorI a, PositionI b)
    {
        return PositionI(a + (VectorI)b, b.d);
    }
    friend constexpr PositionI operator -(PositionI a, VectorI b)
    {
        return PositionI((VectorI)a - b, a.d);
    }
    friend constexpr PositionI operator -(VectorI a, PositionI b)
    {
        return PositionI(a - (VectorI)b, b.d);
    }
    friend constexpr PositionI operator *(PositionI a, VectorI b)
    {
        return PositionI((VectorI)a * b, a.d);
    }
    friend constexpr PositionI operator *(VectorI a, PositionI b)
    {
        return PositionI(a * (VectorI)b, b.d);
    }
    friend constexpr PositionI operator *(PositionI a, std::int32_t b)
    {
        return PositionI((VectorI)a * b, a.d);
    }
    friend constexpr PositionI operator *(std::int32_t a, PositionI b)
    {
        return PositionI(a * (VectorI)b, b.d);
    }
    constexpr PositionI operator -() const
    {
        return PositionI(-(VectorI)*this, d);
    }
    const PositionI & operator +=(VectorI rt)
    {
        *(VectorI *)this += rt;
        return *this;
    }
    const PositionI & operator -=(VectorI rt)
    {
        *(VectorI *)this -= rt;
        return *this;
    }
    const PositionI & operator *=(VectorI rt)
    {
        *(VectorI *)this *= rt;
        return *this;
    }
    const PositionI & operator *=(std::int32_t rt)
    {
        *(VectorI *)this *= rt;
        return *this;
    }
    friend constexpr VectorI operator +(const PositionI & l, const PositionI & r)
    {
        return (VectorI)l + (VectorI)r;
    }
    friend constexpr VectorI operator -(const PositionI & l, const PositionI & r)
    {
        return (VectorI)l - (VectorI)r;
    }
    static PositionI read(stream::Reader &reader)
    {
        VectorI v = VectorI::read(reader);
        Dimension d = stream::read<Dimension>(reader);
        return PositionI(v, d);
    }
    void write(stream::Writer &writer) const
    {
        VectorI::write(writer);
        stream::write<Dimension>(writer, d);
    }
    friend constexpr PositionF operator +(const VectorF &a, const PositionI &b);
    friend constexpr PositionF operator +(const PositionI &a, const VectorF &b);
    friend constexpr PositionF operator -(const VectorF &a, const PositionI &b);
    friend constexpr PositionF operator -(const PositionI &a, const VectorF &b);
    friend constexpr PositionF operator *(const VectorF &a, const PositionI &b);
    friend constexpr PositionF operator *(const PositionI &a, const VectorF &b);
};
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::PositionI>
{
    constexpr std::size_t operator ()(const programmerjake::voxels::PositionI & v) const
    {
        return (std::size_t)v.x + (std::size_t)v.y * 97 + (std::size_t)v.z * 8191 + (std::size_t)v.d * 65537;
    }
};
}

namespace programmerjake
{
namespace voxels
{
class PositionF : public VectorF
{
public:
    Dimension d;
    constexpr PositionF()
        : d(Dimension::Overworld)
    {
    }
    constexpr PositionF(PositionI p)
        : VectorF(p.x, p.y, p.z), d(p.d)
    {
    }
    constexpr PositionF(float x, float y, float z, Dimension d)
        : VectorF(x, y, z), d(d)
    {
    }
    constexpr PositionF(VectorF p, Dimension d)
        : VectorF(p.x, p.y, p.z), d(d)
    {
    }
    constexpr PositionF(VectorI p, Dimension d)
        : VectorF(p.x, p.y, p.z), d(d)
    {
    }
    explicit operator PositionI() const
    {
        return PositionI(VectorF(x, y, z), d);
    }
    friend constexpr bool operator ==(const PositionF & a, const PositionI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend constexpr bool operator !=(const PositionF & a, const PositionI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
    friend constexpr bool operator ==(const PositionI & a, const PositionF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend constexpr bool operator !=(const PositionI & a, const PositionF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
    friend constexpr bool operator ==(const PositionF & a, const PositionF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.d == b.d;
    }
    friend constexpr bool operator !=(const PositionF & a, const PositionF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.d != b.d;
    }
    friend constexpr bool operator ==(const VectorF & a, const PositionF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend constexpr bool operator !=(const VectorF & a, const PositionF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend constexpr bool operator ==(const PositionF & a, const VectorF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend constexpr bool operator !=(const PositionF & a, const VectorF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend constexpr bool operator ==(const VectorI & a, const PositionF & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend constexpr bool operator !=(const VectorI & a, const PositionF & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend constexpr bool operator ==(const PositionF & a, const VectorI & b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend constexpr bool operator !=(const PositionF & a, const VectorI & b)
    {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }
    friend constexpr PositionF operator +(PositionF a, VectorF b)
    {
        return PositionF((VectorF)a + b, a.d);
    }
    friend constexpr PositionF operator +(VectorF a, PositionF b)
    {
        return PositionF(a + (VectorF)b, b.d);
    }
    friend constexpr PositionF operator -(PositionF a, VectorF b)
    {
        return PositionF((VectorF)a - b, a.d);
    }
    friend constexpr PositionF operator -(VectorF a, PositionF b)
    {
        return PositionF(a - (VectorF)b, b.d);
    }
    friend constexpr PositionF operator *(PositionF a, VectorF b)
    {
        return PositionF((VectorF)a * b, a.d);
    }
    friend constexpr PositionF operator *(VectorF a, PositionF b)
    {
        return PositionF(a * (VectorF)b, b.d);
    }
    friend constexpr PositionF operator +(PositionF a, VectorI b)
    {
        return PositionF((VectorF)a + b, a.d);
    }
    friend constexpr PositionF operator +(VectorI a, PositionF b)
    {
        return PositionF(a + (VectorF)b, b.d);
    }
    friend constexpr PositionF operator -(PositionF a, VectorI b)
    {
        return PositionF((VectorF)a - b, a.d);
    }
    friend constexpr PositionF operator -(VectorI a, PositionF b)
    {
        return PositionF(a - (VectorF)b, b.d);
    }
    friend constexpr PositionF operator *(PositionF a, VectorI b)
    {
        return PositionF((VectorF)a * b, a.d);
    }
    friend constexpr PositionF operator *(VectorI a, PositionF b)
    {
        return PositionF(a * (VectorF)b, b.d);
    }
    friend constexpr PositionF operator *(PositionF a, float b)
    {
        return PositionF((VectorF)a * b, a.d);
    }
    friend constexpr PositionF operator *(float a, PositionF b)
    {
        return PositionF(a * (VectorF)b, b.d);
    }
    friend constexpr PositionF operator /(PositionF a, float b)
    {
        return PositionF((VectorF)a / b, a.d);
    }
    constexpr PositionF operator -() const
    {
        return PositionF(-(VectorF)*this, d);
    }
    const PositionF & operator +=(VectorF rt)
    {
        *(VectorF *)this += rt;
        return *this;
    }
    const PositionF & operator -=(VectorF rt)
    {
        *(VectorF *)this -= rt;
        return *this;
    }
    const PositionF & operator *=(VectorF rt)
    {
        *(VectorF *)this *= rt;
        return *this;
    }
    const PositionF & operator /=(VectorF rt)
    {
        *(VectorF *)this /= rt;
        return *this;
    }
    const PositionF & operator *=(float rt)
    {
        *(VectorF *)this *= rt;
        return *this;
    }
    const PositionF & operator /=(float rt)
    {
        *(VectorF *)this /= rt;
        return *this;
    }
    friend constexpr VectorF operator +(const PositionF & l, const PositionF & r)
    {
        return (VectorF)l + (VectorF)r;
    }
    friend constexpr VectorF operator -(const PositionF & l, const PositionF & r)
    {
        return (VectorF)l - (VectorF)r;
    }
    friend constexpr VectorF operator +(const PositionI & l, const PositionF & r)
    {
        return (VectorI)l + (VectorF)r;
    }
    friend constexpr VectorF operator -(const PositionI & l, const PositionF & r)
    {
        return (VectorI)l - (VectorF)r;
    }
    friend constexpr VectorF operator +(const PositionF & l, const PositionI & r)
    {
        return (VectorF)l + (VectorI)r;
    }
    friend constexpr VectorF operator -(const PositionF & l, const PositionI & r)
    {
        return (VectorF)l - (VectorI)r;
    }
    static PositionF read(stream::Reader &reader)
    {
        VectorF v = VectorF::read(reader);
        Dimension d = stream::read<Dimension>(reader);
        return PositionF(v, d);
    }
    void write(stream::Writer &writer) const
    {
        VectorF::write(writer);
        stream::write<Dimension>(writer, d);
    }
};

inline constexpr PositionF operator +(const VectorF &a, const PositionI &b)
{
    return a + (PositionF)b;
}

inline constexpr PositionF operator +(const PositionI &a, const VectorF &b)
{
    return (PositionF)a + b;
}

inline constexpr PositionF operator -(const VectorF &a, const PositionI &b)
{
    return a - (PositionF)b;
}

inline constexpr PositionF operator -(const PositionI &a, const VectorF &b)
{
    return (PositionF)a - b;
}

inline constexpr PositionF operator *(const VectorF &a, const PositionI &b)
{
    return a * (PositionF)b;
}

inline constexpr PositionF operator *(const PositionI &a, const VectorF &b)
{
    return (PositionF)a * b;
}

inline PositionF transform(const Matrix &tform, PositionF p)
{
    return PositionF(transform(tform, (VectorF)p), p.d);
}

struct UpdateList // in here because of include issues : should be in util.h
{
    std::unordered_set<PositionI> updatesSet;
    std::list<PositionI> updatesList;
    void add(PositionI pos)
    {
        if(std::get<1>(updatesSet.insert(pos)))
        {
            updatesList.push_back(pos);
        }
    }
    void clear()
    {
        updatesList.clear();
        updatesSet.clear();
    }
    void merge(const UpdateList & rt)
    {
        for(PositionI pos : rt.updatesList)
        {
            add(pos);
        }
    }
    void remove(PositionI pos)
    {
        if(updatesSet.erase(pos) > 0)
        {
            for(auto i = updatesList.begin(); i != updatesList.end();)
            {
                if(*i == pos)
                {
                    i = updatesList.erase(i);
                    break;
                }
                else
                    i++;
            }
        }
    }
    bool empty() const
    {
        return updatesList.empty();
    }
};
}
}

#endif // POSITION_H_INCLUDED
