#include "player/player.h"

void drawWorld(World &world, Renderer &renderer, Matrix playerToWorld, Dimension d, int32_t viewDistance)
{
    Matrix worldToPlayer = inverse(playerToWorld);
    world.updateCachedMeshes();
    for(RenderLayer rl : enum_traits<RenderLayer>())
    {
        //cout << (int)rl << " " << world.getMeshes()[rl].size() << endl;
        renderer << rl << transform(worldToPlayer, world.getCachedMeshes()[rl]);
    }
}
