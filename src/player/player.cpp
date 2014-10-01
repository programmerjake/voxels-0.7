#include "player/player.h"

void drawWorld(World &world, Renderer &renderer, Matrix playerToWorld, Dimension d, int32_t viewDistance)
{
    PositionF viewPosition = PositionF(playerToWorld.apply(VectorF(0)), d);
    Matrix worldToPlayer = inverse(playerToWorld);
    world.updateCachedMeshes();
    static thread_local enum_array<Mesh, RenderLayer> entityMeshes;
    world.generateEntityMeshes(entityMeshes, (PositionI)viewPosition, viewDistance);
    for(RenderLayer rl : enum_traits<RenderLayer>())
    {
        //cout << (int)rl << " " << world.getMeshes()[rl].size() << endl;
        renderer << rl << transform(worldToPlayer, world.getCachedMeshes()[rl]) << transform(worldToPlayer, entityMeshes[rl]);
    }
}
