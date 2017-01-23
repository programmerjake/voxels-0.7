/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef COBBLESTONE_SPIKE_H_INCLUDED
#define COBBLESTONE_SPIKE_H_INCLUDED

#include "generate/decorator.h"
#include "generate/decorator/pregenerated_instance.h"
#include "block/builtin/cobblestone.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Decorators
{
namespace builtin
{
class CobblestoneSpikeDecorator : public DecoratorDescriptor
{
    friend class global_instance_maker<CobblestoneSpikeDecorator>;

private:
    CobblestoneSpikeDecorator() : DecoratorDescriptor(L"builtin.cobblestone_spike", 1, 1000)
    {
    }

public:
    static const CobblestoneSpikeDecorator *pointer()
    {
        return global_instance_maker<CobblestoneSpikeDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
    /** @brief create a DecoratorInstance for this decorator in a chunk
     *
     * @param chunkBasePosition the base position of the chunk to generate in
     * @param columnBasePosition the base position of the column to generate in
     * @param surfacePosition the surface position of the column to generate in
     * @param lock_manager the WorldLockManager
     * @param chunkBaseIterator a BlockIterator to chunkBasePosition
     * @param blocks the blocks for this chunk
     * @param randomSource the RandomSource
     * @param generateNumber a number that is different for each decorator in a chunk (use for
     *picking a different position each time)
     * @return the new DecoratorInstance or nullptr
     *
     */
    virtual std::shared_ptr<const DecoratorInstance> createInstance(
        PositionI chunkBasePosition,
        PositionI columnBasePosition,
        PositionI surfacePosition,
        WorldLockManager &lock_manager,
        BlockIterator chunkBaseIterator,
        const BlocksGenerateArray &blocks,
        RandomSource &randomSource,
        std::uint32_t generateNumber) const override
    {
        std::shared_ptr<PregeneratedDecoratorInstance> retval =
            std::make_shared<PregeneratedDecoratorInstance>(
                surfacePosition, this, VectorI(-5, 0, -5), VectorI(11, 10, 11));
        for(int i = 0; i < 5; i++)
        {
            retval->setBlock(VectorI(0, i, 0), Block(Blocks::builtin::Cobblestone::descriptor()));
        }
        for(int i = 5; i < 10; i++)
        {
            retval->setBlock(VectorI(5 - i, i, 0),
                             Block(Blocks::builtin::Cobblestone::descriptor()));
            retval->setBlock(VectorI(i - 5, i, 0),
                             Block(Blocks::builtin::Cobblestone::descriptor()));
            retval->setBlock(VectorI(0, i, 5 - i),
                             Block(Blocks::builtin::Cobblestone::descriptor()));
            retval->setBlock(VectorI(0, i, i - 5),
                             Block(Blocks::builtin::Cobblestone::descriptor()));
        }
        return retval;
    }
};
}
}
}
}

#endif // COBBLESTONE_SPIKE_H_INCLUDED
