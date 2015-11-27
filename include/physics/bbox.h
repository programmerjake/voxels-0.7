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

#ifndef PHYSICS_BBOX_H_INCLUDED
#define PHYSICS_BBOX_H_INCLUDED

#include "util/vector.h"

namespace programmerjake
{
namespace voxels
{
namespace physics
{
struct BoundingBox final
{
    VectorF minCorner;
    VectorF maxCorner;
    constexpr BoundingBox(VectorF minCorner, VectorF maxCorner)
        : minCorner(minCorner), maxCorner(maxCorner)
    {
    }
    BoundingBox() : minCorner(), maxCorner()
    {
    }
    constexpr BoundingBox operator|(const BoundingBox &rt) const
    {
        return BoundingBox(elementwiseMin(minCorner, rt.minCorner),
                           elementwiseMax(maxCorner, rt.maxCorner));
    }
    BoundingBox &operator|=(const BoundingBox &rt)
    {
        minCorner = elementwiseMin(minCorner, rt.minCorner);
        maxCorner = elementwiseMax(maxCorner, rt.maxCorner);
        return *this;
    }
    constexpr VectorF extent() const
    {
        return maxCorner - minCorner;
    }
    constexpr bool overlaps(const BoundingBox &other) const
    {
        return minCorner.x <= other.maxCorner.x && minCorner.y <= other.maxCorner.y
               && minCorner.z <= other.maxCorner.z && other.minCorner.x <= maxCorner.x
               && other.minCorner.y <= maxCorner.y && other.minCorner.z <= maxCorner.z;
    }
};
}
}
}

#endif /* PHYSICS_BBOX_H_INCLUDED */
