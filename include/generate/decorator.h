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
#include "util/blocks_generate_array.h"
#include <cassert>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
class World;
class RandomSource;
GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
GCC_PRAGMA(diagnostic ignored "-Wnon-virtual-dtor")
class DecoratorInstance : public std::enable_shared_from_this<DecoratorInstance>
{
GCC_PRAGMA(diagnostic pop)
    DecoratorInstance(const DecoratorInstance &) = delete;
    DecoratorInstance &operator =(const DecoratorInstance &) = delete;
public:
    const PositionI position;
    const DecoratorDescriptorPointer descriptor;
protected:
    DecoratorInstance(PositionI position, DecoratorDescriptorPointer descriptor)
        : position(position), descriptor(descriptor)
    {
        assert(descriptor != nullptr);
    }
public:
    virtual ~DecoratorInstance() = default;
    virtual void generateInChunk(PositionI chunkBasePosition, WorldLockManager &lock_manager, World &world,
                                 BlocksGenerateArray &blocks) const = 0;
};

class DecoratorDescriptor
{
    friend class DecoratorDescriptors_t;
    DecoratorDescriptor(const DecoratorDescriptor &) = delete;
    const DecoratorDescriptor &operator =(const DecoratorDescriptor &) = delete;
private:
    DecoratorDescriptorIndex index;
public:
    const std::wstring name;
    /** the distance (in additional chunks) to search to see if this decorator is used.
     * (ex. 0 means this decorator doesn't generate past chunk boundaries,
     * whereas 5 means that this decorator could potentially span a distance of 5 chunks
     * from the central chunk in the X and Z directions.)
     */
    const int chunkSearchDistance;
    const float priority;
    DecoratorDescriptorIndex getIndex() const
    {
        return index;
    }
    virtual ~DecoratorDescriptor() = default;
    std::uint32_t getInitialDecoratorGenerateNumber() const
    {
        std::uint32_t retval = 0;
        for(wchar_t ch : name)
        {
            retval *= 8191;
            retval += ch;
        }
        return retval;
    }
protected:
    DecoratorDescriptor(std::wstring name, int chunkSearchDistance, float priority);
public:
    /** @brief create a DecoratorInstance for this decorator in a chunk
     *
     * @param chunkBasePosition the base position of the chunk to generate in
     * @param columnBasePosition the base position of the column to generate in
     * @param surfacePosition the surface position of the column to generate in
     * @param lock_manager the WorldLockManager
     * @param chunkBaseIterator a BlockIterator to chunkBasePosition
     * @param blocks the blocks for this chunk
     * @param randomSource the RandomSource
     * @param generateNumber a number that is different for each decorator in a chunk (use for picking a different position each time)
     * @return the new DecoratorInstance or nullptr
     *
     */
    virtual std::shared_ptr<const DecoratorInstance> createInstance(PositionI chunkBasePosition, PositionI columnBasePosition, PositionI surfacePosition,
                                 WorldLockManager &lock_manager, BlockIterator chunkBaseIterator,
                                 const BlocksGenerateArray &blocks,
                                 RandomSource &randomSource, std::uint32_t generateNumber) const = 0;
};
}
}

#endif // DECORATOR_H_INCLUDED
