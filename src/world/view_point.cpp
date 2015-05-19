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
    BlockLightingCache(std::size_t maxChunkCount = 1 << 14)
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
    ObjectCounter<WrappedEntity, 0>::dumpCount();
    ObjectCounter<PhysicsObject, 0>::dumpCount();
    ObjectCounter<PhysicsWorld, 0>::dumpCount();
    ObjectCounter<PhysicsWorld, 1>::dumpCount();
    ObjectCounter<BlockChunk, 0>::dumpCount();
#endif
}
}

void ViewPoint::copyChunkMeshes(World &sourceWorld, World &destWorld, WorldLockManager &source_lock_manager, WorldLockManager &dest_lock_manager, BlockIterator sourceChunkBlockIterator, BlockIterator destChunkBlockIterator)
{
    assert(sourceChunkBlockIterator.position() == destChunkBlockIterator.position());
    PositionI chunkPosition = destChunkBlockIterator.position();
    std::unique_lock<std::mutex> sourceCachedChunkMeshesLock(sourceChunkBlockIterator.chunk->chunkVariables.cachedMeshesLock, std::defer_lock);
    std::unique_lock<std::mutex> destCachedChunkMeshesLock(destChunkBlockIterator.chunk->chunkVariables.cachedMeshesLock, std::defer_lock);
    source_lock_manager.block_biome_lock.clear();
    dest_lock_manager.block_biome_lock.clear();
    std::lock(sourceCachedChunkMeshesLock, destCachedChunkMeshesLock);
    if(!sourceChunkBlockIterator.chunk->chunkVariables.cachedMeshesUpToDate.load())
        destChunkBlockIterator.chunk->chunkVariables.cachedMeshesUpToDate.store(false);
    if(!sourceChunkBlockIterator.chunk->chunkVariables.generatingCachedMeshes)
        destChunkBlockIterator.chunk->chunkVariables.cachedMeshes = sourceChunkBlockIterator.chunk->chunkVariables.cachedMeshes;
    else
        return;
    sourceCachedChunkMeshesLock.unlock();
    destCachedChunkMeshesLock.unlock();
    VectorI subchunkIndex;
    for(subchunkIndex.x = 0; subchunkIndex.x < BlockChunk::subchunkCountX; subchunkIndex.x++)
    {
        for(subchunkIndex.y = 0; subchunkIndex.y < BlockChunk::subchunkCountY; subchunkIndex.y++)
        {
            for(subchunkIndex.z = 0; subchunkIndex.z < BlockChunk::subchunkCountZ; subchunkIndex.z++)
            {
                BlockIterator sourceSubchunkBlockIterator = sourceChunkBlockIterator;
                BlockIterator destSubchunkBlockIterator = destChunkBlockIterator;
                PositionI subchunkPosition = chunkPosition + BlockChunk::getChunkRelativePositionFromSubchunkIndex(subchunkIndex);
                sourceSubchunkBlockIterator.moveTo(subchunkPosition);
                destSubchunkBlockIterator.moveTo(subchunkPosition);
                BlockChunkSubchunk &sourceSubchunk = sourceSubchunkBlockIterator.getSubchunk();
                BlockChunkSubchunk &destSubchunk = destSubchunkBlockIterator.getSubchunk();
                auto otherLocks = unit_range(sourceSubchunk.lock);
                source_lock_manager.block_biome_lock.clear();
                destSubchunkBlockIterator.updateLock(dest_lock_manager, otherLocks.begin(), otherLocks.end());
                source_lock_manager.block_biome_lock.adopt(sourceSubchunk.lock);
                if(!sourceSubchunk.cachedMeshesUpToDate.load())
                    destSubchunk.cachedMeshesUpToDate.store(false);
                destSubchunk.cachedMeshes = sourceSubchunk.cachedMeshes;
            }
        }
    }
    source_lock_manager.block_biome_lock.clear();
    dest_lock_manager.block_biome_lock.clear();
}

