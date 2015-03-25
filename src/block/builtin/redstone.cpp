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
#include "block/builtin/redstone.h"
#include "item/builtin/minerals.h"
#include "item/builtin/torch.h"
#include "entity/builtin/particles/smoke.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{

void RedstoneDust::onReplace(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneDust::descriptor())), bi.position() + VectorF(0.5));
}

void RedstoneDust::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneDust::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void RedstoneDust::onDisattach(BlockUpdateSet &blockUpdateSet, World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind blockUpdateKind) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneDust::descriptor())), blockIterator.position() + VectorF(0.5));
    blockUpdateSet.emplace_back(blockIterator.position(), Block(Air::descriptor()));
}

void RedstoneTorch::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneTorch::descriptor())), bi.position() + VectorF(0.5));
    handleToolDamage(tool);
}

void RedstoneTorch::onReplace(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneTorch::descriptor())), bi.position() + VectorF(0.5));
}

void RedstoneTorch::onDisattach(BlockUpdateSet &blockUpdateSet, World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind blockUpdateKind) const
{
    ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneTorch::descriptor())), blockIterator.position() + VectorF(0.5));
    blockUpdateSet.emplace_back(blockIterator.position(), Block(Air::descriptor()));
}

void RedstoneTorch::generateParticles(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, double currentTime, double deltaTime) const
{
    const double generateSmokePerSecond = 1.0;
    double nextTime = currentTime + deltaTime;
    double currentSmokeCount = std::floor(currentTime * generateSmokePerSecond);
    double nextSmokeCount = std::floor(nextTime * generateSmokePerSecond);
    int generateSmokeCount = limit<int>((int)(nextSmokeCount - currentSmokeCount), 0, 10);
    for(int i = 0; i < generateSmokeCount; i++)
    {
        Entities::builtin::particles::Smoke::addToWorld(world, lock_manager, bi.position() + headPosition);
    }
}

}
}

namespace Items
{
namespace builtin
{

Item RedstoneDust::onUse(Item item, World &world, WorldLockManager &lock_manager, Player &player) const
{
    RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
    if(c.valid() && c.type == RayCasting::Collision::Type::Block)
    {
        BlockIterator bi = world.getBlockIterator(c.blockPosition);
        Block b = Block(Blocks::builtin::RedstoneDust::calcOrientationAndSignalStrength(bi, lock_manager));
        if(player.placeBlock(c, world, lock_manager, b))
            return getAfterPlaceItem();
    }
    return item;
}

}
}
}
}
