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
#include <algorithm>

namespace programmerjake
{
namespace voxels
{
namespace physics
{
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
        sortedObjectsForBroadPhase.push_back(SortingObjectForBroadPhase(objects[i].getBoundingBox(), i));
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
            if((objectI.myCollisionMask & objectJ.othersCollisionMask) == 0 && (objectI.othersCollisionMask & objectJ.myCollisionMask) == 0)
                continue;
            if(objectI.boundingBox.overlaps(objectJ.boundingBox))
                collidingPairsFromBroadPhase.emplace_back(objectI.objectIndex, objectJ.objectIndex);
        }
    }
    sortedObjectsForBroadPhase.clear();
}

void World::narrowPhaseCollisionDetection(WorldLockManager &lock_manager)
{
    // TODO: finish
    assert(false);
    throw std::runtime_error("World::narrowPhaseCollisionDetection is not implemented");
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
}
}
}
