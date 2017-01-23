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
#include "block/builtin/redstone.h"
#include "item/builtin/minerals.h"
#include "item/builtin/torch.h"
#include "entity/builtin/particles/redstone.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
void RedstoneDust::onReplace(World &world,
                             Block b,
                             BlockIterator bi,
                             WorldLockManager &lock_manager) const
{
    ItemDescriptor::addToWorld(world,
                               lock_manager,
                               ItemStack(Item(Items::builtin::RedstoneDust::descriptor())),
                               bi.position() + VectorF(0.5));
}

void RedstoneDust::onBreak(
    World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world,
                                   lock_manager,
                                   ItemStack(Item(Items::builtin::RedstoneDust::descriptor())),
                                   bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void RedstoneDust::onDisattach(World &world,
                               const Block &block,
                               BlockIterator blockIterator,
                               WorldLockManager &lock_manager,
                               BlockUpdateKind blockUpdateKind) const
{
    ItemDescriptor::addToWorld(world,
                               lock_manager,
                               ItemStack(Item(Items::builtin::RedstoneDust::descriptor())),
                               blockIterator.position() + VectorF(0.5));
    world.setBlock(blockIterator, lock_manager, Block(Air::descriptor(), block.lighting));
}

void RedstoneDust::generateParticles(World &world,
                                     Block b,
                                     BlockIterator bi,
                                     WorldLockManager &lock_manager,
                                     double currentTime,
                                     double deltaTime) const
{
    if(signalStrength <= 0)
        return;
    const double generateRedstonePerSecond = 1.0;
    double nextTime = currentTime + deltaTime;
    std::uint32_t v = std::hash<PositionI>()(bi.position());
    v *= 21793497;
    float timeOffset = (float)v / 6127846.0f;
    double currentRedstoneCount = std::floor(currentTime * generateRedstonePerSecond + timeOffset);
    double nextRedstoneCount = std::floor(nextTime * generateRedstonePerSecond + timeOffset);
    int generateRedstoneCount = limit<int>((int)(nextRedstoneCount - currentRedstoneCount), 0, 10);
    for(int i = 0; i < generateRedstoneCount; i++)
    {
        Entities::builtin::particles::Redstone::addToWorld(
            world, lock_manager, bi.position() + VectorF(0.5f, 0.15f, 0.5f));
    }
}

void RedstoneTorch::onBreak(
    World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    ItemDescriptor::addToWorld(world,
                               lock_manager,
                               ItemStack(Item(Items::builtin::RedstoneTorch::descriptor())),
                               bi.position() + VectorF(0.5));
    handleToolDamage(tool);
}

void RedstoneTorch::onReplace(World &world,
                              Block b,
                              BlockIterator bi,
                              WorldLockManager &lock_manager) const
{
    ItemDescriptor::addToWorld(world,
                               lock_manager,
                               ItemStack(Item(Items::builtin::RedstoneTorch::descriptor())),
                               bi.position() + VectorF(0.5));
}

void RedstoneTorch::onDisattach(World &world,
                                const Block &block,
                                BlockIterator blockIterator,
                                WorldLockManager &lock_manager,
                                BlockUpdateKind blockUpdateKind) const
{
    ItemDescriptor::addToWorld(world,
                               lock_manager,
                               ItemStack(Item(Items::builtin::RedstoneTorch::descriptor())),
                               blockIterator.position() + VectorF(0.5));
    world.setBlock(blockIterator, lock_manager, Block(Air::descriptor(), block.lighting));
}

void RedstoneTorch::generateParticles(World &world,
                                      Block b,
                                      BlockIterator bi,
                                      WorldLockManager &lock_manager,
                                      double currentTime,
                                      double deltaTime) const
{
    if(!isOn)
        return;
    const double generateRedstonePerSecond = 1.0;
    double nextTime = currentTime + deltaTime;
    std::uint32_t v = std::hash<PositionI>()(bi.position());
    v *= 21793497;
    float timeOffset = (float)v / 6127846.0f;
    double currentRedstoneCount = std::floor(currentTime * generateRedstonePerSecond + timeOffset);
    double nextRedstoneCount = std::floor(nextTime * generateRedstonePerSecond + timeOffset);
    int generateRedstoneCount = limit<int>((int)(nextRedstoneCount - currentRedstoneCount), 0, 10);
    for(int i = 0; i < generateRedstoneCount; i++)
    {
        Entities::builtin::particles::Redstone::addToWorld(
            world, lock_manager, bi.position() + headPosition);
    }
}
}
}

namespace Items
{
namespace builtin
{
Item RedstoneDust::onUse(Item item,
                         World &world,
                         WorldLockManager &lock_manager,
                         Player &player) const
{
    RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
    if(c.valid() && c.type == RayCasting::Collision::Type::Block)
    {
        BlockIterator bi = world.getBlockIterator(c.blockPosition, lock_manager.tls);
        Block b = Block(
            Blocks::builtin::RedstoneDust::calcOrientationAndSignalStrength(bi, lock_manager));
        if(player.placeBlock(c, world, lock_manager, b))
        {
            BlockDescriptor::addRedstoneBlockUpdates(world, bi, lock_manager, 2);
            return getAfterPlaceItem();
        }
    }
    return item;
}
}
}
}
}
