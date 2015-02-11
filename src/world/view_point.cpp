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

namespace programmerjake
{
namespace voxels
{
void ViewPoint::generateMeshesFn()
{
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
        {
            WorldLockManager lock_manager;
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
                                                BlockLighting lighting = Block::calcBlockLighting(bbi, lock_manager, wlp);
                                                BlockDescriptorPointer bd = bbi.get(lock_manager).descriptor;
                                                if(bd != nullptr)
                                                {
                                                    for(RenderLayer rl : enum_traits<RenderLayer>())
                                                    {
                                                        bd->render(bbi.get(lock_manager), subchunkMeshes->at(rl), bbi, lock_manager, rl, lighting);
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
        }
        if(anyUpdates == false)
            std::this_thread::yield();
        lockIt.lock();
        blockRenderMeshes = std::move(meshes);
    }
}
ViewPoint::ViewPoint(World &world, PositionF position, int32_t viewDistance)
    : position(position), viewDistance(viewDistance), shuttingDown(false), blockRenderMeshes(nullptr), world(world)
{
    generateMeshesThread = std::thread([this](){generateMeshesFn();});
}
ViewPoint::~ViewPoint()
{
    std::unique_lock<std::recursive_mutex> lockIt(theLock);
    shuttingDown = true;
    lockIt.unlock();
    generateMeshesThread.join();
}
}
}
