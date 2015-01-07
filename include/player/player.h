#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "stream/stream.h"
#include "world/world.h"
#include "render/renderer.h"

namespace programmerjake
{
namespace voxels
{
struct PlayerData
{
    std::wstring name;
};

void drawWorld(World &world, Renderer &renderer, Matrix playerToWorld, Dimension d, std::int32_t viewDistance);
}
}

#endif // PLAYER_H_INCLUDED
