/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "physics/physics.h"

namespace programmerjake
{
namespace voxels
{
void PhysicsWorld::runToTime(double stopTime, WorldLockManager &lock_manager)
{
    //getDebugLog() << L"objects.size(): " << objects.size() << L" Run Duration: " << (stopTime - currentTime) << postnl;
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    float stepDuration = 1 / 60.0f;
    std::size_t stepCount = (std::size_t)std::ceil((stopTime - currentTime) / stepDuration - 0.1f);
    constexpr float searchEps = 0.1f;
    for(std::size_t step = 1; step <= stepCount; step++)
    {
        if(step >= stepCount)
            currentTime = stopTime;
        else
            currentTime += stepDuration;
        bool anyCollisions = true;
        for(std::size_t i = 0; i < 10 && anyCollisions; i++)
        {
            anyCollisions = false;
            std::vector<std::shared_ptr<PhysicsObject>> objectsVector;
            objectsVector.reserve(objects.size());
            for(auto i = objects.begin(); i != objects.end();)
            {
                std::shared_ptr<PhysicsObject> v = i->lock();
                if(v == nullptr)
                    i = objects.erase(i);
                else
                {
                    objectsVector.push_back(v);
                    ++i;
                }
            }
            std::vector<std::pair<float, std::shared_ptr<PhysicsObject>>> temporaryObjectsVector;
            temporaryObjectsVector.resize(objectsVector.size());
            for(std::size_t i = 0; i < temporaryObjectsVector.size(); i++)
                temporaryObjectsVector[i] = std::make_pair(objectsVector[i]->getPosition().y - objectsVector[i]->getExtents().y, objectsVector[i]);
            std::sort(temporaryObjectsVector.begin(), temporaryObjectsVector.end(), [](std::pair<float, std::shared_ptr<PhysicsObject>> a, std::pair<float, std::shared_ptr<PhysicsObject>> b)
            {
                return std::get<0>(a) < std::get<0>(b);
            });
            for(std::size_t i = 0; i < temporaryObjectsVector.size(); i++)
                objectsVector[i] = std::get<1>(temporaryObjectsVector[i]);
            for(auto i = objectsVector.begin(); i != objectsVector.end(); i++)
            {
                std::shared_ptr<PhysicsObject> objectA = *i;
                if(!objectA || objectA->isDestroyed())
                    continue;
                PositionF position = objectA->getPosition();
                VectorF velocity = objectA->getVelocity();
                objectA->position[getOldVariableSetIndex()] = position;
                objectA->velocity[getOldVariableSetIndex()] = velocity;
                objectA->objectTime[getOldVariableSetIndex()] = currentTime;
                objectA->supported = false;
                if(objectA->isEmpty())
                {
                    continue;
                }
                if(objectA->isStatic())
                {
                    objectA->supported = true;
                    continue;
                }
                VectorF fMin = position - objectA->extents - VectorF(searchEps);
                VectorF fMax = position + objectA->extents + VectorF(searchEps);
                int minX = ifloor(fMin.x);
                int maxX = iceil(fMax.x);
                int minY = ifloor(fMin.y);
                int maxY = iceil(fMax.y);
                int minZ = ifloor(fMin.z);
                int maxZ = iceil(fMax.z);
                std::shared_ptr<PhysicsObject> objectB;
                BlockIterator bix = getBlockIterator(PositionI(minX, minY, minZ, position.d));
                for(int xPosition = minX; xPosition <= maxX; xPosition++, bix.moveTowardPX())
                {
                    BlockIterator bixy = bix;
                    for(int yPosition = minY; yPosition <= maxY; yPosition++, bixy.moveTowardPY())
                    {
                        BlockIterator bi = bixy;
                        for(int zPosition = minZ; zPosition <= maxZ; zPosition++, bi.moveTowardPZ())
                        {
                            setObjectToBlock(objectB, bi, lock_manager);
                            bool supported = objectA->isSupportedBy(*objectB);
                            if(supported)
                            {
                                objectA->supported = true;
                            }
                        }
                    }
                }
                for(auto j = objectsVector.begin(); j != i; j++)
                {
                    std::shared_ptr<PhysicsObject> objectB = *j;
                    if(!objectB || objectB->isDestroyed())
                        continue;
                    bool supported = objectA->isSupportedBy(*objectB);
                    if(supported)
                    {
                        objectA->supported = true;
                    }
                }
            }
            constexpr std::size_t xScaleFactor = 5, zScaleFactor = 5;
            constexpr std::size_t bigHashPrime = 14713, smallHashPrime = 91;
            struct HashNode final
            {
                ObjectCounter<PhysicsWorld, 1> objectCounter;
                HashNode * hashNext;
                int x, z;
                std::shared_ptr<PhysicsObject> object;
            };
            checked_array<HashNode *, bigHashPrime> overallHashTable;
            overallHashTable.fill(nullptr);
            static thread_local HashNode * freeListHead = nullptr;
            std::vector<std::shared_ptr<PhysicsObject>> collideObjectsList;
            collideObjectsList.reserve(objects.size());
            for(auto i = objects.begin(); i != objects.end();)
            {
                std::shared_ptr<PhysicsObject> o = i->lock();
                if(!o || o->isDestroyed())
                {
                    i = objects.erase(i);
                    continue;
                }
                o->setupNewState();
                if(o->isEmpty())
                {
                    i++;
                    continue;
                }
                PositionF position = o->getPosition();
                VectorF extents = o->getExtents();
                float fMinX = position.x - extents.x;
                float fMaxX = position.x + extents.x;
                int minX = ifloor(fMinX * xScaleFactor);
                int maxX = iceil(fMaxX * xScaleFactor);
                float fMinZ = position.z - extents.z;
                float fMaxZ = position.z + extents.z;
                int minZ = ifloor(fMinZ * zScaleFactor);
                int maxZ = iceil(fMaxZ * zScaleFactor);
                if((std::size_t)(maxZ - minZ) * (std::size_t)(maxX * minX) > (std::size_t)(xScaleFactor + 1) * (std::size_t)(zScaleFactor + 1))
                {
                    collideObjectsList.push_back(o);
                }
                else
                {
                    for(int xPosition = minX; xPosition <= maxX; xPosition++)
                    {
                        for(int zPosition = minZ; zPosition <= maxZ; zPosition++)
                        {
                            HashNode * node = freeListHead;
                            if(node != nullptr)
                                freeListHead = freeListHead->hashNext;
                            else
                                node = new HashNode;
                            std::size_t hash = (std::size_t)(xPosition * 8191 + zPosition) % bigHashPrime;
                            node->hashNext = overallHashTable.at(hash);
                            node->x = xPosition;
                            node->z = zPosition;
                            node->object = o;
                            overallHashTable.at(hash) = node;
                        }
                    }
                }
                i++;
            }
            std::size_t startCollideObjectsListSize = collideObjectsList.size();
            for(auto i = objects.begin(); i != objects.end(); i++)
            {
                std::shared_ptr<PhysicsObject> objectA = i->lock();
                if(objectA->isStatic())
                    continue;
                collideObjectsList.resize(startCollideObjectsListSize);
                PositionF position = objectA->getPosition();
                VectorF extents = objectA->getExtents();
                float fMinX = position.x - extents.x - searchEps;
                float fMaxX = position.x + extents.x + searchEps;
                int minX = ifloor(fMinX * xScaleFactor);
                int maxX = iceil(fMaxX * xScaleFactor);
                float fMinZ = position.z - extents.z - searchEps;
                float fMaxZ = position.z + extents.z + searchEps;
                int minZ = ifloor(fMinZ * zScaleFactor);
                int maxZ = iceil(fMaxZ * zScaleFactor);
                checked_array<HashNode *, smallHashPrime> perObjectHashTable;
                perObjectHashTable.fill(nullptr);
                for(int xPosition = minX; xPosition <= maxX; xPosition++)
                {
                    for(int zPosition = minZ; zPosition <= maxZ; zPosition++)
                    {
                        std::size_t hash = (std::size_t)(xPosition * 8191 + zPosition);
                        hash %= bigHashPrime;
                        HashNode * node = overallHashTable.at(hash);
                        while(node != nullptr)
                        {
                            if(node->x == xPosition && node->z == zPosition) // found one
                            {
                                std::size_t perObjectHash = std::hash<std::shared_ptr<PhysicsObject>>()(node->object) % smallHashPrime;
                                HashNode ** pnode = &perObjectHashTable.at(perObjectHash);
                                HashNode * node2 = *pnode;
                                bool found = false;
                                while(node2 != nullptr)
                                {
                                    if(node2->object == node->object)
                                    {
                                        found = true;
                                        break;
                                    }
                                    pnode = &node2->hashNext;
                                    node2 = *pnode;
                                }
                                if(!found)
                                {
                                    node2 = freeListHead;
                                    if(node2 == nullptr)
                                        node2 = new HashNode;
                                    else
                                        freeListHead = node2->hashNext;
                                    node2->hashNext = perObjectHashTable.at(perObjectHash);
                                    node2->object = node->object;
                                    node2->x = node2->z = 0;
                                    perObjectHashTable.at(perObjectHash) = node2;
                                    collideObjectsList.push_back(node->object);
                                }
                            }
                            node = node->hashNext;
                        }
                    }
                }
                for(HashNode * node : perObjectHashTable)
                {
                    while(node != nullptr)
                    {
                        HashNode * nextNode = node->hashNext;
                        node->hashNext = freeListHead;
                        freeListHead = node;
                        node = nextNode;
                    }
                }
                for(auto objectB : collideObjectsList)
                {
                    if(objectA != objectB && objectA->collides(*objectB))
                    {
                        anyCollisions = true;
                        objectA->adjustPosition(*objectB);
                        //debugLog << "collision" << std::endl;
                    }
                }
                minX = ifloor(fMinX);
                maxX = iceil(fMaxX);
                float fMinY = position.y - extents.y - searchEps;
                float fMaxY = position.y + extents.y + searchEps;
                int minY = ifloor(fMinY);
                int maxY = iceil(fMaxY);
                minZ = ifloor(fMinZ);
                maxZ = iceil(fMaxZ);
                std::shared_ptr<PhysicsObject> objectB;
                BlockIterator bix = getBlockIterator(PositionI(minX, minY, minZ, position.d));
                for(int xPosition = minX; xPosition <= maxX; xPosition++, bix.moveTowardPX())
                {
                    BlockIterator bixy = bix;
                    for(int yPosition = minY; yPosition <= maxY; yPosition++, bixy.moveTowardPY())
                    {
                        BlockIterator bi = bixy;
                        for(int zPosition = minZ; zPosition <= maxZ; zPosition++, bi.moveTowardPZ())
                        {
                            setObjectToBlock(objectB, bi, lock_manager);
                            if(objectA->collides(*objectB))
                            {
                                anyCollisions = true;
                                objectA->adjustPosition(*objectB);
                            }
                        }
                    }
                }
                if(objectA->constraints)
                {
                    for(PhysicsConstraint constraint : *objectA->constraints)
                    {
                        constraint(objectA->position[getNewVariableSetIndex()], objectA->velocity[getNewVariableSetIndex()]);
                    }
                }
            }
            for(HashNode * node : overallHashTable)
            {
                while(node != nullptr)
                {
                    HashNode * nextNode = node->hashNext;
                    node->hashNext = freeListHead;
                    freeListHead = node;
                    node = nextNode;
                }
            }
            swapVariableSetIndex();
        }
    }
}
}
}
