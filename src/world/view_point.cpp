/*
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

namespace programmerjake
{
namespace voxels
{
namespace
{
struct BlockLightingCacheBlock
{
    BlockLighting lighting;
    bool valid = false;
};
struct BlockLightingCacheChunk
{
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
        : maxChunkCount(maxChunkCount)
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
}

void ViewPoint::generateMeshesFn()
{
    BlockLightingCache lightingCache;
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    while(!shuttingDown)
    {
        PositionF position = this->position;
        std::int32_t viewDistance = this->viewDistance;
        std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes = std::make_shared<enum_array<Mesh, RenderLayer>>();
        bool anyUpdates = false;
        if(blockRenderMeshes == nullptr)
            anyUpdates = true;
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
                    std::shared_ptr<enum_array<Mesh, RenderLayer>> chunkMeshes = cbi.chunk->chunkVariables.cachedMeshes.load();
                    if(chunkMeshes != nullptr)
                    {
                        for(RenderLayer rl : enum_traits<RenderLayer>())
                        {
                            meshes->at(rl).append(chunkMeshes->at(rl));
                        }
                        continue; // chunk is up-to-date
                    }
                    chunkMeshes = std::make_shared<enum_array<Mesh, RenderLayer>>();
                    anyUpdates = true;
                    //debugLog << L"generating ... (" << chunkPosition.x << L", " << chunkPosition.y << L", " << chunkPosition.z << L")\x1b[K\r" << std::flush;
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
                                std::shared_ptr<enum_array<Mesh, RenderLayer>> subchunkMeshes = sbi.getSubchunk().cachedMeshes.load();
                                if(subchunkMeshes != nullptr)
                                {
                                    for(RenderLayer rl : enum_traits<RenderLayer>())
                                    {
                                        chunkMeshes->at(rl).append(subchunkMeshes->at(rl));
                                    }
                                    continue; // subchunk is up-to-date
                                }
                                subchunkMeshes = std::make_shared<enum_array<Mesh, RenderLayer>>();
                                WorldLightingProperties wlp = world.getLighting();
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
                                                    BlockLighting lighting = lightingCache.getBlockLighting(bbi, lock_manager, wlp);
                                                    for(RenderLayer rl : enum_traits<RenderLayer>())
                                                    {
                                                        bd->render(bbi.get(lock_manager), subchunkMeshes->at(rl), bbi, lock_manager, rl, lighting);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                sbi.getSubchunk().cachedMeshes.store(subchunkMeshes);
                                for(RenderLayer rl : enum_traits<RenderLayer>())
                                {
                                    chunkMeshes->at(rl).append(subchunkMeshes->at(rl));
                                }
                            }
                        }
                    }
                    cbi.chunk->chunkVariables.cachedMeshes.store(chunkMeshes);
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        meshes->at(rl).append(chunkMeshes->at(rl));
                    }
                }
            }
        }

        if(anyUpdates == false)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        lockIt.lock();
        //if(anyUpdates)
            //debugLog << L"generated render meshes.\x1b[K" << std::endl;
        blockRenderMeshes = std::move(meshes);
    }
}
ViewPoint::ViewPoint(World &world, PositionF position, int32_t viewDistance)
    : position(position), viewDistance(viewDistance), shuttingDown(false), blockRenderMeshes(nullptr), world(world)
{
    {
        std::unique_lock<std::mutex> lockIt(world.viewPointsLock);
        myPositionInViewPointsList = world.viewPoints.insert(world.viewPoints.end(), this);
    }
    generateMeshesThread = std::thread([this](){generateMeshesFn();});
}
ViewPoint::~ViewPoint()
{
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    shuttingDown = true;
    lockIt.unlock();
    generateMeshesThread.join();
    {
        std::unique_lock<std::mutex> lockIt(world.viewPointsLock);
        world.viewPoints.erase(myPositionInViewPointsList);
    }
}
void ViewPoint::render(Renderer &renderer, Matrix worldToCamera, WorldLockManager &lock_manager)
{
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
    std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes = blockRenderMeshes;
    enum_array<Mesh, RenderLayer> entityMeshes;
    PositionF position = this->position;
    std::int32_t viewDistance = this->viewDistance;
    if(!meshes)
        return;
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
                std::unique_lock<std::recursive_mutex> lockChunk(cbi.chunk->chunkVariables.entityListLock);
                for(WrappedEntity *entity = cbi.chunk->chunkVariables.entityListHead; entity != nullptr; entity = entity->chunk_next)
                {
                    if(!entity->entity.good())
                        continue;
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        entity->entity.descriptor->render(entity->entity, entityMeshes.at(rl), rl);
                    }
                }
            }
        }
    }
    BlockIterator bi = world.getBlockIterator((PositionI)position);
    WorldLightingProperties wlp = world.getLighting();
    for(Mesh &mesh : entityMeshes)
    {
        for(Triangle &triangle : mesh.triangles)
        {
            {
                PositionF vertexPosition(triangle.p1, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - relativeVertexPosition;
                bi.moveTo(vertexPositionI);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c1 = lighting.lightVertex(relativeVertexPosition, triangle.c1, triangle.n1);
            }
            {
                PositionF vertexPosition(triangle.p2, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - relativeVertexPosition;
                bi.moveTo(vertexPositionI);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c2 = lighting.lightVertex(relativeVertexPosition, triangle.c2, triangle.n2);
            }
            {
                PositionF vertexPosition(triangle.p3, position.d);
                PositionI vertexPositionI = (PositionI)vertexPosition;
                VectorF relativeVertexPosition = vertexPosition - relativeVertexPosition;
                bi.moveTo(vertexPositionI);
                BlockLighting lighting = lightingCache.getBlockLighting(bi, lock_manager, wlp);
                triangle.c3 = lighting.lightVertex(relativeVertexPosition, triangle.c3, triangle.n3);
            }
        }
    }
    for(RenderLayer rl : enum_traits<RenderLayer>())
    {
        renderer << rl;
        renderer << transform(worldToCamera, meshes->at(rl));
        renderer << transform(worldToCamera, entityMeshes.at(rl));
    }
}
}
}
