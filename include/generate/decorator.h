/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
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
#ifndef DECORATOR_H_INCLUDED
#define DECORATOR_H_INCLUDED

#include <memory>
#include <string>
#include <unordered_map>
#include <cassert>
#include <vector>
#include "util/position.h"
#include "util/block_iterator.h"
#include "util/world_lock_manager.h"
#include "util/checked_array.h"
#include "generate/decorator_declaration.h"
#include "util/block_chunk.h"

namespace programmerjake
{
namespace voxels
{
class World;
class RandomSource;
class Decorator
{
    friend class Decorators_t;
    Decorator(const Decorator &) = delete;
    const Decorator &operator =(const Decorator &) = delete;
private:
    DecoratorIndex index;
public:
    const std::wstring name;
    /** the distance (in additional chunks) to search to see if this Decorator is used.
     * (ex. 0 means this decorator doesn't generate past chunk boundaries,
     * whereas 5 means that this decorator could potentially span a distance of 5 chunks
     * from the central chunk in the X and Z directions.)
     */
    const int chunkSearchDistance;
    DecoratorIndex getIndex() const
    {
        return index;
    }
    virtual ~Decorator() = default;
protected:
    Decorator(std::wstring name, int chunkSearchDistance);
public:
    /** @brief generate this decorator in a chunk
     *
     * @param simulate if the generation should be simulated (viz., don't change any blocks or add entities, just return true if generation would succeed)
     * @param chunkBasePosition the base position of the chunk to generate in
     * @param columnBasePosition the base position of the column to generate in
     * @param surfacePosition the surface position of the column to generate in
     * @param lock_manager the WorldLockManager
     * @param world the World
     * @param blocks the blocks for this chunk
     * @param randomSource the RandomSource
     * @param generateNumber the number of times that a decorator was generated or tried to generate (use for picking a different position each time)
     * @return true if this decorator was generated or would be generated (if called again with simulate = false)
     *
     */
    virtual bool generateInChunk(bool simulate, PositionI chunkBasePosition, PositionI columnBasePosition, PositionI surfacePosition,
                                 WorldLockManager &lock_manager, World &world,
                                 checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks,
                                 RandomSource &randomSource, std::size_t generateNumber) const = 0;
};
}
}

#endif // DECORATOR_H_INCLUDED
