/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef WORLD_GENERATOR_H_INCLUDED
#define WORLD_GENERATOR_H_INCLUDED

#include "world/world.h"
#include <memory>
#include "util/tls.h"

namespace programmerjake
{
namespace voxels
{
class WorldGenerator
{
protected:
    WorldGenerator()
    {
    }
public:
    virtual ~WorldGenerator()
    {
    }
    virtual void generateChunk(PositionI chunkBasePosition, World &world, WorldLockManager &lock_manager, const std::atomic_bool *abortFlag) const = 0;
};
}
}

#endif // WORLD_GENERATOR_H_INCLUDED
