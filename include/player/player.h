#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "stream/stream.h"
#include "world/world.h"
#include "render/renderer.h"

struct PlayerData
{
    wstring name;
};

void drawWorld(World &world, Renderer &renderer, Matrix playerToWorld, Dimension d, int32_t viewDistance);

#endif // PLAYER_H_INCLUDED
