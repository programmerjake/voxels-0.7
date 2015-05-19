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

namespace programmerjake
{
namespace voxels
{
class ViewPoint final
{
    friend class World;
    PositionF position;
    std::int32_t viewDistance;
    std::list<std::thread> generateMeshesThreads;
    std::recursive_mutex theLock;
    bool shuttingDown;
    struct Meshes final
    {
        enum_array<Mesh, RenderLayer> meshes;
        enum_array<MeshBuffer, RenderLayer> meshBuffers;
        Meshes()
            : meshes(), meshBuffers()
        {
        }
    };
    std::shared_ptr<Meshes> blockRenderMeshes;
    std::shared_ptr<Meshes> nextBlockRenderMeshes;
    World &world;
    std::list<ViewPoint *>::iterator myPositionInViewPointsList;
    void generateMeshesFn(bool isPrimaryThread);
    std::shared_ptr<void> pLightingCache;
    static bool generateChunkMeshes(std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes, WorldLockManager &lock_manager, BlockIterator cbi, WorldLightingProperties wlp);
    static void copyChunkMeshes(World &sourceWorld, World &destWorld, WorldLockManager &source_lock_manager, WorldLockManager &dest_lock_manager, BlockIterator sourceChunkBlockIterator, BlockIterator destChunkBlockIterator);
public:
    ViewPoint(World &world, PositionF position, std::int32_t viewDistance = 48);
    ViewPoint(const ViewPoint &rt) = delete;
    const ViewPoint &operator =(const ViewPoint &) = delete;
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
public:
    void render(Renderer &renderer, Matrix worldToCamera, WorldLockManager &lock_manager, Mesh additionalObjects = Mesh());
};
}
}

#endif // VIEW_POINT_H_INCLUDED
