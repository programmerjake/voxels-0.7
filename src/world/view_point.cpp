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
#include "world/view_point.h"
#include <iostream>
#include <chrono>
#include "util/logging.h"
#include "util/wrapped_entity.h"
#include "util/block_chunk.h"
#include "util/checked_array.h"
#include "entity/entity.h"
#include "lighting/lighting.h"
#include <unordered_map>
#include <list>
#include "util/lock.h"
#include "util/iterator.h"
#include <algorithm>
#include "platform/thread_name.h"
#include "util/tls.h"

namespace programmerjake
{
namespace voxels
{
namespace
{
struct BlockLightingCacheBlock
{
    BlockLighting lighting = BlockLighting();
    bool valid = false;
};
struct BlockLightingCacheChunk
{
    BlockLightingCacheChunk()
        : basePosition(),
        lruListIterator(),
        wlp(),
        blocks()
    {
    }
    PositionI basePosition;
    std::list<PositionI>::iterator lruListIterator;
    BlockChunkInvalidateCountType lastValidCount = 0;
    WorldLightingProperties wlp;
    checked_array<checked_array<checked_array<BlockLightingCacheBlock, BlockChunk::subchunkSizeXYZ>, BlockChunk::subchunkSizeXYZ>, BlockChunk::subchunkSizeXYZ> blocks;
};
class BlockLightingCache final
{
    BlockLightingCache(const BlockLightingCache &) = delete;
    const BlockLightingCache &operator =(const BlockLightingCache &) = delete;
private:
    std::unordered_map<PositionI, BlockLightingCacheChunk> chunks;
    std::list<PositionI> lruList; // front is least recently used
    const std::size_t maxChunkCount;
    BlockLightingCacheChunk &getChunk(PositionI basePosition)
    {
        auto iter = chunks.find(basePosition);
        if(iter == chunks.end())
        {
            if(chunks.size() >= maxChunkCount)
            {
                chunks.erase(lruList.front());
                lruList.pop_front();
            }
            BlockLightingCacheChunk &retval = chunks[basePosition];
            retval.basePosition = basePosition;
            retval.lruListIterator = lruList.insert(lruList.end(), basePosition);
            return retval;
        }
        BlockLightingCacheChunk &retval = std::get<1>(*iter);
        lruList.splice(lruList.end(), lruList, retval.lruListIterator);
        return retval;
    }
public:
    BlockLightingCache(std::size_t maxChunkCount = 1 << 10)
        : chunks(),
        lruList(),
        maxChunkCount(maxChunkCount)
    {
    }
    BlockLighting getBlockLighting(BlockIterator bi, WorldLockManager &lock_manager, const WorldLightingProperties &wlp)
    {
        PositionI position = bi.position();
        PositionI basePosition = BlockChunk::getSubchunkBaseAbsolutePosition(position);
        VectorI relativePosition = BlockChunk::getSubchunkRelativePosition(position);
        BlockLightingCacheChunk &chunk = getChunk(basePosition);
        auto invalidateCount = bi.getInvalidateCount(lock_manager);
        if(chunk.lastValidCount < invalidateCount || chunk.wlp != wlp)
        {
            chunk.lastValidCount = invalidateCount;
            chunk.wlp = wlp;
            for(auto &i : chunk.blocks)
            {
                for(auto &j : i)
                {
                    for(BlockLightingCacheBlock &block : j)
                    {
                        block.valid = false;
                    }
                }
            }
        }
        BlockLightingCacheBlock &block = chunk.blocks[relativePosition.x][relativePosition.y][relativePosition.z];
        if(!block.valid)
        {
            block.lighting = Block::calcBlockLighting(bi, lock_manager, wlp);
        }
        return block.lighting;
    }
};

std::atomic_size_t cachedChunkMeshCount(0), cachedSubchunkMeshCount(0), cachedViewPointMeshCount(0);

enum_array<Mesh, RenderLayer> *incCachedMeshCount(std::atomic_size_t *meshCount, enum_array<Mesh, RenderLayer> *retval)
{
    meshCount->fetch_add(1);
    return retval;
}

std::shared_ptr<enum_array<Mesh, RenderLayer>> makeCachedMesh(std::atomic_size_t *meshCount)
{
    return std::shared_ptr<enum_array<Mesh, RenderLayer>>(incCachedMeshCount(meshCount, new enum_array<Mesh, RenderLayer>), [meshCount](enum_array<Mesh, RenderLayer> *v)
    {
        meshCount->fetch_sub(1);
        delete v;
    });
}

void dumpMeshStats()
{
#if 0
    getDebugLog() << L"Chunk Mesh Count:" << cachedChunkMeshCount.load() << L"  Subchunk Mesh Count:" << cachedSubchunkMeshCount.load() << L"  ViewPoint Mesh Count:" << cachedViewPointMeshCount.load() << postnl;
    //ObjectCounter<WrappedEntity, 0>::dumpCount();
    //ObjectCounter<PhysicsObject, 0>::dumpCount();
    //ObjectCounter<PhysicsWorld, 0>::dumpCount();
    //ObjectCounter<PhysicsWorld, 1>::dumpCount();
    ObjectCounter<BlockChunk, 0>::dumpCount();
#endif
}
}

void ViewPoint::queueSubchunk(PositionI subchunkBase, float distanceFromPlayer, std::unique_lock<std::mutex> &lockedSubchunkQueueLock)
{
    if(renderingSubchunks.count(subchunkBase) != 0)
        return;
    if(std::get<1>(queuedSubchunks.insert(subchunkBase)))
    {
        std::uint64_t currentPriority = 0;
        if(!subchunkRenderPriorityQueue.empty())
            currentPriority = subchunkRenderPriorityQueue.top().priority;
        std::uint64_t priority = static_cast<std::uint64_t>(distanceFromPlayer * 1024);
        priority += currentPriority;
        subchunkRenderPriorityQueue.push(SubchunkQueueNode(priority, subchunkBase));
        chunkInvalidSubchunkCountMap[BlockChunk::getChunkBasePosition(subchunkBase)]++;
    }
}

bool ViewPoint::getQueuedSubchunk(PositionI &subchunkBase, std::unique_lock<std::mutex> &lockedSubchunkQueueLock)
{
    if(subchunkRenderPriorityQueue.empty())
        return false;
    subchunkBase = subchunkRenderPriorityQueue.top().subchunkBase;
    subchunkRenderPriorityQueue.pop();
    queuedSubchunks.erase(subchunkBase);
    renderingSubchunks.insert(subchunkBase);
    return true;
}

void ViewPoint::finishedRenderingSubchunk(PositionI subchunkBase, std::unique_lock<std::mutex> &lockedSubchunkQueueLock)
{
    renderingSubchunks.erase(subchunkBase);
    auto iter = chunkInvalidSubchunkCountMap.find(BlockChunk::getChunkBasePosition(subchunkBase));
    if(iter != chunkInvalidSubchunkCountMap.end())
    {
        if(--std::get<1>(*iter) == 0)
        {
            chunkInvalidSubchunkCountMap.erase(iter);
        }
    }
}

bool ViewPoint::generateSubchunkMeshes(WorldLockManager &lock_manager, BlockIterator sbi, WorldLightingProperties wlp)
{
    static thread_local BlockLightingCache lightingCache;
    BlockChunkSubchunk &subchunk = sbi.getSubchunk();
    if(subchunk.generatingCachedMeshes.exchange(true))
        return false;
    sbi.updateLock(lock_manager);
    std::shared_ptr<enum_array<Mesh, RenderLayer>> subchunkMeshes = makeCachedMesh(&cachedSubchunkMeshCount);
    for(std::int32_t bx = 0; bx < BlockChunk::subchunkSizeXYZ; bx++)
    {
        for(std::int32_t by = 0; by < BlockChunk::subchunkSizeXYZ; by++)
        {
            for(std::int32_t bz = 0; bz < BlockChunk::subchunkSizeXYZ; bz++)
            {
                BlockIterator bbi = sbi;
                bbi.moveBy(VectorI(bx, by, bz), lock_manager.tls);
                BlockDescriptorPointer bd = bbi.get(lock_manager).descriptor;
                if(bd != nullptr)
                {
                    if(bd->drawsAnything(bbi.get(lock_manager), bbi, lock_manager))
                    {
                        enum_array<BlockLighting, BlockFaceOrNone> lighting;
                        lighting[BlockFaceOrNone::None] = lightingCache.getBlockLighting(bbi, lock_manager, wlp);
                        for(BlockFace bf : enum_traits<BlockFace>())
                        {
                            BlockIterator bfbi = bbi;
                            bfbi.moveToward(bf, lock_manager.tls);
                            lighting[toBlockFaceOrNone(bf)] = lightingCache.getBlockLighting(bfbi, lock_manager, wlp);
                        }
                        for(RenderLayer rl : enum_traits<RenderLayer>())
                        {
                            bd->render(bbi.get(lock_manager), subchunkMeshes->at(rl), bbi, lock_manager, rl, lighting);
                        }
                    }
                }
            }
        }
    }
    subchunk.cachedMeshes = subchunkMeshes;
    subchunk.generatingCachedMeshes = false;
    sbi.chunk->getChunkVariables().cachedMeshesUpToDate = false;
    return true;
}

namespace
{
float distanceFromInterval(float minimum, float maximum, float position)
{
    if(position < minimum)
        return minimum - position;
    if(position > maximum)
        return position - maximum;
    return 0.0f;
}

float getSubchunkDistance(VectorI subchunkBase, VectorF playerPosition)
{
    VectorF minCorner = VectorF(subchunkBase);
    VectorF maxCorner = minCorner + VectorF(static_cast<float>(BlockChunk::subchunkSizeXYZ));
    VectorF distance = VectorF(distanceFromInterval(minCorner.x, maxCorner.x, playerPosition.x),
                               distanceFromInterval(minCorner.y, maxCorner.y, playerPosition.y),
                               distanceFromInterval(minCorner.z, maxCorner.z, playerPosition.z));
    return abs(distance);
}
}

bool ViewPoint::generateChunkMeshes(std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes, WorldLockManager &lock_manager, BlockIterator cbi, WorldLightingProperties wlp, PositionF playerPosition)
{
    static thread_local BlockLightingCache lightingCache;
    PositionI chunkPosition = cbi.position();
    std::unique_lock<std::mutex> cachedChunkMeshesLock(cbi.chunk->getChunkVariables().cachedMeshesLock);
    if(meshes)
    {
        while(cbi.chunk->getChunkVariables().generatingCachedMeshes)
            cbi.chunk->getChunkVariables().cachedMeshesCond.wait(cachedChunkMeshesLock);
    }
    else if(cbi.chunk->getChunkVariables().generatingCachedMeshes)
    {
        return false; // if not the primary thread, go on to a different chunk
    }
    std::shared_ptr<enum_array<Mesh, RenderLayer>> chunkMeshes = cbi.chunk->getChunkVariables().cachedMeshes;
    if(!cbi.chunk->getChunkVariables().cachedMeshesUpToDate.exchange(true))
        chunkMeshes = nullptr;
    bool lightingChanged = false;
    if(wlp != cbi.chunk->getChunkVariables().wlp)
    {
        chunkMeshes = nullptr;
        cbi.chunk->getChunkVariables().wlp = wlp;
        lightingChanged = true;
    }
    cbi.chunk->getChunkVariables().generatingCachedMeshes = (chunkMeshes == nullptr);
    cachedChunkMeshesLock.unlock();
    if(chunkMeshes != nullptr)
    {
        if(meshes)
        {
            for(RenderLayer rl : enum_traits<RenderLayer>())
            {
                meshes->at(rl).append(chunkMeshes->at(rl));
            }
        }
        return false; // chunk is up-to-date
    }
    chunkMeshes = makeCachedMesh(&cachedChunkMeshCount);
    //getDebugLog() << L"generating ... (" << chunkPosition.x << L", " << chunkPosition.y << L", " << chunkPosition.z << L")\x1b[K\r" << post;
    VectorI subchunkIndex;
    for(subchunkIndex.x = 0; subchunkIndex.x < BlockChunk::subchunkCountX; subchunkIndex.x++)
    {
        for(subchunkIndex.y = 0; subchunkIndex.y < BlockChunk::subchunkCountY; subchunkIndex.y++)
        {
            for(subchunkIndex.z = 0; subchunkIndex.z < BlockChunk::subchunkCountZ; subchunkIndex.z++)
            {
                BlockIterator sbi = cbi;
                PositionI subchunkPosition = chunkPosition + BlockChunk::getChunkRelativePositionFromSubchunkIndex(subchunkIndex);
                sbi.moveTo(subchunkPosition, lock_manager.tls);
                BlockChunkSubchunk &subchunk = sbi.getSubchunk();
                std::shared_ptr<enum_array<Mesh, RenderLayer>> subchunkMeshes = subchunk.cachedMeshes.load(std::memory_order_relaxed);
                if(subchunkMeshes != nullptr)
                {
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        chunkMeshes->at(rl).append(subchunkMeshes->at(rl));
                    }
                }
                if(subchunk.cachedMeshesInvalidated.exchange(false) || lightingChanged)
                {
                    queueSubchunk(subchunkPosition, getSubchunkDistance(subchunkPosition, playerPosition));
                }
            }
        }
    }
    cachedChunkMeshesLock.lock();
    cbi.chunk->getChunkVariables().cachedMeshes = chunkMeshes;
    cbi.chunk->getChunkVariables().generatingCachedMeshes = false;
    cbi.chunk->getChunkVariables().cachedMeshesCond.notify_all();
    cachedChunkMeshesLock.unlock();
    for(RenderLayer rl : enum_traits<RenderLayer>())
    {
        meshes->at(rl).append(chunkMeshes->at(rl));
    }
    return true;
}

void ViewPoint::generateMeshesFn(TLS &tls)
{
    auto lastDumpMeshStatsTime = std::chrono::steady_clock::now();
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    std::shared_ptr<Meshes> cachedMeshes = nullptr;
    while(!shuttingDown)
    {
        PositionF position = this->position;
        std::int32_t viewDistance = this->viewDistance;
        std::shared_ptr<Meshes> meshes = cachedMeshes;
        cachedMeshes = nullptr;
        if(!meshes)
        {
            meshes = nextBlockRenderMeshes;
            nextBlockRenderMeshes = nullptr;
            if(meshes == nullptr)
                meshes = std::make_shared<Meshes>();
        }
        bool anyUpdates = false;
        std::shared_ptr<enum_array<Mesh, RenderLayer>> lastMeshes = nullptr;
        if(blockRenderMeshes == nullptr)
            anyUpdates = true;
        else
        {
            lastMeshes = std::shared_ptr<enum_array<Mesh, RenderLayer>>(blockRenderMeshes, &blockRenderMeshes->meshes);
        }
        lockIt.unlock();
        PositionI chunkPosition;
        PositionI minChunkPosition = BlockChunk::getChunkBasePosition((PositionI)position - VectorI(viewDistance));
        PositionI maxChunkPosition = BlockChunk::getChunkBasePosition((PositionI)position + VectorI(viewDistance));
        for(chunkPosition = minChunkPosition; chunkPosition.x <= maxChunkPosition.x; chunkPosition.x += BlockChunk::chunkSizeX)
        {
            for(chunkPosition.y = minChunkPosition.y; chunkPosition.y <= maxChunkPosition.y; chunkPosition.y += BlockChunk::chunkSizeY)
            {
                for(chunkPosition.z = minChunkPosition.z; chunkPosition.z <= maxChunkPosition.z; chunkPosition.z += BlockChunk::chunkSizeZ)
                {
                    lockIt.lock();
                    if(shuttingDown)
                        return;
                    lockIt.unlock();
                    World::ThreadPauseGuard pauseGuard(world);
                    pauseGuard.checkForPause();
                    WorldLockManager lock_manager(tls);
                    BlockIterator cbi = world.getBlockIterator(chunkPosition, lock_manager.tls);
                    WorldLightingProperties wlp = world.getLighting(chunkPosition.d);
                    if(generateChunkMeshes(meshes ? std::shared_ptr<enum_array<Mesh, RenderLayer>>(meshes, &meshes->meshes) : std::shared_ptr<enum_array<Mesh, RenderLayer>>(nullptr), lock_manager, cbi, wlp, position))
                        anyUpdates = true;
                }
                auto currentTime = std::chrono::steady_clock::now();
                if(currentTime - std::chrono::seconds(1) >= lastDumpMeshStatsTime)
                {
                    lastDumpMeshStatsTime = currentTime;
                    dumpMeshStats();
                }
            }
        }

        if(!anyUpdates)
        {
            if(lastMeshes == nullptr)
                anyUpdates = true;
            else
            {
                for(RenderLayer rl : enum_traits<RenderLayer>())
                {
                    if(meshes->meshes[rl] != (*lastMeshes)[rl])
                    {
                        anyUpdates = true;
                        break;
                    }
                }
            }
        }

        if(anyUpdates)
        {
            for(RenderLayer rl : enum_traits<RenderLayer>())
            {
                meshes->meshBuffers[rl].set(meshes->meshes[rl], true);
            }
        }

        if(anyUpdates == false)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        lockIt.lock();
        //if(anyUpdates)
            //debugLog << L"generated render meshes.\x1b[K" << std::endl;
        if(anyUpdates)
        {
            blockRenderMeshes = std::move(meshes);
        }
        else
        {
            for(RenderLayer rl : enum_traits<RenderLayer>())
            {
                meshes->meshes[rl].clear();
            }
            cachedMeshes = meshes;
        }
    }
}

void ViewPoint::generateSubchunkMeshesFn(TLS &tls)
{
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    while(!shuttingDown)
    {
        lockIt.unlock();
        PositionI subchunkBase;
        if(getQueuedSubchunk(subchunkBase))
        {
            WorldLockManager lock_manager(tls);
            BlockIterator sbi = world.getBlockIterator(subchunkBase, tls);
            WorldLightingProperties wlp = world.getLighting(subchunkBase.d);
            generateSubchunkMeshes(lock_manager, sbi, wlp);
            finishedRenderingSubchunk(subchunkBase);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        lockIt.lock();
    }
}

ViewPoint::ViewPoint(World &world, PositionF position, int32_t viewDistance)
    : position(position),
    viewDistance(viewDistance),
    generateSubchunkMeshesThreads(),
    generateMeshesThread(),
    theLock(),
    shuttingDown(false),
    blockRenderMeshes(nullptr),
    nextBlockRenderMeshes(nullptr),
    world(world),
    myPositionInViewPointsList(),
    pLightingCache(),
    subchunkRenderPriorityQueue(),
    queuedSubchunks(),
    renderingSubchunks(),
    chunkInvalidSubchunkCountMap(),
    subchunkQueueLock()
{
    {
        std::unique_lock<std::mutex> lockIt(world.viewPointsLock);
        myPositionInViewPointsList = world.viewPoints.insert(world.viewPoints.end(), this);
    }
    generateMeshesThread = std::thread([this]()
    {
        setThreadName(L"mesh primary generate");
        TLS tls;
        generateMeshesFn(tls);
    });
    std::size_t threadCount = getProcessorCount();
    for(std::size_t i = 0; i < threadCount; i++)
    {
        generateSubchunkMeshesThreads.emplace_back([this]()
        {
            setThreadName(L"mesh secondary generate");
            TLS tls;
            generateSubchunkMeshesFn(tls);
        });
    }
}
ViewPoint::~ViewPoint()
{
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    shuttingDown = true;
    lockIt.unlock();
    generateMeshesThread.join();
    for(std::thread &generateSubchunkMeshesThread : generateSubchunkMeshesThreads)
    {
        generateSubchunkMeshesThread.join();
    }
    {
        std::unique_lock<std::mutex> lockIt(world.viewPointsLock);
        world.viewPoints.erase(myPositionInViewPointsList);
    }
}
void ViewPoint::render(Renderer &renderer, Matrix worldToCamera, WorldLockManager &lock_manager, Mesh additionalObjects)
{
    Matrix cameraToWorld = inverse(worldToCamera);
    std::unique_lock<std::recursive_mutex> lockViewPoint(theLock);
    typedef std::unordered_map<std::thread::id, std::shared_ptr<BlockLightingCache>> LightingCacheMapType;
    std::shared_ptr<LightingCacheMapType> pLightingCacheMap = std::static_pointer_cast<LightingCacheMapType>(pLightingCache);
    if(!pLightingCacheMap)
    {
        pLightingCacheMap = std::make_shared<LightingCacheMapType>();
        pLightingCache = std::static_pointer_cast<void>(pLightingCacheMap);
    }
    std::shared_ptr<BlockLightingCache> &pBlockLightingCache = (*pLightingCacheMap)[std::this_thread::get_id()];
    if(pBlockLightingCache == nullptr)
        pBlockLightingCache = std::make_shared<BlockLightingCache>();
    BlockLightingCache &lightingCache = *pBlockLightingCache;
    std::shared_ptr<Meshes> meshes = blockRenderMeshes;
    if(!nextBlockRenderMeshes)
    {
        nextBlockRenderMeshes = std::make_shared<Meshes>();
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            std::size_t size = 0;
            if(meshes)
                size = meshes->meshes[rl].size();
            const std::size_t minSize = 1024;
            if(size < minSize)
                size = minSize;
            size = size * 2;
            if(rl == RenderLayer::Translucent)
                continue;
            nextBlockRenderMeshes->meshBuffers[rl] = MeshBuffer(size);
        }
    }
    enum_array<Mesh, RenderLayer> entityMeshes;
    entityMeshes[RenderLayer::Opaque].append(additionalObjects);
    PositionF position = this->position;
    std::int32_t viewDistance = this->viewDistance;
    lockViewPoint.unlock();
    PositionI chunkPosition;
    PositionI minChunkPosition = BlockChunk::getChunkBasePosition((PositionI)position - VectorI(viewDistance));
    PositionI maxChunkPosition = BlockChunk::getChunkBasePosition((PositionI)position + VectorI(viewDistance));
    for(chunkPosition = minChunkPosition; chunkPosition.x <= maxChunkPosition.x; chunkPosition.x += BlockChunk::chunkSizeX)
    {
        for(chunkPosition.y = minChunkPosition.y; chunkPosition.y <= maxChunkPosition.y; chunkPosition.y += BlockChunk::chunkSizeY)
        {
            for(chunkPosition.z = minChunkPosition.z; chunkPosition.z <= maxChunkPosition.z; chunkPosition.z += BlockChunk::chunkSizeZ)
            {
                BlockIterator cbi = world.getBlockIterator(chunkPosition, lock_manager.tls);
                lock_manager.clear();
                std::unique_lock<std::recursive_mutex> lockChunk(cbi.chunk->getChunkVariables().entityListLock);
                for(WrappedEntity &entity : cbi.chunk->getChunkVariables().entityList)
                {
                    if(!entity.entity.good())
                        continue;
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        entity.entity.descriptor->render(entity.entity, entityMeshes[rl], rl, cameraToWorld);
                    }
                }
            }
        }
    }
    BlockIterator bi = world.getBlockIterator((PositionI)position, lock_manager.tls);
    WorldLightingProperties wlp = world.getLighting(position.d);
    for(Mesh &mesh : entityMeshes)
    {
        for(Triangle &triangle : mesh.triangles)
        {
            {
                PositionF vertexPosition(triangle.p1, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - vertexPositionI;
                bi.moveTo(vertexPositionI, lock_manager.tls);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c1 = lighting.lightVertex(relativeVertexPosition, triangle.c1, triangle.n1);
            }
            {
                PositionF vertexPosition(triangle.p2, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - vertexPositionI;
                bi.moveTo(vertexPositionI, lock_manager.tls);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c2 = lighting.lightVertex(relativeVertexPosition, triangle.c2, triangle.n2);
            }
            {
                PositionF vertexPosition(triangle.p3, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - vertexPositionI;
                bi.moveTo(vertexPositionI, lock_manager.tls);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c3 = lighting.lightVertex(relativeVertexPosition, triangle.c3, triangle.n3);
            }
        }
    }
    std::size_t renderedTranslucentTriangles = entityMeshes[RenderLayer::Translucent].size();
    if(meshes)
        renderedTranslucentTriangles += meshes->meshes[RenderLayer::Translucent].size();
    static thread_local std::vector<std::pair<const Triangle *, float>> triangleIndirectArray;
    triangleIndirectArray.clear();
    triangleIndirectArray.reserve(renderedTranslucentTriangles);
    VectorF cameraPosition = cameraToWorld.apply(VectorF(0));
    if(meshes)
    {
        for(const Triangle &tri : meshes->meshes[RenderLayer::Translucent].triangles)
        {
            VectorF averagePosition = (tri.p1 + tri.p2 + tri.p3) * (1.0f / 3);
            float distanceSquared = absSquared(cameraPosition - averagePosition);
            triangleIndirectArray.emplace_back(&tri, distanceSquared);
        }
    }
    for(const Triangle &tri : entityMeshes[RenderLayer::Translucent].triangles)
    {
        VectorF averagePosition = (tri.p1 + tri.p2 + tri.p3) * (1.0f / 3);
        float distanceSquared = absSquared(cameraPosition - averagePosition);
        triangleIndirectArray.emplace_back(&tri, distanceSquared);
    }
    std::sort(triangleIndirectArray.begin(), triangleIndirectArray.end(), [](std::pair<const Triangle *, float> a, std::pair<const Triangle *, float> b)->bool
    {
        return std::get<1>(a) > std::get<1>(b);
    });
    static thread_local Mesh translucentMesh;
    translucentMesh.clear();
    translucentMesh.triangles.reserve(renderedTranslucentTriangles);
    for(std::pair<const Triangle *, float> tri : triangleIndirectArray)
    {
        translucentMesh.triangles.push_back(*std::get<0>(tri));
    }
    triangleIndirectArray.clear();
    translucentMesh.image = entityMeshes[RenderLayer::Translucent].image;
    if(!translucentMesh.image && meshes)
        translucentMesh.image = meshes->meshes[RenderLayer::Translucent].image;
    std::size_t renderedTriangles = renderedTranslucentTriangles;
    for(RenderLayer rl : enum_traits<RenderLayer>())
    {
        renderer << rl;
        if(rl == RenderLayer::Translucent)
        {
            renderer << transform(worldToCamera, translucentMesh);
            continue;
        }
        if(meshes)
        {
            renderedTriangles += meshes->meshes[rl].size();
            if(meshes->meshBuffers[rl].empty())
            {
                renderer << transform(worldToCamera, meshes->meshes[rl]);
            }
            else
            {
                renderer << transform(worldToCamera, meshes->meshBuffers[rl]);
            }
        }
        renderer << transform(worldToCamera, entityMeshes[rl]);
        renderedTriangles += entityMeshes[rl].size();
    }
    getDebugLog() << L"rendered " << renderedTriangles << L"/" << renderedTranslucentTriangles << L" triangles." << postr;
}
}
}
