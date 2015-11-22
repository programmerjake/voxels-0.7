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

#ifndef PHYSICS_COLLISION_H_INCLUDED
#define PHYSICS_COLLISION_H_INCLUDED

#include "util/vector.h"

namespace programmerjake
{
namespace voxels
{
namespace physics
{
struct Collision final
{
    bool empty;
    float time;
    VectorF point;
    VectorF normal;
    constexpr Collision() : empty(true), time(-1), point(), normal()
    {
    }
    constexpr Collision(float time, VectorF point, VectorF normal)
        : empty(false), time(time), point(point), normal(normal)
    {
    }
};
}
}
}

#endif /* PHYSICS_COLLISION_H_INCLUDED */
