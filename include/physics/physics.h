/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef PHYSICS_OBJECT_H_INCLUDED
#define PHYSICS_OBJECT_H_INCLUDED

#include <memory>
#include "util/matrix.h"
#include "util/position.h"
#include "stream/stream.h"
#include "util/enum_traits.h"
#include "util/block_chunk.h"
#include "block/block.h"
#include "util/block_iterator.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <functional>
#include <iostream>
#include <cmath>
#include <tuple>
#include <mutex>
#include "util/cached_variable.h"
#include "util/logging.h"
#include "util/object_counter.h"
#include "util/ordered_weak_ptr.h"
#include "util/lock.h"
#include "util/util.h"
#include "util/tls.h"

namespace programmerjake
{
namespace voxels
{
class PhysicsWorld;

struct PhysicsProperties final
{
    float bounceFactor, slideFactor, dragFactor;
    typedef std::uint32_t CollisionMaskType;
    CollisionMaskType
        myCollisionMask; // for checking when this object is colliding with other objects
    CollisionMaskType
        othersCollisionMask; // for checking when other objects are colliding with this object
    static constexpr CollisionMaskType blockCollisionMask = 1 << 0;
    static constexpr CollisionMaskType itemCollisionMask = 1 << 1;
    static constexpr CollisionMaskType playerCollisionMask = 1 << 2;
    static constexpr CollisionMaskType defaultCollisionMask = 0xFFFFFF & ~itemCollisionMask;
    VectorF gravity;
    explicit PhysicsProperties(CollisionMaskType myCollisionMask = defaultCollisionMask,
                               CollisionMaskType othersCollisionMask = defaultCollisionMask,
                               float bounceFactor = std::sqrt(0.5f),
                               float slideFactor = 1 - std::sqrt(0.5f),
                               VectorF gravity = defaultGravityVector,
                               float dragFactor = 1)
        : bounceFactor(limit(bounceFactor, 0.0f, 1.0f)),
          slideFactor(limit(slideFactor, 0.0f, 1.0f)),
          dragFactor(dragFactor),
          myCollisionMask(myCollisionMask),
          othersCollisionMask(othersCollisionMask),
          gravity(gravity)
    {
    }
    static PhysicsProperties read(stream::Reader &reader)
    {
        PhysicsProperties retval;
        retval.bounceFactor = stream::read_limited<float32_t>(reader, 0, 1);
        retval.slideFactor = stream::read_limited<float32_t>(reader, 0, 1);
        retval.myCollisionMask = stream::read<CollisionMaskType>(reader);
        retval.othersCollisionMask = stream::read<CollisionMaskType>(reader);
        retval.gravity = stream::read<VectorF>(reader);
        retval.dragFactor = stream::read_limited<float32_t>(reader, 0, 1e5);
        return retval;
    }
    void write(stream::Writer &writer) const
    {
        stream::write<float32_t>(writer, bounceFactor);
        stream::write<float32_t>(writer, slideFactor);
        stream::write<CollisionMaskType>(writer, myCollisionMask);
        stream::write<CollisionMaskType>(writer, othersCollisionMask);
        stream::write<VectorF>(writer, gravity);
        stream::write<float32_t>(writer, dragFactor);
    }
    bool canCollideWith(const PhysicsProperties &r) const
    {
        return (myCollisionMask & r.othersCollisionMask) != 0;
    }
};

struct PhysicsConstraintData
{
    PhysicsConstraintData() = default;
    virtual ~PhysicsConstraintData() = default;
    PhysicsConstraintData(const PhysicsConstraintData &) = default;
    PhysicsConstraintData(PhysicsConstraintData &&) = default;
    PhysicsConstraintData &operator=(const PhysicsConstraintData &) = default;
    PhysicsConstraintData &operator=(PhysicsConstraintData &&) = default;
};

struct PhysicsConstraintDescriptor
{
protected:
    constexpr PhysicsConstraintDescriptor()
    {
    }

public:
    virtual ~PhysicsConstraintDescriptor()
    {
    }
    virtual void run(const std::shared_ptr<PhysicsConstraintData> &data,
                     PositionF &position,
                     VectorF &velocity) const = 0;
};

struct PhysicsConstraint final
{
    PhysicsConstraint(const PhysicsConstraint &) = default;
    PhysicsConstraint &operator=(const PhysicsConstraint &) = default;
    PhysicsConstraint(PhysicsConstraint &&) = default;
    PhysicsConstraint &operator=(PhysicsConstraint &&) = default;
    const PhysicsConstraintDescriptor *descriptor;
    std::shared_ptr<PhysicsConstraintData> data;
    PhysicsConstraint(const PhysicsConstraintDescriptor *descriptor = nullptr,
                      std::shared_ptr<PhysicsConstraintData> data = nullptr)
        : descriptor(descriptor), data(data)
    {
    }
    void operator()(PositionF &position, VectorF &velocity) const
    {
        if(descriptor != nullptr)
            descriptor->run(data, position, velocity);
    }
    bool empty() const
    {
        return descriptor == nullptr;
    }
    static PhysicsConstraint read(stream::Reader &reader)
    {
        throw stream::IOException("can't read a physics constraint");
    }
    void write(stream::Writer &writer) const
    {
        throw stream::IOException("can't write a physics constraint");
    }
};

namespace stream
{
GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
template <>
struct read<PhysicsConstraint> : public read_base<PhysicsConstraint>
{
    GCC_PRAGMA(diagnostic pop)
    read(Reader &reader) : read_base<PhysicsConstraint>(PhysicsConstraint::read(reader))
    {
    }
};

template <>
struct write<PhysicsConstraint>
{
    write(Writer &writer, PhysicsConstraint value)
    {
        value.write(writer);
    }
};
}

class PhysicsObject;

class PhysicsCollisionHandler
{
public:
    virtual ~PhysicsCollisionHandler() = default;
    virtual void onCollide(std::shared_ptr<PhysicsObject> collidingObject,
                           std::shared_ptr<PhysicsObject> otherObject) = 0;
    virtual void onCollideWithBlock(std::shared_ptr<PhysicsObject> collidingObject,
                                    BlockIterator otherObject,
                                    WorldLockManager &lock_manager) = 0;
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
class PhysicsObject final : public std::enable_shared_from_this<PhysicsObject>
{
    GCC_PRAGMA(diagnostic pop)
    friend class PhysicsWorld;

public:
    ObjectCounter<PhysicsObject, 0> objectCounter;

private:
    checked_array<PositionF, 2> position;
    checked_array<VectorF, 2> velocity;
    checked_array<double, 2> objectTime;
    bool affectedByGravity;
    bool isStatic_;
    bool supported = false;
    bool destroyed = false;
    enum Type
    {
        Box,
        Cylinder,
        Empty,
    };
    Type type;
    VectorF extents;
    std::weak_ptr<PhysicsWorld> world;
    std::uint64_t latestUpdateTag = 0;
    std::size_t newStateCount = 0;
    PhysicsProperties properties;
    checked_array<BlockEffects, 2> blockEffects;
    std::shared_ptr<const std::vector<PhysicsConstraint>> constraints;
    std::shared_ptr<PhysicsCollisionHandler> collisionHandler;
    PhysicsObject(const PhysicsObject &) = delete;
    const PhysicsObject &operator=(const PhysicsObject &) = delete;
    PhysicsObject(PositionF position,
                  VectorF velocity,
                  bool affectedByGravity,
                  bool isStatic,
                  VectorF extents,
                  std::shared_ptr<PhysicsWorld> world,
                  PhysicsProperties properties,
                  Type type,
                  std::shared_ptr<PhysicsCollisionHandler> collisionHandler);
    void setToBlock(const BlockShape &shape, PositionI blockPosition)
    {
        if(shape.empty())
        {
            type = Type::Empty;
            return;
        }
        PositionF p = shape.offset + blockPosition;
        position[0] = p;
        position[1] = p;
        velocity[0] = VectorF(0);
        velocity[1] = VectorF(0);
        blockEffects[0] = nullptr;
        blockEffects[1] = nullptr;
        affectedByGravity = false;
        isStatic_ = true;
        type = Type::Box;
        extents = shape.extents;
        supported = true;
        properties = PhysicsProperties(PhysicsProperties::blockCollisionMask,
                                       PhysicsProperties::blockCollisionMask);
    }
    static std::shared_ptr<PhysicsObject> makeEmptyNoAddToWorld(
        PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> world);

public:
    static std::shared_ptr<PhysicsObject> makeBox(
        PositionF position,
        VectorF velocity,
        bool affectedByGravity,
        bool isStatic,
        VectorF extents,
        PhysicsProperties properties,
        std::shared_ptr<PhysicsWorld> world,
        std::shared_ptr<PhysicsCollisionHandler> collisionHandler = nullptr);
    static std::shared_ptr<PhysicsObject> makeCylinder(
        PositionF position,
        VectorF velocity,
        bool affectedByGravity,
        bool isStatic,
        float radius,
        float yExtents,
        PhysicsProperties properties,
        std::shared_ptr<PhysicsWorld> world,
        std::shared_ptr<PhysicsCollisionHandler> collisionHandler = nullptr);
    static std::shared_ptr<PhysicsObject> makeEmpty(PositionF position,
                                                    VectorF velocity,
                                                    std::shared_ptr<PhysicsWorld> world,
                                                    VectorF gravity = VectorF(0));
    ~PhysicsObject();
    PositionF getPosition() const;
    VectorF getVelocity() const;
    BlockEffects getBlockEffects() const;
    std::shared_ptr<PhysicsObject> setConstraints(
        std::shared_ptr<const std::vector<PhysicsConstraint>> constraints = nullptr)
    {
        this->constraints = constraints;
        return shared_from_this();
    }
    std::shared_ptr<PhysicsObject> setConstraints(const std::vector<PhysicsConstraint> &constraints)
    {
        this->constraints = std::make_shared<std::vector<PhysicsConstraint>>(constraints);
        return shared_from_this();
    }
    std::shared_ptr<PhysicsCollisionHandler> getCollisionHandler() const
    {
        return collisionHandler;
    }
    bool isAffectedByGravity() const
    {
        return affectedByGravity;
    }
    bool isSupported() const
    {
        return supported;
    }
    bool isStatic() const
    {
        return isStatic_;
    }
    bool isDestroyed() const
    {
        return destroyed;
    }
    void destroy()
    {
        destroyed = true;
    }
    VectorF getExtents() const
    {
        return extents;
    }
    bool isCylinder() const
    {
        return type == Type::Cylinder;
    }
    bool isBox() const
    {
        return type == Type::Box;
    }
    bool isEmpty() const
    {
        return type == Type::Empty;
    }
    std::shared_ptr<PhysicsWorld> getWorld() const
    {
        std::shared_ptr<PhysicsWorld> world = this->world.lock();
        assert(world);
        return world;
    }
    PhysicsProperties getProperties() const;
    VectorF getGravity() const
    {
        return getProperties().gravity;
    }

private:
    void setNewState(PositionF newPosition, VectorF newVelocity);
    void setupNewState();
    void adjustPosition(const PhysicsObject &rt);

public:
    void setCurrentState(PositionF newPosition, VectorF newVelocity);
    void setCurrentState(PositionF newPosition, VectorF newVelocity, VectorF gravity);
    void setGravity(VectorF gravity);
    bool collides(const PhysicsObject &rt) const;
    bool isSupportedBy(const PhysicsObject &rt) const;
    void transferToNewWorld(std::shared_ptr<PhysicsWorld> newWorld);
    bool collidesWithBlock(const BlockShape &shape, PositionI blockPosition)
    {
        std::shared_ptr<PhysicsObject> temp =
            makeEmptyNoAddToWorld(PositionF(), VectorF(), world.lock());
        temp->setToBlock(shape, blockPosition);
        return collides(*temp);
    }
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
class PhysicsWorld final : public std::enable_shared_from_this<PhysicsWorld>
{
    GCC_PRAGMA(diagnostic pop)
    friend class PhysicsObject;

private:
    mutable checked_recursive_lock<200> theLockImp;
    mutable generic_lock_wrapper theLock; // lock for all state except for this->chunks
    double currentTime = 0;
    int variableSetIndex = 0;

private:
    static BlockShape getBlockShape(const Block &b)
    {
        if(!b)
            return BlockShape(VectorF(0.5f), VectorF(0.5f));
        return b.descriptor->blockShape;
    }

public:
    PhysicsWorld()
        : theLockImp(),
          theLock(theLockImp),
          chunks(),
          objects(),
          eventsQueue(),
          eventsSet(),
          changedObjects()
    {
    }
    BlockChunkMap chunks; // not locked by theLock
    BlockIterator getBlockIterator(PositionI pos, TLS &tls)
    {
        return BlockIterator(&chunks, pos, tls);
    }

private:
    static BlockShape getBlockShape(BlockIterator bi, WorldLockManager &lock_manager)
    {
        return getBlockShape(bi.get(lock_manager));
    }
    BlockShape getBlockShape(PositionI pos, WorldLockManager &lock_manager)
    {
        BlockIterator bi = getBlockIterator(pos, lock_manager.tls);
        return getBlockShape(bi, lock_manager);
    }
    void setObjectToBlockShape(std::shared_ptr<PhysicsObject> &object,
                               BlockShape shape,
                               PositionI pos)
    {
        if(object == nullptr)
            object = PhysicsObject::makeEmptyNoAddToWorld(pos, VectorF(0), shared_from_this());
        object->setToBlock(shape, pos);
    }
    void setObjectToBlock(std::shared_ptr<PhysicsObject> &object,
                          PositionI pos,
                          WorldLockManager &lock_manager)
    {
        if(object == nullptr)
            object = PhysicsObject::makeEmptyNoAddToWorld(pos, VectorF(0), shared_from_this());
        object->setToBlock(getBlockShape(pos, lock_manager), pos);
    }
    void setObjectToBlock(std::shared_ptr<PhysicsObject> &object,
                          BlockIterator bi,
                          WorldLockManager &lock_manager)
    {
        if(object == nullptr)
            object =
                PhysicsObject::makeEmptyNoAddToWorld(bi.position(), VectorF(0), shared_from_this());
        object->setToBlock(getBlockShape(bi, lock_manager), bi.position());
    }
    void setObjectToBlockEffectRegion(std::shared_ptr<PhysicsObject> &object,
                                      PositionI pos,
                                      WorldLockManager &lock_manager)
    {
        BlockIterator bi = getBlockIterator(pos, lock_manager.tls);
        setObjectToBlockEffectRegion(object, bi, lock_manager);
    }
    void setObjectToBlockEffectRegion(std::shared_ptr<PhysicsObject> &object,
                                      BlockIterator bi,
                                      WorldLockManager &lock_manager)
    {
        if(object == nullptr)
            object =
                PhysicsObject::makeEmptyNoAddToWorld(bi.position(), VectorF(0), shared_from_this());
        Block b = bi.get(lock_manager);
        BlockShape shape(nullptr);
        if(b.good())
            shape = b.descriptor->getEffectShape(bi, lock_manager);
        object->setToBlock(shape, bi.position());
    }

public:
    static constexpr float distanceEPS = 20 * eps;
    static constexpr float timeEPS = eps;
    inline double getCurrentTime() const
    {
        std::unique_lock<generic_lock_wrapper> lockIt(theLock);
        return currentTime;
    }
    inline int getOldVariableSetIndex() const
    {
        std::unique_lock<generic_lock_wrapper> lockIt(theLock);
        return variableSetIndex;
    }
    inline int getNewVariableSetIndex() const
    {
        std::unique_lock<generic_lock_wrapper> lockIt(theLock);
        return 1 - variableSetIndex;
    }

private:
    std::unordered_set<ordered_weak_ptr<PhysicsObject>> objects;
    void addObject(std::shared_ptr<PhysicsObject> o)
    {
        std::unique_lock<generic_lock_wrapper> lockIt(theLock);
        objects.insert(o);
    }
    void removeObject(std::shared_ptr<PhysicsObject> o)
    {
        std::unique_lock<generic_lock_wrapper> lockIt(theLock);
        objects.erase(o);
    }
    struct CollisionEvent final
    {
        ObjectCounter<PhysicsWorld, 0> objectCounter;
        double collisionTime;
        ordered_weak_ptr<PhysicsObject> a, b;
        std::uint64_t aTag, bTag;
        CollisionEvent(double collisionTime,
                       std::shared_ptr<PhysicsObject> a,
                       std::shared_ptr<PhysicsObject> b)
            : objectCounter(),
              collisionTime(collisionTime),
              a(a),
              b(b),
              aTag(a->latestUpdateTag),
              bTag(b->latestUpdateTag)
        {
        }
        bool operator==(const CollisionEvent &rt) const
        {
            if(collisionTime != rt.collisionTime)
                return false;
            if(aTag == rt.aTag && a.lock() == rt.a.lock() && bTag == rt.bTag
               && b.lock() == rt.b.lock())
                return true;
            if(aTag == rt.bTag && a.lock() == rt.b.lock() && bTag == rt.aTag
               && b.lock() == rt.a.lock())
                return true;
            return false;
        }
        bool operator!=(const CollisionEvent &rt) const
        {
            return !operator==(rt);
        }
    };
    struct CollisionEventHash final
    {
        std::size_t operator()(const CollisionEvent &ce) const
        {
            return std::hash<double>()(ce.collisionTime) + static_cast<std::size_t>(ce.aTag)
                   + static_cast<std::size_t>(ce.bTag);
        }
    };
    struct CollisionEventCompare final
    {
        bool operator()(const CollisionEvent &a, const CollisionEvent &b) const
        {
            return a.collisionTime < b.collisionTime;
        }
    };
    std::priority_queue<CollisionEvent, std::vector<CollisionEvent>, CollisionEventCompare>
        eventsQueue;
    std::unordered_set<CollisionEvent, CollisionEventHash> eventsSet;
    std::unordered_set<ordered_weak_ptr<PhysicsObject>> changedObjects;
    void swapVariableSetIndex()
    {
        variableSetIndex = (variableSetIndex != 0 ? 0 : 1);
    }

public:
    void runToTime(double stopTime, WorldLockManager &lock_manager);
    void stepTime(double deltaTime, WorldLockManager &lock_manager)
    {
        runToTime(deltaTime + getCurrentTime(), lock_manager);
    }
};

inline void PhysicsObject::transferToNewWorld(std::shared_ptr<PhysicsWorld> newWorld)
{
    std::shared_ptr<PhysicsWorld> world = getWorld();
    if(world == newWorld)
        return;
    std::unique_lock<generic_lock_wrapper> worldLock(world->theLock, std::defer_lock);
    std::unique_lock<generic_lock_wrapper> newWorldLock(newWorld->theLock, std::defer_lock);
    std::lock(worldLock, newWorldLock);
    double deltaTime = newWorld->getCurrentTime() - world->getCurrentTime();
    for(double &v : objectTime)
        v += deltaTime;
    world->removeObject(shared_from_this());
    newWorld->addObject(shared_from_this());
    this->world = newWorld;
}

#if 0
class PhysicsObjectConstructor : public std::enable_shared_from_this<PhysicsObjectConstructor>
{
public:
    virtual std::shared_ptr<PhysicsObject> make(PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> world) const = 0;
    virtual ~PhysicsObjectConstructor()
    {
    }
protected:
    enum class Shape : std::uint8_t
    {
        Empty,
        Box,
        Cylinder,
        DEFINE_ENUM_LIMITS(Empty, Cylinder)
    };
public:
    static std::shared_ptr<const PhysicsObjectConstructor> cylinderMaker(float radius, float yExtent, bool affectedByGravity, bool isStatic, PhysicsProperties properties, std::shared_ptr<PhysicsCollisionHandler> collisionHandler = nullptr, std::vector<PhysicsConstraint> constraints = {});
    static std::shared_ptr<const PhysicsObjectConstructor> boxMaker(VectorF extents, bool affectedByGravity, bool isStatic, PhysicsProperties properties, std::shared_ptr<PhysicsCollisionHandler> collisionHandler = nullptr, std::vector<PhysicsConstraint> constraints = {});
    static std::shared_ptr<const PhysicsObjectConstructor> empty(VectorF gravity = VectorF(0));
};

class PhysicsObjectConstructorCylinder final : public PhysicsObjectConstructor
{
private:
    float radius;
    float yExtent;
    bool affectedByGravity;
    bool isStatic;
    PhysicsProperties properties;
    std::vector<PhysicsConstraint> constraints;
public:
    PhysicsObjectConstructorCylinder(float radius, float yExtent, bool affectedByGravity, bool isStatic, PhysicsProperties properties, std::vector<PhysicsConstraint> constraints)
        : radius(radius), yExtent(yExtent), affectedByGravity(affectedByGravity), isStatic(isStatic), properties(properties), constraints(constraints)
    {
    }
    virtual std::shared_ptr<PhysicsObject> make(PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> world) const override
    {
        auto retval = PhysicsObject::makeCylinder(position, velocity, affectedByGravity, isStatic, radius, yExtent, properties, world);
        retval->setConstraints(constraints);
        return retval;
    }
};

inline std::shared_ptr<const PhysicsObjectConstructor> PhysicsObjectConstructor::cylinderMaker(float radius, float yExtent, bool affectedByGravity, bool isStatic, PhysicsProperties properties, std::shared_ptr<PhysicsCollisionHandler>, std::vector<PhysicsConstraint> constraints)
{
    return std::shared_ptr<const PhysicsObjectConstructor>(new PhysicsObjectConstructorCylinder(radius, yExtent, affectedByGravity, isStatic, properties, constraints));
}

class PhysicsObjectConstructorBox final : public PhysicsObjectConstructor
{
private:
    VectorF extents;
    bool affectedByGravity;
    bool isStatic;
    PhysicsProperties properties;
    std::vector<PhysicsConstraint> constraints;
public:
    PhysicsObjectConstructorBox(VectorF extents, bool affectedByGravity, bool isStatic, PhysicsProperties properties, std::vector<PhysicsConstraint> constraints)
        : extents(extents), affectedByGravity(affectedByGravity), isStatic(isStatic), properties(properties)
    {
    }
    virtual std::shared_ptr<PhysicsObject> make(PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> world) const override
    {
        auto retval = PhysicsObject::makeBox(position, velocity, affectedByGravity, isStatic, extents, properties, world);
        retval->setConstraints(constraints);
        return retval;
    }
};

inline std::shared_ptr<const PhysicsObjectConstructor> PhysicsObjectConstructor::boxMaker(VectorF extents, bool affectedByGravity, bool isStatic, PhysicsProperties properties, std::vector<PhysicsConstraint> constraints)
{
    return std::shared_ptr<const PhysicsObjectConstructor>(new PhysicsObjectConstructorBox(extents, affectedByGravity, isStatic, properties, constraints));
}

class PhysicsObjectConstructorEmpty final : public PhysicsObjectConstructor
{
private:
    VectorF gravity;
public:
    PhysicsObjectConstructorEmpty(VectorF gravity)
        : gravity(gravity)
    {
    }
    virtual std::shared_ptr<PhysicsObject> make(PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> world) const override
    {
        return PhysicsObject::makeEmpty(position, velocity, world, gravity);
    }
};

inline std::shared_ptr<const PhysicsObjectConstructor> PhysicsObjectConstructor::empty(VectorF gravity)
{
    return std::shared_ptr<const PhysicsObjectConstructor>(new PhysicsObjectConstructorEmpty(gravity));
}
#endif

inline PhysicsObject::PhysicsObject(PositionF position,
                                    VectorF velocity,
                                    bool affectedByGravity,
                                    bool isStatic,
                                    VectorF extents,
                                    std::shared_ptr<PhysicsWorld> world,
                                    PhysicsProperties properties,
                                    Type type,
                                    std::shared_ptr<PhysicsCollisionHandler> collisionHandler)
    : objectCounter(),
      position{position, position},
      velocity{velocity, velocity},
      objectTime{world->getCurrentTime(), world->getCurrentTime()},
      affectedByGravity(affectedByGravity),
      isStatic_(isStatic),
      type(type),
      extents(extents),
      world(world),
      properties(properties),
      blockEffects(),
      constraints(),
      collisionHandler(collisionHandler)
{
}

inline std::shared_ptr<PhysicsObject> PhysicsObject::makeBox(
    PositionF position,
    VectorF velocity,
    bool affectedByGravity,
    bool isStatic,
    VectorF extents,
    PhysicsProperties properties,
    std::shared_ptr<PhysicsWorld> world,
    std::shared_ptr<PhysicsCollisionHandler> collisionHandler)
{
    std::shared_ptr<PhysicsObject> retval =
        std::shared_ptr<PhysicsObject>(new PhysicsObject(position,
                                                         velocity,
                                                         affectedByGravity,
                                                         isStatic,
                                                         extents,
                                                         world,
                                                         properties,
                                                         Type::Box,
                                                         collisionHandler));
    world->objects.insert(retval);
    world->changedObjects.insert(retval);
    return retval;
}

inline std::shared_ptr<PhysicsObject> PhysicsObject::makeCylinder(
    PositionF position,
    VectorF velocity,
    bool affectedByGravity,
    bool isStatic,
    float radius,
    float yExtents,
    PhysicsProperties properties,
    std::shared_ptr<PhysicsWorld> world,
    std::shared_ptr<PhysicsCollisionHandler> collisionHandler)
{
    std::shared_ptr<PhysicsObject> retval =
        std::shared_ptr<PhysicsObject>(new PhysicsObject(position,
                                                         velocity,
                                                         affectedByGravity,
                                                         isStatic,
                                                         VectorF(radius, yExtents, radius),
                                                         world,
                                                         properties,
                                                         Type::Cylinder,
                                                         collisionHandler));
    world->objects.insert(retval);
    world->changedObjects.insert(retval);
    return retval;
}

inline std::shared_ptr<PhysicsObject> PhysicsObject::makeEmpty(PositionF position,
                                                               VectorF velocity,
                                                               std::shared_ptr<PhysicsWorld> world,
                                                               VectorF gravity)
{
    std::shared_ptr<PhysicsObject> retval = std::shared_ptr<PhysicsObject>(
        new PhysicsObject(position,
                          velocity,
                          gravity != VectorF(0),
                          true,
                          VectorF(),
                          world,
                          PhysicsProperties(PhysicsProperties::defaultCollisionMask,
                                            PhysicsProperties::defaultCollisionMask,
                                            std::sqrt(0.5f),
                                            1 - std::sqrt(0.5f),
                                            gravity),
                          Type::Empty,
                          nullptr));
    world->objects.insert(retval);
    return retval;
}

inline std::shared_ptr<PhysicsObject> PhysicsObject::makeEmptyNoAddToWorld(
    PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> world)
{
    return std::shared_ptr<PhysicsObject>(new PhysicsObject(position,
                                                            velocity,
                                                            false,
                                                            true,
                                                            VectorF(),
                                                            world,
                                                            PhysicsProperties(),
                                                            Type::Empty,
                                                            nullptr));
}

inline PhysicsObject::~PhysicsObject()
{
}

inline BlockEffects PhysicsObject::getBlockEffects() const
{
    auto world = getWorld();
    std::unique_lock<generic_lock_wrapper> lockIt(world->theLock);
    int variableSetIndex = world->getOldVariableSetIndex();
    return blockEffects[variableSetIndex];
}

inline PositionF PhysicsObject::getPosition() const
{
    auto world = getWorld();
    std::unique_lock<generic_lock_wrapper> lockIt(world->theLock);
    int variableSetIndex = world->getOldVariableSetIndex();
    float deltaTime = static_cast<float>(world->getCurrentTime() - objectTime[variableSetIndex]);
    VectorF gravityVector = getProperties().gravity;
    if(!affectedByGravity)
        gravityVector = VectorF(0);
    else if(isSupported() && gravityVector.y < 0.0f)
        gravityVector.y = 0.0f;
    float currentDrag = blockEffects[variableSetIndex].drag * getProperties().dragFactor;
    if(currentDrag <= 1e-5)
    {
        return position[variableSetIndex] + deltaTime * velocity[variableSetIndex]
               + 0.5f * deltaTime * deltaTime * gravityVector;
    }
    float temp1 = std::exp(currentDrag * deltaTime);
    return position[variableSetIndex]
           + (gravityVector + gravityVector * temp1 * (currentDrag * deltaTime - 1)
              + currentDrag * (temp1 - 1) * velocity[variableSetIndex])
                 * (1 / (currentDrag * currentDrag * temp1));
}

inline VectorF PhysicsObject::getVelocity() const
{
    auto world = getWorld();
    std::unique_lock<generic_lock_wrapper> lockIt(world->theLock);
    int variableSetIndex = world->getOldVariableSetIndex();
    VectorF gravityVector = getProperties().gravity;
    if(!affectedByGravity)
        gravityVector = VectorF(0);
    else if(isSupported() && gravityVector.y < 0.0f)
        gravityVector.y = 0.0f;
    float deltaTime = static_cast<float>(world->getCurrentTime() - objectTime[variableSetIndex]);
    float currentDrag = blockEffects[variableSetIndex].drag * getProperties().dragFactor;
    if(currentDrag <= 1e-5)
    {
        return velocity[variableSetIndex] + deltaTime * gravityVector;
    }
    float temp1 = std::exp(currentDrag * deltaTime);
    return (gravityVector * (temp1 - 1) + currentDrag * velocity[variableSetIndex])
           * (1 / (currentDrag * temp1));
}

inline void PhysicsObject::setNewState(PositionF newPosition, VectorF newVelocity)
{
    auto world = getWorld();
    std::unique_lock<generic_lock_wrapper> lockIt(world->theLock);
    int variableSetIndex = world->getNewVariableSetIndex();
    objectTime[variableSetIndex] = world->getCurrentTime();
    newPosition += position[variableSetIndex] * static_cast<float>(newStateCount);
    newVelocity += velocity[variableSetIndex] * static_cast<float>(newStateCount);
    newStateCount++;
    newPosition /= static_cast<float>(newStateCount);
    newVelocity /= static_cast<float>(newStateCount);
    position[variableSetIndex] = newPosition;
    velocity[variableSetIndex] = newVelocity;
    world->changedObjects.insert(shared_from_this());
    latestUpdateTag++;
}

inline void PhysicsObject::setupNewState()
{
    auto world = getWorld();
    std::unique_lock<generic_lock_wrapper> lockIt(world->theLock);
    int oldVariableSetIndex = world->getOldVariableSetIndex();
    int newVariableSetIndex = world->getNewVariableSetIndex();
    objectTime[newVariableSetIndex] = objectTime[oldVariableSetIndex];
    position[newVariableSetIndex] = position[oldVariableSetIndex];
    velocity[newVariableSetIndex] = velocity[oldVariableSetIndex];
    blockEffects[newVariableSetIndex] = blockEffects[oldVariableSetIndex];
    newStateCount = 0;
}

inline bool PhysicsObject::collides(const PhysicsObject &rt) const
{
    if(isEmpty() || rt.isEmpty())
        return false;
    if(!properties.canCollideWith(rt.properties))
        return false;
    auto world = getWorld();
    assert(world == rt.getWorld());
    PositionF lPosition = getPosition();
    PositionF rPosition = rt.getPosition();
    if(lPosition.d != rPosition.d)
        return false;
    VectorF lExtents = extents;
    VectorF rExtents = rt.extents;
    VectorF extentsSum = lExtents + rExtents;
    VectorF deltaPosition = static_cast<VectorF>(lPosition) - static_cast<VectorF>(rPosition);
    if(std::abs(deltaPosition.x) > PhysicsWorld::distanceEPS + extentsSum.x)
        return false;
    if(std::abs(deltaPosition.y) > PhysicsWorld::distanceEPS + extentsSum.y)
        return false;
    if(std::abs(deltaPosition.z) > PhysicsWorld::distanceEPS + extentsSum.z)
        return false;
    if(isBox() && rt.isBox())
        return true;
    if(isCylinder() && rt.isCylinder())
    {
        deltaPosition.y = 0;
        return abs(deltaPosition) <= extentsSum.x + PhysicsWorld::distanceEPS;
    }
    if((isBox() && rt.isCylinder()) || (rt.isBox() && isCylinder()))
    {
        VectorF cylinderCenter;
        float cylinderRadius;
        VectorF boxCenter;
        VectorF boxExtents;
        if(isBox())
        {
            cylinderCenter = static_cast<VectorF>(rPosition);
            cylinderRadius = rt.extents.x;
            boxCenter = static_cast<VectorF>(lPosition);
            boxExtents = extents;
        }
        else
        {
            cylinderCenter = static_cast<VectorF>(lPosition);
            cylinderRadius = extents.x;
            boxCenter = static_cast<VectorF>(rPosition);
            boxExtents = rt.extents;
        }
        cylinderCenter.y = 0;
        boxCenter.y = 0;
        VectorF deltaCenter = cylinderCenter - boxCenter;
        if(std::abs(deltaCenter.x) < boxExtents.x)
            return true;
        if(std::abs(deltaCenter.z) < boxExtents.z)
            return true;
        VectorF v = deltaCenter;
        v -= boxExtents * VectorF(static_cast<float>(sgn(deltaCenter.x)),
                                  0,
                                  static_cast<float>(sgn(deltaCenter.z)));
        return abs(v) <= cylinderRadius + PhysicsWorld::distanceEPS;
    }
    UNREACHABLE();
    return false;
}

inline void PhysicsObject::adjustPosition(const PhysicsObject &rt)
{
    if(isStatic())
        return;
    if(!properties.canCollideWith(rt.properties))
        return;
    PositionF aPosition = getPosition();
    PositionF bPosition = rt.getPosition();
    VectorF aVelocity = getVelocity();
    VectorF bVelocity = rt.getVelocity();
    VectorF deltaPosition = aPosition - bPosition;
    VectorF extentsSum = getExtents() + rt.getExtents();
    VectorF deltaVelocity = aVelocity - bVelocity;
    float interpolationT = 0.5f;
    if(rt.isStatic())
        interpolationT = 1.0f;
    float interpolationTY = interpolationT;
    if(rt.isSupported())
        interpolationTY = 1.0f;
    VectorF normal(0);
    if(deltaPosition.x == 0)
        deltaPosition.x = PhysicsWorld::distanceEPS;
    if(deltaPosition.y == 0)
        deltaPosition.y = PhysicsWorld::distanceEPS;
    if(deltaPosition.z == 0)
        deltaPosition.z = PhysicsWorld::distanceEPS;
    if(isBox() && rt.isBox())
    {
        VectorF AbsDeltaPosition = VectorF(
            std::abs(deltaPosition.x), std::abs(deltaPosition.y), std::abs(deltaPosition.z));
        VectorF surfaceOffset =
            extentsSum - AbsDeltaPosition + VectorF(PhysicsWorld::distanceEPS * 2);
        if(surfaceOffset.x < surfaceOffset.y && surfaceOffset.x < surfaceOffset.z)
        {
            normal.x = static_cast<float>(sgn(deltaPosition.x));
            aPosition.x += interpolationT * normal.x * surfaceOffset.x;
        }
        else if(surfaceOffset.y < surfaceOffset.z)
        {
            normal.y = static_cast<float>(sgn(deltaPosition.y));
            aPosition.y += interpolationTY * normal.y * surfaceOffset.y;
        }
        else
        {
            normal.z = static_cast<float>(sgn(deltaPosition.z));
            aPosition.z += interpolationT * normal.z * surfaceOffset.z;
        }
    }
    else if(isCylinder() && rt.isCylinder())
    {
        float absDeltaY = std::abs(deltaPosition.y);
        VectorF xzDeltaPosition = deltaPosition;
        xzDeltaPosition.y = 0;
        float deltaCylindricalR = abs(xzDeltaPosition);
        float ySurfaceOffset = extentsSum.y - absDeltaY + PhysicsWorld::distanceEPS * 2;
        float rSurfaceOffset = extentsSum.x - deltaCylindricalR + PhysicsWorld::distanceEPS * 2;
        if(ySurfaceOffset < rSurfaceOffset)
        {
            normal.y = static_cast<float>(sgn(deltaPosition.y));
            aPosition.y += interpolationTY * normal.y * ySurfaceOffset;
        }
        else
        {
            normal = xzDeltaPosition / deltaCylindricalR;
            aPosition += interpolationT * rSurfaceOffset * normal;
        }
    }
    else if(isCylinder() && rt.isBox())
    {
        float absDeltaY = std::abs(deltaPosition.y);
        float ySurfaceOffset = extentsSum.y - absDeltaY + PhysicsWorld::distanceEPS * 2;
        VectorF xzDeltaPosition = deltaPosition;
        xzDeltaPosition.y = 0;
        VectorF horizontalNormal;
        float horizontalSurfaceOffset;
        VectorF absXZDeltaPosition =
            VectorF(std::abs(deltaPosition.x), 0, std::abs(deltaPosition.z));
        VectorF xzSurfaceOffset =
            VectorF(extents.x, 0, extents.x) + rt.extents - absXZDeltaPosition
            + VectorF(PhysicsWorld::distanceEPS * 2, 0, PhysicsWorld::distanceEPS * 2);
        if(absXZDeltaPosition.x < rt.extents.x && absXZDeltaPosition.z < rt.extents.z)
        {
            if(xzSurfaceOffset.x < xzSurfaceOffset.z)
            {
                horizontalNormal = VectorF(static_cast<float>(sgn(deltaPosition.x)), 0, 0);
                horizontalSurfaceOffset = xzSurfaceOffset.x;
            }
            else
            {
                horizontalNormal = VectorF(0, 0, static_cast<float>(sgn(deltaPosition.z)));
                horizontalSurfaceOffset = xzSurfaceOffset.z;
            }
        }
        else if(absXZDeltaPosition.x < rt.extents.x + PhysicsWorld::distanceEPS)
        {
            horizontalNormal = VectorF(0, 0, static_cast<float>(sgn(deltaPosition.z)));
            horizontalSurfaceOffset = xzSurfaceOffset.z;
        }
        else if(absXZDeltaPosition.z < rt.extents.z + PhysicsWorld::distanceEPS)
        {
            horizontalNormal = VectorF(static_cast<float>(sgn(deltaPosition.x)), 0, 0);
            horizontalSurfaceOffset = xzSurfaceOffset.x;
        }
        else
        {
            VectorF closestPoint = VectorF(limit(deltaPosition.x, -rt.extents.x, rt.extents.x),
                                           0,
                                           limit(deltaPosition.z, -rt.extents.z, rt.extents.z));
            VectorF v = xzDeltaPosition - closestPoint;
            float r = abs(v);
            horizontalSurfaceOffset = extents.x - r + PhysicsWorld::distanceEPS * 2;
            horizontalNormal = normalize(v);
        }
        if(ySurfaceOffset < horizontalSurfaceOffset)
        {
            normal.y = static_cast<float>(sgn(deltaPosition.y));
            aPosition.y += interpolationTY * normal.y * ySurfaceOffset;
        }
        else
        {
            normal = horizontalNormal;
            aPosition += interpolationT * horizontalSurfaceOffset * normal;
        }
    }
    else if(isBox() && rt.isCylinder())
    {
        float absDeltaY = std::abs(deltaPosition.y);
        float ySurfaceOffset = extentsSum.y - absDeltaY + PhysicsWorld::distanceEPS * 2;
        VectorF xzDeltaPosition = deltaPosition;
        xzDeltaPosition.y = 0;
        VectorF horizontalNormal;
        float horizontalSurfaceOffset;
        VectorF absXZDeltaPosition =
            VectorF(std::abs(deltaPosition.x), 0, std::abs(deltaPosition.z));
        VectorF xzSurfaceOffset =
            VectorF(rt.extents.x, 0, rt.extents.x) + extents - absXZDeltaPosition
            + VectorF(PhysicsWorld::distanceEPS * 2, 0, PhysicsWorld::distanceEPS * 2);
        if(absXZDeltaPosition.x < extents.x && absXZDeltaPosition.z < extents.z)
        {
            if(xzSurfaceOffset.x < xzSurfaceOffset.z)
            {
                horizontalNormal = VectorF(static_cast<float>(sgn(deltaPosition.x)), 0, 0);
                horizontalSurfaceOffset = xzSurfaceOffset.x;
            }
            else
            {
                horizontalNormal = VectorF(0, 0, static_cast<float>(sgn(deltaPosition.z)));
                horizontalSurfaceOffset = xzSurfaceOffset.z;
            }
        }
        else if(absXZDeltaPosition.x < extents.x + PhysicsWorld::distanceEPS)
        {
            horizontalNormal = VectorF(0, 0, static_cast<float>(sgn(deltaPosition.z)));
            horizontalSurfaceOffset = xzSurfaceOffset.z;
        }
        else if(absXZDeltaPosition.z < extents.z + PhysicsWorld::distanceEPS)
        {
            horizontalNormal = VectorF(static_cast<float>(sgn(deltaPosition.x)), 0, 0);
            horizontalSurfaceOffset = xzSurfaceOffset.x;
        }
        else
        {
            VectorF closestPoint = VectorF(limit(deltaPosition.x, -extents.x, extents.x),
                                           0,
                                           limit(deltaPosition.z, -extents.z, extents.z));
            VectorF v = xzDeltaPosition - closestPoint;
            float r = abs(v);
            horizontalSurfaceOffset = rt.extents.x - r + PhysicsWorld::distanceEPS * 2;
            horizontalNormal = normalize(v);
        }
        if(ySurfaceOffset < horizontalSurfaceOffset)
        {
            normal.y = static_cast<float>(sgn(deltaPosition.y));
            aPosition.y += interpolationTY * normal.y * ySurfaceOffset;
        }
        else
        {
            normal = horizontalNormal;
            aPosition += interpolationT * horizontalSurfaceOffset * normal;
        }
    }
    else
        UNREACHABLE();
    if(dot(deltaVelocity, normal) < 0)
        aVelocity -=
            ((1 + properties.bounceFactor * rt.properties.bounceFactor) * dot(deltaVelocity, normal)
                 * normal
             + (1 - properties.slideFactor) * (1 - rt.properties.slideFactor)
                   * (deltaVelocity - normal * dot(deltaVelocity, normal))) * interpolationT;
    else
        aVelocity = interpolate(0.5f, aVelocity, bVelocity);
    setNewState(aPosition, aVelocity);
}

inline bool PhysicsObject::isSupportedBy(const PhysicsObject &rt) const
{
    if(isStatic())
        return false;
    if(!rt.isSupported() && !rt.isStatic())
        return false;
    if(!properties.canCollideWith(rt.properties))
        return false;
    if(isEmpty() || rt.isEmpty())
        return false;
    PositionF aPosition = getPosition();
    PositionF bPosition = rt.getPosition();
    if(aPosition.d != bPosition.d)
        return false;
    VectorF extentsSum = extents + rt.extents;
    VectorF deltaPosition = aPosition - bPosition;
    if(deltaPosition.x + PhysicsWorld::distanceEPS > -extentsSum.x
       && deltaPosition.x - PhysicsWorld::distanceEPS < extentsSum.x
       && deltaPosition.z + PhysicsWorld::distanceEPS > -extentsSum.z
       && deltaPosition.z - PhysicsWorld::distanceEPS < extentsSum.z)
    {
        if(deltaPosition.y > 0)
        {
            if(deltaPosition.y < PhysicsWorld::distanceEPS * 4 + extentsSum.y)
            {
                if(isBox() && rt.isBox())
                    return true;
                if(isCylinder() && rt.isCylinder())
                {
                    deltaPosition.y = 0;
                    return abs(deltaPosition) <= extentsSum.x + PhysicsWorld::distanceEPS;
                }
                if((isBox() && rt.isCylinder()) || (rt.isBox() && isCylinder()))
                {
                    VectorF cylinderCenter;
                    float cylinderRadius;
                    VectorF boxCenter;
                    VectorF boxExtents;
                    if(isBox())
                    {
                        cylinderCenter = static_cast<VectorF>(bPosition);
                        cylinderRadius = rt.extents.x;
                        boxCenter = static_cast<VectorF>(aPosition);
                        boxExtents = extents;
                    }
                    else
                    {
                        cylinderCenter = static_cast<VectorF>(aPosition);
                        cylinderRadius = extents.x;
                        boxCenter = static_cast<VectorF>(bPosition);
                        boxExtents = rt.extents;
                    }
                    cylinderCenter.y = 0;
                    boxCenter.y = 0;
                    VectorF deltaCenter = cylinderCenter - boxCenter;
                    if(std::abs(deltaCenter.x) < boxExtents.x)
                        return true;
                    if(std::abs(deltaCenter.z) < boxExtents.z)
                        return true;
                    VectorF v = deltaCenter;
                    v -= boxExtents * VectorF(static_cast<float>(sgn(deltaCenter.x)),
                                              0,
                                              static_cast<float>(sgn(deltaCenter.z)));
                    return abs(v) <= cylinderRadius + PhysicsWorld::distanceEPS;
                }
                UNREACHABLE();
            }
        }
    }
    return false;
}

inline void PhysicsObject::setCurrentState(PositionF newPosition, VectorF newVelocity)
{
    std::shared_ptr<PhysicsWorld> pworld = world.lock();
    std::unique_lock<generic_lock_wrapper> lockIt(pworld->theLock);
    auto i = pworld->getOldVariableSetIndex();
    position[i] = newPosition;
    velocity[i] = newVelocity;
    objectTime[i] = pworld->getCurrentTime();
}

inline void PhysicsObject::setCurrentState(PositionF newPosition,
                                           VectorF newVelocity,
                                           VectorF gravity)
{
    std::shared_ptr<PhysicsWorld> pworld = world.lock();
    std::unique_lock<generic_lock_wrapper> lockIt(pworld->theLock);
    auto i = pworld->getOldVariableSetIndex();
    position[i] = newPosition;
    velocity[i] = newVelocity;
    objectTime[i] = pworld->getCurrentTime();
    properties.gravity = gravity;
}

inline void PhysicsObject::setGravity(VectorF gravity)
{
    std::shared_ptr<PhysicsWorld> pworld = world.lock();
    std::unique_lock<generic_lock_wrapper> lockIt(pworld->theLock);
    properties.gravity = gravity;
}

inline PhysicsProperties PhysicsObject::getProperties() const
{
    std::shared_ptr<PhysicsWorld> pworld = world.lock();
    std::unique_lock<generic_lock_wrapper> lockIt(pworld->theLock);
    return properties;
}
}
}

#endif // PHYSICS_OBJECT_H_INCLUDED
