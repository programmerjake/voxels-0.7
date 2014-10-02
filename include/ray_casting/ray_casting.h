#ifndef RAY_CASTING_H_INCLUDED
#define RAY_CASTING_H_INCLUDED

#include "util/vector.h"
#include "util/position.h"
#include "util/block_face.h"
#include "util/enum_traits.h"
#include <algorithm>
#include <tuple>

using namespace std;

class World;
class Entity;

namespace RayCasting
{
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
    constexpr PositionF eval(float t) const
    {
        return startPosition + t * direction;
    }
    float collideWithPlane(pair<VectorF, float> planeEquation) const
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
        return collideWithPlane(pair<VectorF, float>(normal, d));
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
    tuple<bool, float, BlockFace> getAABoxEnterFace(VectorF minCorner, VectorF maxCorner) const
    {
        float minCornerX = collideWithPlane(VectorF(-1, 0, 0), minCorner.x);
        float maxCornerX = collideWithPlane(VectorF(-1, 0, 0), maxCorner.x);
        float xt = min(minCornerX, maxCornerX);
        BlockFace xbf = minCornerX < maxCornerX ? BlockFace::NX : BlockFace::PX;
        float minCornerY = collideWithPlane(VectorF(0, -1, 0), minCorner.y);
        float maxCornerY = collideWithPlane(VectorF(0, -1, 0), maxCorner.y);
        float yt = min(minCornerY, maxCornerY);
        BlockFace ybf = minCornerY < maxCornerY ? BlockFace::NY : BlockFace::PY;
        float minCornerZ = collideWithPlane(VectorF(0, 0, -1), minCorner.z);
        float maxCornerZ = collideWithPlane(VectorF(0, 0, -1), maxCorner.z);
        float zt = min(minCornerZ, maxCornerZ);
        BlockFace zbf = minCornerZ < maxCornerZ ? BlockFace::NZ : BlockFace::PZ;
        if(xt > yt && xt > zt)
            return tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt > zt)
            return tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
    }
    tuple<bool, float, BlockFace> getAABoxExitFace(VectorF minCorner, VectorF maxCorner) const
    {
        float minCornerX = collideWithPlane(VectorF(-1, 0, 0), minCorner.x);
        float maxCornerX = collideWithPlane(VectorF(-1, 0, 0), maxCorner.x);
        float xt = max(minCornerX, maxCornerX);
        BlockFace xbf = minCornerX > maxCornerX ? BlockFace::NX : BlockFace::PX;
        float minCornerY = collideWithPlane(VectorF(0, -1, 0), minCorner.y);
        float maxCornerY = collideWithPlane(VectorF(0, -1, 0), maxCorner.y);
        float yt = max(minCornerY, maxCornerY);
        BlockFace ybf = minCornerY > maxCornerY ? BlockFace::NY : BlockFace::PY;
        float minCornerZ = collideWithPlane(VectorF(0, 0, -1), minCorner.z);
        float maxCornerZ = collideWithPlane(VectorF(0, 0, -1), maxCorner.z);
        float zt = max(minCornerZ, maxCornerZ);
        BlockFace zbf = minCornerZ > maxCornerZ ? BlockFace::NZ : BlockFace::PZ;
        if(xt < yt && xt < zt)
            return tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt < zt)
            return tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return tuple<bool, float, BlockFace>(isPointInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
    }
};

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
    Entity *entity;
    constexpr Collision()
        : t(-1), type(Type::None), world(nullptr), blockPosition(), entity(nullptr)
    {
    }
    explicit constexpr Collision(World &world)
        : t(-1), type(Type::None), world(&world), blockPosition(), entity(nullptr)
    {
    }
    constexpr Collision(World &world, float t, PositionI blockPosition)
        : t(t), type(Type::Block), world(&world), blockPosition(blockPosition), entity(nullptr)
    {
    }
    constexpr Collision(World &world, float t, Entity &entity)
        : t(t), type(Type::Entity), world(&world), blockPosition(), entity(&entity)
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

#endif // RAY_CASTING_H_INCLUDED
