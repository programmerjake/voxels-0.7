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
#include "block/builtin/grass.h"
#include "block/builtin/dirt.h"
#include "item/builtin/dirt.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
void Grass::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Dirt::descriptor())), bi.position() + VectorF(0.5));
    handleToolDamage(tool);
}
void Grass::randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const
{
    BlockIterator bi = blockIterator;
    bi.moveBy(VectorI(0, 1, 0));
    Block b = bi.get(lock_manager);
    if(b.lighting.toFloat(world.getLighting(bi.position().d)) >= 4.0f / 15 && b.descriptor->lightProperties.isTotallyTransparent())
        return;
    world.setBlock(blockIterator, lock_manager, Block(Dirt::descriptor()));
}
}
}
}
}
