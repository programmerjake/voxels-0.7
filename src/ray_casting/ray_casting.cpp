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
#include "ray_casting/ray_casting.h"
#include "util/util.h"
#include <cmath>
#include "util/logging.h"

namespace programmerjake
{
namespace voxels
{
namespace RayCasting
{
constexpr float Ray::eps;

namespace
{
void initRayBlockIteratorDimension(float rayDirection, float rayStartPosition, int currentPosition,
                                   float &nextRayIntersectionT, float &rayIntersectionStepT,
                                   int &positionDelta)
{
    if(rayDirection == 0)
        return;
    float invDirection = 1 / rayDirection;
    rayIntersectionStepT = std::fabs(invDirection);
    int targetPosition;
    if(rayDirection < 0)
    {
        targetPosition = iceil(rayStartPosition) - 1;
        positionDelta = -1;
    }
    else
    {
        targetPosition = ifloor(rayStartPosition) + 1;
        positionDelta = 1;
    }
    nextRayIntersectionT = (targetPosition - rayStartPosition) * invDirection;
}
}

RayBlockIterator::RayBlockIterator(Ray ray)
    : ray(ray), currentValue(Ray::eps, (PositionI)ray.startPosition), nextRayIntersectionTValues(0), rayIntersectionStepTValues(0), positionDeltaValues(0)
{
    PositionI &currentPosition = std::get<1>(currentValue);
    initRayBlockIteratorDimension(ray.direction.x, ray.startPosition.x,
                                  currentPosition.x, nextRayIntersectionTValues.x,
                                  rayIntersectionStepTValues.x, positionDeltaValues.x);
    initRayBlockIteratorDimension(ray.direction.y, ray.startPosition.y,
                                  currentPosition.y, nextRayIntersectionTValues.y,
                                  rayIntersectionStepTValues.y, positionDeltaValues.y);
    initRayBlockIteratorDimension(ray.direction.z, ray.startPosition.z,
                                  currentPosition.z, nextRayIntersectionTValues.z,
                                  rayIntersectionStepTValues.z, positionDeltaValues.z);
    //getDebugLog() << ray.startPosition << ray.direction << currentPosition << postnl;
}

const RayBlockIterator &RayBlockIterator::operator ++()
{
    PositionI &currentPosition = std::get<1>(currentValue);
    float &t = std::get<0>(currentValue);
    if(ray.direction.x != 0 && (ray.direction.y == 0 || nextRayIntersectionTValues.x < nextRayIntersectionTValues.y) && (ray.direction.z == 0 || nextRayIntersectionTValues.x < nextRayIntersectionTValues.z))
    {
        t = nextRayIntersectionTValues.x;
        nextRayIntersectionTValues.x += rayIntersectionStepTValues.x;
        currentPosition.x += positionDeltaValues.x;
    }
    else if(ray.direction.y != 0 && (ray.direction.z == 0 || nextRayIntersectionTValues.y < nextRayIntersectionTValues.z))
    {
        t = nextRayIntersectionTValues.y;
        nextRayIntersectionTValues.y += rayIntersectionStepTValues.y;
        currentPosition.y += positionDeltaValues.y;
    }
    else if(ray.direction.z != 0)
    {
        t = nextRayIntersectionTValues.z;
        nextRayIntersectionTValues.z += rayIntersectionStepTValues.z;
        currentPosition.z += positionDeltaValues.z;
    }
    //getDebugLog() << currentPosition << postnl;
    return *this;
}
}
}
}
