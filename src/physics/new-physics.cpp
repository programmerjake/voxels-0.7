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
#include "util/lock.h"
#include <algorithm>

namespace programmerjake
{
namespace voxels
{
namespace physics
{
BoundingBox World::ObjectImp::getBoundingBox() const
{
    switch(shape.getTag())
    {
    case ShapeTag::None:
        return BoundingBox(position, position);
    case ShapeTag::Box:
    {
        BoxShape boxShape = shape.getBox();
        return BoundingBox(position - 0.5f * boxShape.size, position + 0.5f * boxShape.size);
    }
    case ShapeTag::Cylinder:
    {
        CylinderShape cylinderShape = shape.getCylinder();
        VectorF size =
            VectorF(cylinderShape.diameter, cylinderShape.height, cylinderShape.diameter);
        return BoundingBox(position - 0.5f * size, position + 0.5f * size);
    }
    }
    UNREACHABLE();
}

void World::stepTime(double deltaTime, WorldLockManager &lock_manager)
{
    lock_manager.clear();
    auto lockedStepTimeLock = make_unique_lock(stepTimeLock);
    readPublicData();
    integratePositions(deltaTime, lock_manager);
    broadPhaseCollisionDetection(lock_manager);
    narrowPhaseCollisionDetection(lock_manager);
    setupConstraints(lock_manager);
    solveConstraints(lock_manager);
    updatePublicData(deltaTime, lock_manager);
}

void World::addToWorld(ObjectImp object)
{
    auto lockIt = make_unique_lock(publicState.lock);
    publicState.newObjectsQueue.push_back(std::move(object));
}

void World::readPublicData()
{
    lastObjectsArray.swap(objects);
    objects.clear();
    objects.reserve(lastObjectsArray.size());
    Object::AllMutexesLock publicObjectsLock;
    std::unique_lock<Object::AllMutexesLock> lockedPublicObjects(publicObjectsLock);
    for(ObjectImp &object : lastObjectsArray)
    {
        std::shared_ptr<Object> publicObject = object.object.lock();
        if(!publicObject)
            continue;
        object.properties = publicObject->properties;
        object.motor = publicObject->motor;
        object.motorVelocity = publicObject->motorVelocity;
        objects.push_back(std::move(object));
    }
    lockedPublicObjects.unlock();
    lastObjectsArray.clear();
}

void World::broadPhaseCollisionDetection(WorldLockManager &lock_manager)
{
    sortedObjectsForBroadPhase.clear();
    sortedObjectsForBroadPhase.reserve(objects.size());
    BoundingBox totalBoundingBox(VectorF(0), VectorF(0));
    for(std::size_t i = 0; i < objects.size(); i++)
    {
        auto myCollisionMask = objects[i].properties.myCollisionMask;
        auto othersCollisionMask = objects[i].properties.othersCollisionMask;
        if(objects[i].shape.empty() || (myCollisionMask == 0 && othersCollisionMask == 0))
            continue;
        BoundingBox boundingBox = objects[i].getBoundingBox();
        if(i == 0)
            totalBoundingBox = boundingBox;
        else
            totalBoundingBox |= boundingBox;
        sortedObjectsForBroadPhase.push_back(SortingObjectForBroadPhase(objects[i].getBoundingBox(),
                                                                        i,
                                                                        myCollisionMask,
                                                                        othersCollisionMask,
                                                                        objects[i].position.d));
    }
    collidingPairsFromBroadPhase.clear();
    collidingPairsFromBroadPhase.reserve(sortedObjectsForBroadPhase.size() * 12); // likely maximum
    VectorF totalExtent = totalBoundingBox.extent();
    if(totalExtent.x > totalExtent.y && totalExtent.x > totalExtent.z)
    {
        for(SortingObjectForBroadPhase &object : sortedObjectsForBroadPhase)
        {
            object.sortingIntervalStart = object.boundingBox.minCorner.x;
            object.sortingIntervalEnd = object.boundingBox.maxCorner.x;
        }
    }
    else if(totalExtent.y > totalExtent.z)
    {
        for(SortingObjectForBroadPhase &object : sortedObjectsForBroadPhase)
        {
            object.sortingIntervalStart = object.boundingBox.minCorner.y;
            object.sortingIntervalEnd = object.boundingBox.maxCorner.y;
        }
    }
    else
    {
        for(SortingObjectForBroadPhase &object : sortedObjectsForBroadPhase)
        {
            object.sortingIntervalStart = object.boundingBox.minCorner.z;
            object.sortingIntervalEnd = object.boundingBox.maxCorner.z;
        }
    }
    std::stable_sort(sortedObjectsForBroadPhase.begin(),
                     sortedObjectsForBroadPhase.end(),
                     [](const SortingObjectForBroadPhase &a, const SortingObjectForBroadPhase &b)
                     {
                         return a.sortingIntervalStart < b.sortingIntervalStart;
                     });
    for(std::size_t i = 0; i < sortedObjectsForBroadPhase.size(); i++)
    {
        SortingObjectForBroadPhase objectI = sortedObjectsForBroadPhase[i];
        for(std::size_t j = i + 1; j < sortedObjectsForBroadPhase.size(); j++)
        {
            SortingObjectForBroadPhase objectJ = sortedObjectsForBroadPhase[j];
            if(objectJ.sortingIntervalStart > objectI.sortingIntervalEnd)
                break;
            if(objectI.dimension != objectJ.dimension)
                continue;
            if((objectI.myCollisionMask & objectJ.othersCollisionMask) == 0
               && (objectI.othersCollisionMask & objectJ.myCollisionMask) == 0)
                continue;
            if(objectI.boundingBox.overlaps(objectJ.boundingBox))
                collidingPairsFromBroadPhase.emplace_back(objectI.objectIndex, objectJ.objectIndex);
        }
    }
    sortedObjectsForBroadPhase.clear();
}

void World::narrowPhaseCollisionDetection(WorldLockManager &lock_manager)
{
    collisions.clear();
    collisions.reserve(2 * collidingPairsFromBroadPhase.size());
    for(std::pair<std::size_t, std::size_t> collidingPair : collidingPairsFromBroadPhase)
    {
        std::size_t objectAIndex = std::get<0>(collidingPair);
        std::size_t objectBIndex = std::get<1>(collidingPair);
        const ObjectImp &objectA = objects[objectAIndex];
        const ObjectImp &objectB = objects[objectBIndex];
        if((objectA.properties.myCollisionMask & objectB.properties.othersCollisionMask) == 0 && (objectA.properties.othersCollisionMask & objectB.properties.myCollisionMask) == 0)
            continue;
        Collision collision = collide(objectA, objectB, objectAIndex, objectBIndex);
        if(collision.empty())
            continue;
        if((objectA.properties.myCollisionMask & objectB.properties.othersCollisionMask) != 0)
            collisions.push_back(collision);
        if((objectB.properties.myCollisionMask & objectA.properties.othersCollisionMask) != 0)
            collisions.push_back(collision.reversed());
    }
    collidingPairsFromBroadPhase.clear();
}

void World::setupConstraints(WorldLockManager &lock_manager)
{
    // TODO: finish
    assert(false);
    throw std::runtime_error("World::setupConstraints is not implemented");
}

void World::solveConstraints(WorldLockManager &lock_manager)
{
    // TODO: finish
    assert(false);
    throw std::runtime_error("World::solveConstraints is not implemented");
}

void World::integratePositions(double deltaTime, WorldLockManager &lock_manager)
{
    // TODO: finish
    assert(false);
    throw std::runtime_error("World::integratePositions is not implemented");
}

void World::updatePublicData(double deltaTime, WorldLockManager &lock_manager)
{
    // TODO: finish
    assert(false);
    throw std::runtime_error("World::updatePublicData is not implemented");
}

Collision World::collideBoxBox(const ObjectImp &me,
                               const ObjectImp &other,
                               std::size_t myObjectIndex,
                               std::size_t otherObjectIndex)
{
    BoxShape myShape = me.shape.getBox();
    BoxShape otherShape = other.shape.getBox();
    if(me.position.d != other.position.d)
        return Collision();
    VectorF myMinCorner = me.position - myShape.size * 0.5f;
    VectorF myMaxCorner = me.position + myShape.size * 0.5f;
    VectorF otherMinCorner = other.position - otherShape.size * 0.5f;
    VectorF otherMaxCorner = other.position + otherShape.size * 0.5f;
    VectorI axisSign = VectorI(1);
    VectorF axisPenetration = otherMaxCorner - myMinCorner;
    VectorF myCollisionCorner = myMaxCorner;
    if(me.position.x < other.position.x)
    {
        axisSign.x = -1;
        axisPenetration.x = myMaxCorner.x - otherMinCorner.x;
        myCollisionCorner.x = myMinCorner.x;
    }
    if(me.position.y < other.position.y)
    {
        axisSign.y = -1;
        axisPenetration.y = myMaxCorner.y - otherMinCorner.y;
        myCollisionCorner.y = myMinCorner.y;
    }
    if(me.position.z < other.position.z)
    {
        axisSign.z = -1;
        axisPenetration.z = myMaxCorner.z - otherMinCorner.z;
        myCollisionCorner.z = myMinCorner.z;
    }
    if(axisPenetration.x > 0 && axisPenetration.y > 0 && axisPenetration.z > 0)
    {
        ContactPoint contactPoint;
        VectorF collisionPoint = myCollisionCorner - axisSign * axisPenetration * 0.5f;
        if(axisPenetration.x < axisPenetration.y && axisPenetration.x < axisPenetration.z)
        {
            contactPoint =
                ContactPoint(collisionPoint, VectorF(-axisSign.x, 0, 0), axisPenetration.x);
        }
        else if(axisPenetration.y < axisPenetration.z)
        {
            contactPoint =
                ContactPoint(collisionPoint, VectorF(0, -axisSign.y, 0), axisPenetration.y);
        }
        else
        {
            contactPoint =
                ContactPoint(collisionPoint, VectorF(0, 0, -axisSign.z), axisPenetration.z);
        }
        return Collision(contactPoint, myObjectIndex, otherObjectIndex);
    }
    return Collision();
}

Collision World::collideBoxCylinder(const ObjectImp &me,
                                    const ObjectImp &other,
                                    std::size_t myObjectIndex,
                                    std::size_t otherObjectIndex)
{
    BoxShape myShape = me.shape.getBox();
    CylinderShape otherShape = other.shape.getCylinder();
    if(me.position.d != other.position.d)
        return Collision();
    VectorF myMinCorner = me.position - myShape.size * 0.5f;
    VectorF myMaxCorner = me.position + myShape.size * 0.5f;
    float otherBottom = other.position.y - 0.5f * otherShape.height;
    float otherTop = other.position.y + 0.5f * otherShape.height;
    float otherRadius = otherShape.diameter * 0.5f;
    int yAxisSign = 1;
    float yPenetration = otherTop - myMinCorner.y;
    VectorF collisionPoint = other.position;
    collisionPoint.y = otherTop;
    if(me.position.y < other.position.y)
    {
        yPenetration = myMaxCorner.y - otherBottom;
        yAxisSign = -1;
        collisionPoint.y = otherBottom;
    }
    if(yPenetration <= 0)
        return Collision();
    VectorF displacementXZ(0);
    if(other.position.x < myMinCorner.x)
    {
        displacementXZ.x = other.position.x - myMinCorner.x;
        collisionPoint.x = myMinCorner.x;
    }
    else if(other.position.x > myMaxCorner.x)
    {
        displacementXZ.x = other.position.x - myMaxCorner.x;
        collisionPoint.x = myMaxCorner.x;
    }
    if(other.position.z < myMinCorner.z)
    {
        displacementXZ.z = other.position.z - myMinCorner.z;
        collisionPoint.z = myMinCorner.z;
    }
    else if(other.position.z > myMaxCorner.z)
    {
        displacementXZ.z = other.position.z - myMaxCorner.z;
        collisionPoint.z = myMaxCorner.z;
    }
    float radialPenetration = otherRadius - abs(displacementXZ);
    if(radialPenetration <= 0)
        return Collision();
    if(yPenetration > radialPenetration)
    {
        VectorF normal = displacementXZ;
        if(normal == VectorF(0))
        {
            VectorI axisSign(1);
            VectorF axisPenetration(other.position - myMinCorner);
            VectorF myCollisionCorner = myMaxCorner;
            if(me.position.x < other.position.x)
            {
                axisSign.x = -1;
                axisPenetration.x = myMaxCorner.x - other.position.x;
                myCollisionCorner.x = myMinCorner.x;
            }
            if(me.position.z < other.position.z)
            {
                axisSign.z = -1;
                axisPenetration.z = myMaxCorner.z - other.position.z;
                myCollisionCorner.z = myMinCorner.z;
            }
            ContactPoint contactPoint;
            VectorF collisionPoint = myCollisionCorner - axisSign * axisPenetration * 0.5f;
            collisionPoint.y = me.position.y;
            if(axisPenetration.x < axisPenetration.z)
            {
                contactPoint = ContactPoint(
                    collisionPoint, VectorF(-axisSign.x, 0, 0), axisPenetration.x + otherRadius);
            }
            else
            {
                contactPoint = ContactPoint(
                    collisionPoint, VectorF(0, 0, -axisSign.z), axisPenetration.z + otherRadius);
            }
            return Collision(contactPoint, myObjectIndex, otherObjectIndex);
        }
        normal = normalize(normal);
        ContactPoint contactPoint(collisionPoint, normal, radialPenetration);
        return Collision(contactPoint, myObjectIndex, otherObjectIndex);
    }
    ContactPoint contactPoint(collisionPoint, VectorF(0, -yAxisSign, 0), yPenetration);
    return Collision(contactPoint, myObjectIndex, otherObjectIndex);
}

Collision World::collideCylinderBox(const ObjectImp &me,
                                    const ObjectImp &other,
                                    std::size_t myObjectIndex,
                                    std::size_t otherObjectIndex)
{
    return collideBoxCylinder(other, me, myObjectIndex, otherObjectIndex).reversed();
}

Collision World::collideCylinderCylinder(const ObjectImp &me,
                                         const ObjectImp &other,
                                         std::size_t myObjectIndex,
                                         std::size_t otherObjectIndex)
{
    CylinderShape myShape = me.shape.getCylinder();
    CylinderShape otherShape = other.shape.getCylinder();
    if(me.position.d != other.position.d)
        return Collision();
    float myBottom = me.position.y - 0.5f * myShape.height;
    float myTop = me.position.y + 0.5f * myShape.height;
    float myRadius = myShape.diameter * 0.5f;
    float otherBottom = other.position.y - 0.5f * otherShape.height;
    float otherTop = other.position.y + 0.5f * otherShape.height;
    float otherRadius = otherShape.diameter * 0.5f;
    int yAxisSign = 1;
    float yPenetration = otherTop - myBottom;
    VectorF collisionPoint = other.position;
    collisionPoint.y = otherTop;
    if(me.position.y < other.position.y)
    {
        yPenetration = myTop - otherBottom;
        yAxisSign = -1;
        collisionPoint.y = otherBottom;
    }
    if(yPenetration <= 0)
        return Collision();
    VectorF displacementXZ = other.position - me.position;
    displacementXZ.y = 0;
    float radialPenetration = myRadius + otherRadius - abs(displacementXZ);
    if(radialPenetration <= 0)
        return Collision();
    if(yPenetration > radialPenetration)
    {
        VectorF normal = displacementXZ;
        if(normal != VectorF(0))
        {
            normal = normalize(normal);
            ContactPoint contactPoint(collisionPoint, normal, radialPenetration);
            return Collision(contactPoint, myObjectIndex, otherObjectIndex);
        }
    }
    ContactPoint contactPoint(collisionPoint, VectorF(0, -yAxisSign, 0), yPenetration);
    return Collision(contactPoint, myObjectIndex, otherObjectIndex);
}

Collision World::collide(const ObjectImp &me,
                         const ObjectImp &other,
                         std::size_t myObjectIndex,
                         std::size_t otherObjectIndex)
{
    switch(me.shape.getTag())
    {
    case ShapeTag::None:
        return Collision();
    case ShapeTag::Box:
    {
        switch(other.shape.getTag())
        {
        case ShapeTag::None:
            return Collision();
        case ShapeTag::Box:
            return collideBoxBox(me, other, myObjectIndex, otherObjectIndex);
        case ShapeTag::Cylinder:
            return collideBoxCylinder(me, other, myObjectIndex, otherObjectIndex);
        }
        break;
    }
    case ShapeTag::Cylinder:
    {
        switch(other.shape.getTag())
        {
        case ShapeTag::None:
            return Collision();
        case ShapeTag::Box:
            return collideCylinderBox(me, other, myObjectIndex, otherObjectIndex);
        case ShapeTag::Cylinder:
            return collideCylinderCylinder(me, other, myObjectIndex, otherObjectIndex);
        }
        break;
    }
    }
    UNREACHABLE();
}
}
}
}
