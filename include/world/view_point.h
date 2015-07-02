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
#ifndef VIEW_POINT_H_INCLUDED
#define VIEW_POINT_H_INCLUDED

#include "world/world.h"
#include "render/render_layer.h"
#include "render/mesh.h"
#include "render/renderer.h"
#include "util/enum_traits.h"
#include "platform/platform.h"
#include <mutex>
#include <thread>
#include <list>
#include "util/tls.h"
#include <queue>
#include <vector>

namespace programmerjake
{
namespace voxels
{
class ViewPoint final
{
    friend class World;
    ViewPoint(const ViewPoint &rt) = delete;
    const ViewPoint &operator =(const ViewPoint &) = delete;
private:
    struct Meshes final
    {
        enum_array<Mesh, RenderLayer> meshes;
        enum_array<MeshBuffer, RenderLayer> meshBuffers;
        Meshes()
            : meshes(), meshBuffers()
        {
        }
    };
    struct SubchunkQueueNode final
    {
        std::uint64_t priority; // smaller means run first
        PositionI subchunkBase;
        SubchunkQueueNode(std::uint64_t priority, PositionI subchunkBase)
            : priority(priority), subchunkBase(subchunkBase)
        {
        }
    };
    struct SubchunkQueueNodeCompare final
    {
        bool operator ()(const SubchunkQueueNode &a, const SubchunkQueueNode &b) const
        {
            std::uint64_t difference = b.priority - a.priority; // use subtract and check sign bit to handle wraparound
            const std::uint64_t mask = (std::uint64_t)1 << 63;
            if(difference & mask)
                return true;
            return false;
        }
    };
private:
    PositionF position;
    std::int32_t viewDistance;
    std::list<std::thread> generateSubchunkMeshesThreads;
    std::thread generateMeshesThread;
    std::recursive_mutex theLock;
    bool shuttingDown;
    std::shared_ptr<Meshes> blockRenderMeshes;
    std::shared_ptr<Meshes> nextBlockRenderMeshes;
    World &world;
    std::list<ViewPoint *>::iterator myPositionInViewPointsList;
    std::shared_ptr<void> pLightingCache;
    std::priority_queue<SubchunkQueueNode, std::vector<SubchunkQueueNode>, SubchunkQueueNodeCompare> subchunkRenderPriorityQueue;
    std::unordered_set<PositionI> queuedSubchunks;
    std::unordered_set<PositionI> renderingSubchunks;
    std::unordered_map<PositionI, std::size_t> chunkInvalidSubchunkCountMap;
    std::mutex subchunkQueueLock;
private:
    void generateSubchunkMeshesFn(TLS &tls);
    void generateMeshesFn(TLS &tls);
    void queueSubchunk(PositionI subchunkBase, float distanceFromPlayer, std::unique_lock<std::mutex> &lockedSubchunkQueueLock);
    void queueSubchunk(PositionI subchunkBase, float distanceFromPlayer)
    {
        std::unique_lock<std::mutex> lockIt(subchunkQueueLock);
        queueSubchunk(subchunkBase, distanceFromPlayer, lockIt);
    }
    bool getQueuedSubchunk(PositionI &subchunkBase, std::unique_lock<std::mutex> &lockedSubchunkQueueLock);
    bool getQueuedSubchunk(PositionI &subchunkBase)
    {
        std::unique_lock<std::mutex> lockIt(subchunkQueueLock);
        return getQueuedSubchunk(subchunkBase, lockIt);
    }
    void finishedRenderingSubchunk(PositionI subchunkBase, std::unique_lock<std::mutex> &lockedSubchunkQueueLock);
    void finishedRenderingSubchunk(PositionI subchunkBase)
    {
        std::unique_lock<std::mutex> lockIt(subchunkQueueLock);
        finishedRenderingSubchunk(subchunkBase, lockIt);
    }
    static bool generateSubchunkMeshes(WorldLockManager &lock_manager, BlockIterator sbi, WorldLightingProperties wlp);
    bool generateChunkMeshes(std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes, WorldLockManager &lock_manager, BlockIterator cbi, WorldLightingProperties wlp, PositionF playerPosition);
public:
    ViewPoint(World &world, PositionF position, std::int32_t viewDistance = 48);
    ~ViewPoint();
    PositionF getPosition()
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        return position;
    }
    std::int32_t getViewDistance()
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        return viewDistance;
    }
    void setPosition(PositionF newPosition)
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        position = newPosition;
    }
    void setViewDistance(std::int32_t newViewDistance)
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        viewDistance = newViewDistance;
    }
    void getPositionAndViewDistance(PositionF &position, std::int32_t &viewDistance)
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        position = this->position;
        viewDistance = this->viewDistance;
    }
    void setPositionAndViewDistance(PositionF position, std::int32_t viewDistance)
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        this->position = position;
        this->viewDistance = viewDistance;
    }
    void render(Renderer &renderer, Matrix worldToCamera, WorldLockManager &lock_manager, Mesh additionalObjects = Mesh());
};
}
}

#endif // VIEW_POINT_H_INCLUDED
