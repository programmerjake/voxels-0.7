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
