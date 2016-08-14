/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef VIEW_POINT_H_INCLUDED
#define VIEW_POINT_H_INCLUDED

#include "world/world.h"
#include "render/mesh.h"
#include "render/renderer.h"
#include <memory>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
class ViewPoint final
{
    friend class World;
    ViewPoint(const ViewPoint &rt) = delete;
    const ViewPoint &operator=(const ViewPoint &) = delete;

private:
    struct Implementation;
    const std::shared_ptr<Implementation> implementation;

public:
    ViewPoint(World &world, PositionF position, std::int32_t viewDistance = 48);
    PositionF getPosition();
    std::int32_t getViewDistance();
    void setPosition(PositionF newPosition);
    void setViewDistance(std::int32_t newViewDistance);
    void getPositionAndViewDistance(PositionF &position, std::int32_t &viewDistance);
    void setPositionAndViewDistance(PositionF position, std::int32_t viewDistance);
    void render(Renderer &renderer,
                Transform worldToCamera,
                WorldLockManager &lock_manager,
                Mesh additionalObjects = Mesh());
};
}
}

#endif // VIEW_POINT_H_INCLUDED
