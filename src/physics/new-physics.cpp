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
void World::stepTime(double deltaTime, WorldLockManager &lock_manager)
{
    lock_manager.clear();
    auto lockedStepTimeLock = make_unique_lock(stepTimeLock);
    readPublicData();
    broadPhaseCollisionDetection(lock_manager);
    narrowPhaseCollisionDetection(lock_manager);
    setupConstraints(lock_manager);
    solveConstraints(lock_manager);
    integratePositions(deltaTime, lock_manager);
    updatePublicData(deltaTime, lock_manager);
}

void World::addToWorld(ObjectImp object)
{
    auto lockIt = make_unique_lock(publicState.lock);
    publicState.newObjectsQueue.push_back(std::move(object));
}

void World::readPublicData()
{

    for(auto iter = )
    // TODO: finish
    assert(false);
    throw std::runtime_error("World::readPublicData is not implemented");
}

void World::broadPhaseCollisionDetection(WorldLockManager &lock_manager)
{
    // TODO: finish
    assert(false);
    throw std::runtime_error("World::broadPhaseCollisionDetection is not implemented");
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
