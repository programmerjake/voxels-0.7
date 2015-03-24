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
#include "block/builtin/ores.h"
#include "item/builtin/ores.h"
#include "item/builtin/minerals.h"
#include "texture/texture_atlas.h"
#include "world/world.h"
#include <random>

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{

void CoalOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Coal::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void IronOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::IronOre::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void LapisLazuliOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        int dropCount = std::uniform_int_distribution<int>(4, 8)(world.getRandomGenerator());
        for(int i = 0; i < dropCount; i++)
            ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::LapisLazuli::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void GoldOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::GoldOre::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void DiamondOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Diamond::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void RedstoneOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        int dropCount = std::uniform_int_distribution<int>(4, 5)(world.getRandomGenerator());
        for(int i = 0; i < dropCount; i++)
            ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneDust::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

bool RedstoneOre::onUse(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const
{
    world.setBlock(bi, lock_manager, Block(LitRedstoneOre::descriptor()));
    return StoneBlock::onUse(world, b, bi, lock_manager, player);
}

bool RedstoneOre::onStartAttack(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const
{
    world.setBlock(bi, lock_manager, Block(LitRedstoneOre::descriptor()));
    return true;
}

void LitRedstoneOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        int dropCount = std::uniform_int_distribution<int>(4, 5)(world.getRandomGenerator());
        for(int i = 0; i < dropCount; i++)
            ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::RedstoneDust::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

void LitRedstoneOre::randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const
{
    world.setBlock(blockIterator, lock_manager, Block(RedstoneOre::descriptor()));
}

void EmeraldOre::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Emerald::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}

}
}
}
}
