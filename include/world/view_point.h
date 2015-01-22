#ifndef VIEW_POINT_H_INCLUDED
#define VIEW_POINT_H_INCLUDED

#include "world/world.h"
#include "render/render_layer.h"
#include "render/mesh.h"
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
    std::shared_ptr<enum_array<Mesh, RenderLayer>> getBlockRenderMeshes()
    {
        std::unique_lock<std::recursive_mutex> lockIt(theLock);
        return blockRenderMeshes;
    }
};
}
}

#endif // VIEW_POINT_H_INCLUDED
