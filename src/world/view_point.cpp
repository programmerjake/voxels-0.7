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
