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
#ifndef VIEW_POINT_H_INCLUDED
#define VIEW_POINT_H_INCLUDED

#include "world/world.h"
#include "render/render_layer.h"
#include "render/mesh.h"
#include "render/renderer.h"
#include "util/enum_traits.h"
#include <mutex>
#include <thread>

namespace programmerjake
{
namespace voxels
{
class ViewPoint final
{
    PositionF position;
    int32_t viewDistance;
    std::thread generateMeshesThread;
    std::recursive_mutex theLock;
    bool shuttingDown;
    std::shared_ptr<enum_array<Mesh, RenderLayer>> blockRenderMeshes;
    World &world;
    void generateMeshesFn();
public:
    ViewPoint(World &world, PositionF position, int32_t viewDistance = 48);
    ViewPoint(const ViewPoint &rt) = delete;
    const ViewPoint &operator =(const ViewPoint &) = delete;
    ~ViewPoint();
    PositionF getPosition()
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        return position;
    }
    int32_t getViewDistance()
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        return viewDistance;
    }
    void setPosition(PositionF newPosition)
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        position = newPosition;
    }
    void setViewDistance(int32_t newViewDistance)
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        viewDistance = newViewDistance;
    }
private:
    std::shared_ptr<enum_array<Mesh, RenderLayer>> getBlockRenderMeshes()
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        return blockRenderMeshes;
    }
public:
    void render(Renderer &renderer, Matrix worldToCamera)
    {
        std::shared_ptr<enum_array<Mesh, RenderLayer>> meshes = getBlockRenderMeshes();
        if(!meshes)
            return;
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            renderer << rl;
            renderer << transform(worldToCamera, meshes->at(rl));
        }
    }
};
}
}

#endif // VIEW_POINT_H_INCLUDED
