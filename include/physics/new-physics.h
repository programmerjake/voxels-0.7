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
/*
 * Some of the algorithms and structures are from Bullet
 * Bullet license (zlib):
 *
 * Bullet Continuous Collision Detection and Physics Library
 * http://bulletphysics.org

 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in
 * a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not
 * be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#ifndef PHYSICS_PHYSICS_H_INCLUDED
#define PHYSICS_PHYSICS_H_INCLUDED

#include "physics/properties.h"
#include "physics/collision.h"
#include "physics/bbox.h"
#include "util/enum_traits.h"
#include "stream/stream.h"
#include "util/position.h"
#include "util/block_chunk.h"
#include "util/lock.h"
#include "util/rw_lock.h"
#include "util/block_iterator.h"
#include <cstdint>
#include <cmath>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <deque>

namespace programmerjake
{
namespace voxels
{
namespace physics
{
class Object;

class World final : public std::enable_shared_from_this<World>
{
    friend class Object;
    World(const World &rt) = delete;
    World &operator=(const World &rt) = delete;

private:
    struct PrivateAccessTag final
    {
    };

    enum class ShapeTag : std::uint8_t
    {
        None,
        Box,
        Cylinder,
        DEFINE_ENUM_LIMITS(None, Cylinder)
    };

    struct NoShape final
    {
    };

    struct BoxShape final
    {
        VectorF extents;
        explicit BoxShape(VectorF extents) : extents(extents)
        {
        }
    };

    struct CylinderShape final
    {
        float diameter;
        float height;
        explicit CylinderShape(float diameter, float height) : diameter(diameter), height(height)
        {
        }
    };

    class Shape final
    {
    private:
        ShapeTag tag;
        union Data
        {
            NoShape noShape;
            BoxShape box;
            CylinderShape cylinder;
            Data() : noShape()
            {
            }
        };
        Data data;
        void copyConstruct(const Shape &rt)
        {
            tag = rt.tag;
            switch(tag)
            {
            case ShapeTag::None:
                break;
            case ShapeTag::Box:
                construct_object(data.box, rt.data.box);
                break;
            case ShapeTag::Cylinder:
                construct_object(data.cylinder, rt.data.cylinder);
                break;
            }
        }
        void moveConstruct(Shape &rt)
        {
            tag = rt.tag;
            switch(tag)
            {
            case ShapeTag::None:
                break;
            case ShapeTag::Box:
                construct_object(data.box, std::move(rt.data.box));
                break;
            case ShapeTag::Cylinder:
                construct_object(data.cylinder, std::move(rt.data.cylinder));
                break;
            }
            rt.reset();
        }

    public:
        ShapeTag getTag() const
        {
            return tag;
        }
        template <ShapeTag checkWithTag>
        bool isShape() const
        {
            return tag == checkWithTag;
        }
        bool empty() const
        {
            return isShape<ShapeTag::None>();
        }
        bool isBox() const
        {
            return isShape<ShapeTag::Box>();
        }
        bool isCylinder() const
        {
            return isShape<ShapeTag::Cylinder>();
        }
        BoxShape getBox() const
        {
            assert(isBox());
            return data.box;
        }
        CylinderShape getCylinder() const
        {
            assert(isCylinder());
            return data.cylinder;
        }
        Shape() : tag(ShapeTag::None), data()
        {
        }
        explicit Shape(BoxShape box) : tag(ShapeTag::Box), data()
        {
            construct_object(data.box, std::move(box));
        }
        explicit Shape(CylinderShape cylinder) : tag(ShapeTag::Cylinder), data()
        {
            construct_object(data.cylinder, std::move(cylinder));
        }
        void reset()
        {
            switch(tag)
            {
            case ShapeTag::None:
                break;
            case ShapeTag::Box:
                data.box.~BoxShape();
                break;
            case ShapeTag::Cylinder:
                data.cylinder.~CylinderShape();
                break;
            }
            tag = ShapeTag::None;
        }
        Shape(const Shape &rt) : tag(), data()
        {
            copyConstruct(rt);
        }
        Shape(Shape &&rt)
        {
            moveConstruct(rt);
        }
        Shape &operator=(const Shape &rt)
        {
            if(&rt == this)
                return *this;
            reset();
            copyConstruct(rt);
            return *this;
        }
        Shape &operator=(Shape &&rt)
        {
            reset();
            moveConstruct(rt);
            return *this;
        }
        ~Shape()
        {
            reset();
        }
    };

    struct ObjectImp
    {
        Shape shape;
        Properties properties;
        PositionF position;
        VectorF velocity;
        bool isSupported;
        bool motor;
        VectorF motorVelocity;
        std::weak_ptr<Object> object;
        bool empty() const
        {
            return shape.empty();
        }
        BoundingBox getBoundingBox() const;
        ObjectImp(Shape shape,
                  Properties properties,
                  PositionF position,
                  VectorF velocity,
                  const std::shared_ptr<Object> &object,
                  bool isSupported = false)
            : shape(shape),
              properties(properties),
              position(position),
              velocity(velocity),
              isSupported(isSupported),
              motor(false),
              motorVelocity(0),
              object(object)
        {
        }
        ObjectImp()
            : shape(),
              properties(),
              position(VectorF(0), Dimension::Overworld),
              velocity(0.0f),
              isSupported(false),
              motor(false),
              motorVelocity(0),
              object()
        {
        }
    };

    struct SortingObjectForBroadPhase final
    {
        BoundingBox boundingBox;
        float sortingIntervalStart = 0;
        float sortingIntervalEnd = 0;
        std::size_t objectIndex;
        Properties::CollisionMaskType myCollisionMask;
        Properties::CollisionMaskType othersCollisionMask;
        Dimension dimension;
        SortingObjectForBroadPhase(BoundingBox boundingBox,
                                   std::size_t objectIndex,
                                   Properties::CollisionMaskType myCollisionMask,
                                   Properties::CollisionMaskType othersCollisionMask,
                                   Dimension dimension)
            : boundingBox(boundingBox),
              objectIndex(objectIndex),
              myCollisionMask(myCollisionMask),
              othersCollisionMask(othersCollisionMask),
              dimension(dimension)
        {
        }
        SortingObjectForBroadPhase()
            : boundingBox(), objectIndex(), myCollisionMask(0), othersCollisionMask(0), dimension()
        {
        }
    };

    std::mutex stepTimeLock;
    std::vector<ObjectImp> objects;
    std::vector<ObjectImp> lastObjectsArray;
    std::vector<SortingObjectForBroadPhase> sortedObjectsForBroadPhase;
    std::vector<std::pair<std::size_t, std::size_t>> collidingPairsFromBroadPhase;
    struct PublicState final
    {
        typedef std::mutex LockType;
        mutable LockType lock;
        double currentTime;
        std::deque<ObjectImp> newObjectsQueue;
    };
    PublicState publicState;
    void addToWorld(ObjectImp object);
    void readPublicData();
    void broadPhaseCollisionDetection(WorldLockManager &lock_manager);
    void narrowPhaseCollisionDetection(WorldLockManager &lock_manager);
    void setupConstraints(WorldLockManager &lock_manager);
    void solveConstraints(WorldLockManager &lock_manager);
    void integratePositions(double deltaTime, WorldLockManager &lock_manager);
    void updatePublicData(double deltaTime, WorldLockManager &lock_manager);
    static Collision collideBoxBox(const ObjectImp &me, const ObjectImp &other, std::size_t myObjectIndex, std::size_t otherObjectIndex);
    static Collision collideBoxCylinder(const ObjectImp &me, const ObjectImp &other, std::size_t myObjectIndex, std::size_t otherObjectIndex);
    static Collision collideCylinderBox(const ObjectImp &me, const ObjectImp &other, std::size_t myObjectIndex, std::size_t otherObjectIndex);
    static Collision collideCylinderCylinder(const ObjectImp &me, const ObjectImp &other, std::size_t myObjectIndex, std::size_t otherObjectIndex);
    static Collision collide(const ObjectImp &me, const ObjectImp &other, std::size_t myObjectIndex, std::size_t otherObjectIndex);

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
    double getCurrentTime() const
    {
        auto lockIt = make_unique_lock(publicState.lock);
        return publicState.currentTime;
    }
    void stepTime(double deltaTime, WorldLockManager &lock_manager);
};

class Object final
{
    friend class World;

private:
    static constexpr int objectMutexLog2Count = 4;
    static constexpr std::size_t objectMutexCount = 1 << objectMutexLog2Count;
    static constexpr std::size_t objectMutexModCountMask = objectMutexCount - 1;
    const std::size_t objectMutexIndex;
    static std::mutex &getObjectMutex(std::size_t objectMutexIndex)
    {
        static std::mutex mutexes[objectMutexCount];
        return mutexes[objectMutexIndex];
    }
    struct ObjectMutexIterator final
    {
        std::size_t index;
        ObjectMutexIterator(std::size_t index = objectMutexCount) : index(index)
        {
        }
        std::mutex &operator*() const
        {
            return getObjectMutex(index);
        }
        std::mutex *operator->() const
        {
            return &getObjectMutex(index);
        }
        ObjectMutexIterator &operator++()
        {
            index++;
            return *this;
        }
        ObjectMutexIterator operator++(int)
        {
            return ObjectMutexIterator(index++);
        }
        bool operator==(const ObjectMutexIterator &rt) const
        {
            return index == rt.index;
        }
        bool operator!=(const ObjectMutexIterator &rt) const
        {
            return index != rt.index;
        }
    };
    struct AllMutexesLock final
    {
        void lock()
        {
            lock_all(ObjectMutexIterator(0), ObjectMutexIterator());
        }
        bool try_lock()
        {
            return try_lock_all(ObjectMutexIterator(0), ObjectMutexIterator());
        }
        void unlock()
        {
            for(std::size_t i = 0; i < objectMutexCount; i++)
                getObjectMutex(i).unlock();
        }
    };
    static std::size_t newObjectMutexIndex()
    {
        static std::atomic_size_t nextObjectMutexIndex(0);
        return nextObjectMutexIndex++ & objectMutexModCountMask;
    }
    std::mutex &getMutex() const
    {
        return getObjectMutex(objectMutexIndex);
    }
    Properties properties;
    PositionF position;
    VectorF velocity;
    bool supported;
    bool motor;
    VectorF motorVelocity;
    typedef World::PrivateAccessTag PrivateAccessTag;

public:
    Properties getProperties() const
    {
        auto lockIt = make_unique_lock(getMutex());
        return properties;
    }
    void setProperties(Properties properties)
    {
        auto lockIt = make_unique_lock(getMutex());
        this->properties = properties;
    }
    PositionF getPosition() const
    {
        auto lockIt = make_unique_lock(getMutex());
        return position;
    }
    VectorF getVelocity() const
    {
        auto lockIt = make_unique_lock(getMutex());
        return velocity;
    }
    bool isSupported() const
    {
        auto lockIt = make_unique_lock(getMutex());
        return supported;
    }
    bool hasMotor() const
    {
        auto lockIt = make_unique_lock(getMutex());
        return motor;
    }
    VectorF getMotorVelocity() const
    {
        auto lockIt = make_unique_lock(getMutex());
        return motorVelocity;
    }
    std::pair<VectorF, bool> getMotor() const
    {
        auto lockIt = make_unique_lock(getMutex());
        return std::pair<VectorF, bool>(motorVelocity, motor);
    }
    void setMotorVelocity(VectorF motorVelocity)
    {
        auto lockIt = make_unique_lock(getMutex());
        this->motorVelocity = motorVelocity;
        motor = true;
    }
    void removeMotor()
    {
        auto lockIt = make_unique_lock(getMutex());
        this->motorVelocity = VectorF(0);
        motor = false;
    }
    Object(PrivateAccessTag privateAccessTag,
           PositionF position,
           VectorF velocity,
           Properties properties)
        : objectMutexIndex(newObjectMutexIndex()),
          properties(properties),
          position(position),
          velocity(velocity),
          supported(false),
          motor(false),
          motorVelocity(0)
    {
    }
    static std::shared_ptr<Object> makePoint(World &world,
                                             PositionF position,
                                             VectorF velocity,
                                             Properties properties)
    {
        std::shared_ptr<Object> retval =
            std::make_shared<Object>(PrivateAccessTag(), position, velocity, properties);
        world.addToWorld(World::ObjectImp(World::Shape(), properties, position, velocity, retval));
        return retval;
    }
    static std::shared_ptr<Object> makeBox(
        World &world, PositionF position, VectorF velocity, VectorF extents, Properties properties)
    {
        std::shared_ptr<Object> retval =
            std::make_shared<Object>(PrivateAccessTag(), position, velocity, properties);
        world.addToWorld(World::ObjectImp(
            World::Shape(World::BoxShape(extents)), properties, position, velocity, retval));
        return retval;
    }
    static std::shared_ptr<Object> makeCylinder(World &world,
                                                PositionF position,
                                                VectorF velocity,
                                                float diameter,
                                                float height,
                                                Properties properties)
    {
        std::shared_ptr<Object> retval =
            std::make_shared<Object>(PrivateAccessTag(), position, velocity, properties);
        world.addToWorld(World::ObjectImp(World::Shape(World::CylinderShape(diameter, height)),
                                          properties,
                                          position,
                                          velocity,
                                          retval));
        return retval;
    }
};
}
}
}

#endif /* PHYSICS_PHYSICS_H_INCLUDED */
