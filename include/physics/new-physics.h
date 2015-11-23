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
#include "physics/collision.h"
#include "util/enum_traits.h"
#include "stream/stream.h"
#include "util/position.h"
#include "util/block_chunk.h"
#include "util/lock.h"
#include <cstdint>
#include <cmath>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>

namespace programmerjake
{
namespace voxels
{
namespace physics
{
enum class ShapeTag : std::uint8_t
{
    Point,
    Box,
    Cylinder,
    DEFINE_ENUM_LIMITS(Point, Cylinder)
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

class Object;

class World final : public std::enable_shared_from_this<World>
{
    World(const World &rt) = delete;
    World &operator=(const World &rt) = delete;

private:
    struct PrivateAccessTag final
    {
    };

public:
    BlockChunkMap chunks;
    BlockIterator getBlockIterator(PositionI pos, TLS &tls)
    {
        return BlockIterator(&chunks, pos, tls);
    }
    World(PrivateAccessTag);
    ~World();
    static std::shared_ptr<World> make()
    {
        return std::make_shared<World>(PrivateAccessTag());
    }
    Object *make(PrivateAccessTag,
                 ShapeTag tag,
                 PositionF position,
                 VectorF velocity,
                 Properties properties);
};

class Object final
{
    friend class World;
    Object(const Object &rt) = delete;
    Object &operator=(const Object &rt) = delete;

private:
    union ShapeType
    {
        BoxShape box;
        CylinderShape cylinder;
        ShapeType()
        {
        }
        ShapeType(BoxShape box) : box(box)
        {
        }
        ShapeType(CylinderShape cylinder) : cylinder(cylinder)
        {
        }
        ~ShapeType()
        {
        }
    };
    const ShapeTag tag;
    ShapeType shape;
    Properties properties;
    PositionF newPosition;
    PositionF position;
    VectorF newVelocity;
    VectorF velocity;
    VectorF appliedForces;
    float lastTime;
    World *const world;
    bool isSupported;
    typedef World::PrivateAccessTag PrivateAccessTag;

public:
    Properties getProperties() const;
    void setProperties(Properties properties);
    ~Object();
    Object(PrivateAccessTag,
           World *world,
           ShapeTag tag,
           PositionF position,
           VectorF velocity,
           Properties properties);
    static std::shared_ptr<Object> makeEmpty(World &world,
                                             PositionF position,
                                             VectorF velocity,
                                             Properties properties);
    static std::shared_ptr<Object> makeBox(
        World &world, PositionF position, VectorF velocity, VectorF extents, Properties properties);
    static std::shared_ptr<Object> makeCylinder(World &world,
                                                PositionF position,
                                                VectorF velocity,
                                                Properties properties);
    BoundingBox getBoundingBox(float maxTime, PrivateAccessTag);
    bool isPoint() const
    {
        return tag == ShapeTag::Point;
    }
    Collision getNextCollision(const Object &other, float maxTime, PrivateAccessTag);
};
}
}
}

#endif /* PHYSICS_PHYSICS_H_INCLUDED */
