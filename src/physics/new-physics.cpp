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

#include "physics/new-physics.h"

namespace programmerjake
{
namespace voxels
{
namespace physics
{
Object::Object(PrivateAccessTag,
               World *world,
               ShapeTag tag,
               PositionF position,
               VectorF velocity,
               Properties properties)
    : tag(tag),
      shape(),
      properties(properties),
      newPosition(position),
      position(position),
      newVelocity(velocity),
      velocity(velocity),
      appliedForces(0),
      lastTime(0),
      world(world),
      isSupported(false)
{
    switch(tag)
    {
    case ShapeTag::Point:
        break;
    case ShapeTag::Box:
        construct_object(shape.box);
        break;
    case ShapeTag::Cylinder:
        construct_object(shape.cylinder);
        break;
    }
}

Object::~Object()
{
    switch(tag)
    {
    case ShapeTag::Point:
        break;
    case ShapeTag::Box:
        shape.box.~BoxShape();
        break;
    case ShapeTag::Cylinder:
        shape.cylinder.~CylinderShape();
        break;
    }
}
}
}
}
