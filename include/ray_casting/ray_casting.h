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
#ifndef RAY_CASTING_H_INCLUDED
#define RAY_CASTING_H_INCLUDED

#include "util/vector.h"
#include "util/position.h"
#include "util/block_face.h"
#include "util/enum_traits.h"
#include <algorithm>
#include <tuple>
#include <cstdint>
#include <atomic>
#include <cassert>
#include <iterator>

namespace programmerjake
{
namespace voxels
{
class World;
struct Entity;

namespace RayCasting
{
typedef std::uint32_t BlockCollisionMask;
constexpr BlockCollisionMask BlockCollisionMaskGround = 1 << 0;
constexpr BlockCollisionMask BlockCollisionMaskFluid = 1 << 1;
constexpr BlockCollisionMask BlockCollisionMaskDefault = ~BlockCollisionMaskFluid;
constexpr BlockCollisionMask BlockCollisionMaskEverything = ~(BlockCollisionMask)0;
inline BlockCollisionMask allocateBlockCollisionMask()
{
    static std::atomic_int nextBit(2);
    int bit = nextBit++;
    assert(bit < 32);
    return (BlockCollisionMask)1 << bit;
}

struct Ray final
{
    PositionF startPosition;
    VectorF direction;
    constexpr Dimension dimension() const
    {
        return startPosition.d;
    }
    static constexpr float eps = 1e-4;
    constexpr Ray(PositionF startPosition, VectorF direction)
        : startPosition(startPosition), direction(direction)
    {
    }
    constexpr Ray()
        : startPosition(), direction()
    {
    }
    constexpr PositionF eval(float t) const
    {
        return startPosition + t * direction;
    }
    float collideWithPlane(std::pair<VectorF, float> planeEquation) const
    {
        float divisor = dot(std::get<0>(planeEquation), direction);
        if(divisor == 0)
            return -1;
        float numerator = -dot(std::get<0>(planeEquation), startPosition) - std::get<1>(planeEquation);
        float retval = numerator / divisor;
        return retval >= eps ? retval : -1;
    }
    float collideWithPlane(VectorF normal, float d) const
    {
        return collideWithPlane(std::pair<VectorF, float>(normal, d));
    }
private:
    template <char ignoreAxis>
    static bool isPointInAABoxIgnoreAxis(VectorF minCorner, VectorF maxCorner, VectorF pos)
    {
        static_assert(ignoreAxis == 'X' || ignoreAxis == 'Y' || ignoreAxis == 'Z' || ignoreAxis == ' ', "invalid ignore axis");
        bool isPointInBoxX = ignoreAxis == 'X' || (pos.x >= minCorner.x && pos.x <= maxCorner.x);
        bool isPointInBoxY = ignoreAxis == 'Y' || (pos.y >= minCorner.y && pos.y <= maxCorner.y);
        bool isPointInBoxZ = ignoreAxis == 'Z' || (pos.z >= minCorner.z && pos.z <= maxCorner.z);
        return isPointInBoxX && isPointInBoxY && isPointInBoxZ;
    }
public:
    static bool isPointInAABox(VectorF minCorner, VectorF maxCorner, VectorF pos)
    {
        return isPointInAABoxIgnoreAxis<' '>(minCorner, maxCorner, pos);
    }
    std::tuple<bool, float, BlockFace> getAABoxEnterFace(VectorF minCorner, VectorF maxCorner) const
    {
        float minCornerX = collideWithPlane(VectorF(-1, 0, 0), minCorner.x);
        float maxCornerX = collideWithPlane(VectorF(-1, 0, 0), maxCorner.x);
        float xt = std::min(minCornerX, maxCornerX);
        BlockFace xbf = minCornerX < maxCornerX ? BlockFace::NX : BlockFace::PX;
        float minCornerY = collideWithPlane(VectorF(0, -1, 0), minCorner.y);
        float maxCornerY = collideWithPlane(VectorF(0, -1, 0), maxCorner.y);
        float yt = std::min(minCornerY, maxCornerY);
        BlockFace ybf = minCornerY < maxCornerY ? BlockFace::NY : BlockFace::PY;
        float minCornerZ = collideWithPlane(VectorF(0, 0, -1), minCorner.z);
        float maxCornerZ = collideWithPlane(VectorF(0, 0, -1), maxCorner.z);
        float zt = std::min(minCornerZ, maxCornerZ);
        BlockFace zbf = minCornerZ < maxCornerZ ? BlockFace::NZ : BlockFace::PZ;
        if(xt > yt && xt > zt)
            return std::tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt > zt)
            return std::tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return std::tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
    }
    std::tuple<bool, float, BlockFace> getAABoxExitFace(VectorF minCorner, VectorF maxCorner) const
    {
        float minCornerX = collideWithPlane(VectorF(-1, 0, 0), minCorner.x);
        float maxCornerX = collideWithPlane(VectorF(-1, 0, 0), maxCorner.x);
        float xt = std::max(minCornerX, maxCornerX);
        BlockFace xbf = minCornerX > maxCornerX ? BlockFace::NX : BlockFace::PX;
        float minCornerY = collideWithPlane(VectorF(0, -1, 0), minCorner.y);
        float maxCornerY = collideWithPlane(VectorF(0, -1, 0), maxCorner.y);
        float yt = std::max(minCornerY, maxCornerY);
        BlockFace ybf = minCornerY > maxCornerY ? BlockFace::NY : BlockFace::PY;
        float minCornerZ = collideWithPlane(VectorF(0, 0, -1), minCorner.z);
        float maxCornerZ = collideWithPlane(VectorF(0, 0, -1), maxCorner.z);
        float zt = std::max(minCornerZ, maxCornerZ);
        BlockFace zbf = minCornerZ > maxCornerZ ? BlockFace::NZ : BlockFace::PZ;
        if(xt < yt && xt < zt && xt > 0)
            return std::tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt < zt && yt > 0)
            return std::tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return std::tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
    }
    friend Ray transform(Matrix tform, Ray r);
};

inline Ray transform(Matrix tform, Ray r)
{
    return Ray(transform(tform, r.startPosition), tform.applyNoTranslate(r.direction));
}

class RayBlockIterator
{
public:
    typedef const std::pair<float, PositionI> value_type;
private:
    Ray ray;
    std::pair<float, PositionI> currentValue;
    VectorF nextRayIntersectionTValues, rayIntersectionStepTValues;
    VectorI positionDeltaValues;
    explicit RayBlockIterator(Ray ray);
public:
    RayBlockIterator()
        : RayBlockIterator(Ray())
    {
    }
    friend RayBlockIterator makeRayBlockIterator(Ray ray);
    value_type &operator *() const
    {
        return currentValue;
    }
    value_type *operator ->() const
    {
        return &currentValue;
    }
    RayBlockIterator operator ++(int)
    {
        RayBlockIterator retval = *this;
        operator ++();
        return retval;
    }
    const RayBlockIterator &operator ++();
};

inline RayBlockIterator makeRayBlockIterator(Ray ray)
{
    return RayBlockIterator(ray);
}

struct Collision final
{
    float t;
    enum class Type
    {
        None,
        Block,
        Entity,
        DEFINE_ENUM_LIMITS(None, Entity)
    };
    Type type;
    World *world;
    PositionI blockPosition;
    BlockFaceOrNone blockFace;
    Entity *entity;
    constexpr Collision()
        : t(-1), type(Type::None), world(nullptr), blockPosition(), blockFace(BlockFaceOrNone::None), entity(nullptr)
    {
    }
    explicit constexpr Collision(World &world)
        : t(-1), type(Type::None), world(&world), blockPosition(), blockFace(BlockFaceOrNone::None), entity(nullptr)
    {
    }
    constexpr Collision(World &world, float t, PositionI blockPosition, BlockFaceOrNone blockFace)
        : t(t), type(Type::Block), world(&world), blockPosition(blockPosition), blockFace(blockFace), entity(nullptr)
    {
    }
    constexpr Collision(World &world, float t, Entity &entity)
        : t(t), type(Type::Entity), world(&world), blockPosition(), blockFace(), entity(&entity)
    {
    }
    constexpr bool valid() const
    {
        return t >= Ray::eps && world != nullptr && ((type == Type::Entity && entity != nullptr) || (type == Type::Block && entity == nullptr));
    }
    constexpr bool operator <(Collision r) const
    {
        return valid() && (!r.valid() || t < r.t);
    }
    constexpr bool operator ==(Collision r) const
    {
        return type == r.type && (type == Type::None || (t < Ray::eps && r.t < Ray::eps) || t == r.t) && (type != Type::Block || blockPosition == r.blockPosition) && (type != Type::Entity || entity == r.entity);
    }
    constexpr bool operator !=(Collision r) const
    {
        return !operator ==(r);
    }
    constexpr bool operator >(Collision r) const
    {
        return r < *this;
    }
    constexpr bool operator >=(Collision r) const
    {
        return !operator <(r);
    }
    constexpr bool operator <=(Collision r) const
    {
        return !(r < *this);
    }
};
}
}
}

#endif // RAY_CASTING_H_INCLUDED
