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
#include "block/builtin/ladder.h"
#include "item/builtin/ladder.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{

void Ladder::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Ladder::descriptor())), bi.position() + VectorF(0.5));
    handleToolDamage(tool);
}

void Ladder::onReplace(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Ladder::descriptor())), bi.position() + VectorF(0.5));
}

void Ladder::onDisattach(World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind blockUpdateKind) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Ladder::descriptor())), blockIterator.position() + VectorF(0.5));
    world.setBlock(blockIterator, lock_manager, Block(Air::descriptor(), block.lighting));
}

}
}
}
}