bool ViewPoint::generateChunkMeshes(std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes, WorldLockManager &lock_manager, BlockIterator cbi, WorldLightingProperties wlp)
{
    static thread_local BlockLightingCache lightingCache;
    PositionI chunkPosition = cbi.position();
    std::unique_lock<std::mutex> cachedChunkMeshesLock(cbi.chunk->chunkVariables.cachedMeshesLock);
    if(meshes)
    {
        while(cbi.chunk->chunkVariables.generatingCachedMeshes)
            cbi.chunk->chunkVariables.cachedMeshesCond.wait(cachedChunkMeshesLock);
    }
    else if(cbi.chunk->chunkVariables.generatingCachedMeshes)
    {
        return false; // if not the primary thread, go on to a different chunk
    }
    std::shared_ptr<enum_array<Mesh, RenderLayer>> chunkMeshes = cbi.chunk->chunkVariables.cachedMeshes;
    if(!cbi.chunk->chunkVariables.cachedMeshesUpToDate.exchange(true))
        chunkMeshes = nullptr;
    bool lightingChanged = false;
    if(wlp != cbi.chunk->chunkVariables.wlp)
    {
        chunkMeshes = nullptr;
        cbi.chunk->chunkVariables.wlp = wlp;
        lightingChanged = true;
    }
    cbi.chunk->chunkVariables.generatingCachedMeshes = (chunkMeshes == nullptr);
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
                sbi.moveTo(subchunkPosition);
                sbi.updateLock(lock_manager);
                BlockChunkSubchunk &subchunk = sbi.getSubchunk();
                if(!subchunk.cachedMeshesUpToDate.exchange(true))
                    subchunk.cachedMeshes = nullptr;
                if(lightingChanged && subchunk.cachedMeshes != nullptr)
                {
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        if(subchunk.cachedMeshes->at(rl).size() != 0) // if there's nothing displayed here then we don't need to re-render
                        {
                            subchunk.cachedMeshes = nullptr;
                            break;
                        }
                    }
                }
                std::shared_ptr<enum_array<Mesh, RenderLayer>> subchunkMeshes = subchunk.cachedMeshes;
                if(subchunkMeshes != nullptr)
                {
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        chunkMeshes->at(rl).append(subchunkMeshes->at(rl));
                    }
                    continue; // subchunk is up-to-date
                }
                subchunkMeshes = makeCachedMesh(&cachedSubchunkMeshCount);
                for(std::int32_t bx = 0; bx < BlockChunk::subchunkSizeXYZ; bx++)
                {
                    for(std::int32_t by = 0; by < BlockChunk::subchunkSizeXYZ; by++)
                    {
                        for(std::int32_t bz = 0; bz < BlockChunk::subchunkSizeXYZ; bz++)
                        {
                            BlockIterator bbi = sbi;
                            bbi.moveTo(subchunkPosition + VectorI(bx, by, bz));
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
                                        bfbi.moveToward(bf);
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
                sbi.updateLock(lock_manager);
                subchunk.cachedMeshes = subchunkMeshes;
                for(RenderLayer rl : enum_traits<RenderLayer>())
                {
                    chunkMeshes->at(rl).append(subchunkMeshes->at(rl));
                }
            }
        }
    }
    cachedChunkMeshesLock.lock();
    cbi.chunk->chunkVariables.cachedMeshes = chunkMeshes;
    cbi.chunk->chunkVariables.generatingCachedMeshes = false;
    cbi.chunk->chunkVariables.cachedMeshesCond.notify_all();
    cachedChunkMeshesLock.unlock();
    if(meshes)
    {
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            meshes->at(rl).append(chunkMeshes->at(rl));
        }
    }
    return true;
}

void ViewPoint::generateMeshesFn(bool isPrimaryThread)
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
        if(isPrimaryThread && !meshes)
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
                    WorldLockManager lock_manager;
                    BlockIterator cbi = world.getBlockIterator(chunkPosition);
                    WorldLightingProperties wlp = world.getLighting(chunkPosition.d);
                    if(generateChunkMeshes(meshes ? std::shared_ptr<enum_array<Mesh, RenderLayer>>(meshes, &meshes->meshes) : std::shared_ptr<enum_array<Mesh, RenderLayer>>(nullptr), lock_manager, cbi, wlp))
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

        if(isPrimaryThread && !anyUpdates)
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

        if(isPrimaryThread && anyUpdates)
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
        if(isPrimaryThread && anyUpdates)
        {
            blockRenderMeshes = std::move(meshes);
        }
        else if(meshes)
        {
            for(RenderLayer rl : enum_traits<RenderLayer>())
            {
                meshes->meshes[rl].clear();
            }
            cachedMeshes = meshes;
        }
    }
}
ViewPoint::ViewPoint(World &world, PositionF position, int32_t viewDistance)
    : position(position),
    viewDistance(viewDistance),
    generateMeshesThreads(),
    theLock(),
    shuttingDown(false),
    blockRenderMeshes(nullptr),
    nextBlockRenderMeshes(nullptr),
    world(world),
    myPositionInViewPointsList(),
    pLightingCache()
{
    {
        std::unique_lock<std::mutex> lockIt(world.viewPointsLock);
        myPositionInViewPointsList = world.viewPoints.insert(world.viewPoints.end(), this);
    }
    const int threadCount = 5;
    generateMeshesThreads.emplace_back([this](){generateMeshesFn(true);});
    for(int i = 1; i < threadCount; i++)
        generateMeshesThreads.emplace_back([this](){generateMeshesFn(false);});
}
ViewPoint::~ViewPoint()
{
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    shuttingDown = true;
    lockIt.unlock();
    for(std::thread &generateMeshesThread : generateMeshesThreads)
    {
        generateMeshesThread.join();
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
                BlockIterator cbi = world.getBlockIterator(chunkPosition);
                lock_manager.clear();
                std::unique_lock<std::recursive_mutex> lockChunk(cbi.chunk->chunkVariables.entityListLock);
                for(WrappedEntity &entity : cbi.chunk->chunkVariables.entityList)
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
    BlockIterator bi = world.getBlockIterator((PositionI)position);
    WorldLightingProperties wlp = world.getLighting(position.d);
    for(Mesh &mesh : entityMeshes)
    {
        for(Triangle &triangle : mesh.triangles)
        {
            {
                PositionF vertexPosition(triangle.p1, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - vertexPositionI;
                bi.moveTo(vertexPositionI);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c1 = lighting.lightVertex(relativeVertexPosition, triangle.c1, triangle.n1);
            }
            {
                PositionF vertexPosition(triangle.p2, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - vertexPositionI;
                bi.moveTo(vertexPositionI);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c2 = lighting.lightVertex(relativeVertexPosition, triangle.c2, triangle.n2);
            }
            {
                PositionF vertexPosition(triangle.p3, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - vertexPositionI;
                bi.moveTo(vertexPositionI);
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
