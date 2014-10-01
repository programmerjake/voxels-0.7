#ifndef RAY_CASTING_H_INCLUDED
#define RAY_CASTING_H_INCLUDED

#include "util/vector.h"
#include "util/position.h"
#include "util/block_face.h"

class World;

namespace RayCasting
{
struct Ray final
{
    PositionF startPosition;
    VectorF direction;
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
    bool isCollisionInAABoxIgnoreAxis(VectorF minCorner, VectorF maxCorner, VectorF pos) const
    {
        static_assert(ignoreAxis == 'X' || ignoreAxis == 'Y' || ignoreAxis == 'Z', "invalid ignore axis");
        bool isCollisionInBoxX = (ignoreAxis == 'X' || pos.x >= minCorner.x && pos.x <= maxCorner.x);
        bool isCollisionInBoxY = (ignoreAxis == 'Y' || pos.y >= minCorner.y && pos.y <= maxCorner.y);
        bool isCollisionInBoxZ = (ignoreAxis == 'Z' || pos.z >= minCorner.z && pos.z <= maxCorner.z);
        return isCollisionInBoxX && isCollisionInBoxY && isCollisionInBoxZ;
    }
public:
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
            return tuple<bool, float, BlockFace>(isCollisionInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt > zt)
            return tuple<bool, float, BlockFace>(isCollisionInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return tuple<bool, float, BlockFace>(isCollisionInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
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
            return tuple<bool, float, BlockFace>(isCollisionInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt < zt)
            return tuple<bool, float, BlockFace>(isCollisionInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return tuple<bool, float, BlockFace>(isCollisionInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
    }
};
}

#endif // RAY_CASTING_H_INCLUDED
