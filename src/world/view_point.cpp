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
        int32_t viewDistance = this->viewDistance;
        std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes = std::make_shared<enum_array<Mesh, RenderLayer>>();
        lockIt.unlock();
        {
            WorldLockManager lock_manager;
            PositionI chunkPosition;
            #error finish
        }
        lockIt.lock();
        blockRenderMeshes = std::move(meshes);
    }
}
ViewPoint::ViewPoint(World &world, PositionF position, int32_t viewDistance)
    : position(position), viewDistance(viewDistance), shuttingDown(false), blockRenderMeshes(nullptr), world(world)
{
    generateMeshesThread = thread([this](){generateMeshesFn();});
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
