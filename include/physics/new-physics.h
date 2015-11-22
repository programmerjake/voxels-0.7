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
#ifndef PHYSICS_PHYSICS_H_INCLUDED
#define PHYSICS_PHYSICS_H_INCLUDED

#include "physics/properties.h"
#include "util/enum_traits.h"
#include "stream/stream.h"
#include <cstdint>
#include <cmath>
#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace physics
{

enum class ShapeTag : std::uint8_t
{
    Empty,
    Box,
    Cylinder,
    DEFINE_ENUM_LIMITS(Empty, Cylinder)
};

struct BoxShape final
{
    VectorF extents;
};

struct CylinderShape final
{
    float diameter;
    float height;
};

class World;

class Object final
{
    friend class World;
    Object(const Object &rt) = delete;
    Object &operator =(const Object &rt) = delete;
private:
    union ShapeType
    {
        BoxShape box;
        CylinderShape cylinder;
        ShapeType()
        {
        }
        ShapeType(BoxShape box)
            : box(box)
        {
        }
        ShapeType(CylinderShape cylinder)
            : cylinder(cylinder)
        {
        }
        ~ShapeType()
        {
        }
    };
    ShapeTag tag;
    ShapeType shape;
    Properties properties;
    PositionF newPosition;
    PositionF position;
    VectorF newVelocity;
    VectorF velocity;
    VectorF appliedForces;
    float lastTime;
    World * const world;
    struct PrivateAccessTag final
    {
    };
public:
    Properties getProperties() const;
    void setProperties(Properties properties);
    ~Object()
    {
        switch(tag)
        {
        case ShapeTag::Empty:
            break;
        case ShapeTag::Box:
            shape.box.~BoxShape();
            break;
        case ShapeTag::Cylinder:
            shape.cylinder.~CylinderShape();
            break;
        }
    }
    Object(PrivateAccessTag, World *world, ShapeTag tag, PositionF position,
        VectorF velocity, Properties properties)
        : tag(tag),
          shape(),
          properties(properties),
          newPosition(position),
          position(position),
          newVelocity(velocity),
          velocity(velocity),
          appliedForces(0),
          lastTime(0),
          world(world)
    {
        switch(tag)
        {
        case ShapeTag::Empty:
            break;
        case ShapeTag::Box:
            construct_object(shape.box);
            break;
        case ShapeTag::Cylinder:
            construct_object(shape.cylinder);
            break;
        }
    }
    static std::shared_ptr<Object> makeEmpty(World &world, PositionF position,
        VectorF velocity, Properties properties);
    static std::shared_ptr<Object> makeBox(World &world, PositionF position,
        VectorF velocity, VectorF extents, Properties properties);
    static std::shared_ptr<Object> makeCylinder(World &world,
        PositionF position, VectorF velocity, Properties properties);
};

}
}
}

#endif /* PHYSICS_PHYSICS_H_INCLUDED */
