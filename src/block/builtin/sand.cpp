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

#include "item/builtin/sand.h"
#include "block/builtin/sand.h"
#include "entity/builtin/falling_sand.h"
#include "render/generate.h"
#include "texture/texture_atlas.h"
#include "item/builtin/minerals.h" // flint

namespace programmerjake
{
namespace voxels
{

namespace Blocks
{
namespace builtin
{
void Sand::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Sand::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}
Entity *Sand::makeFallingBlockEntity(World &world, PositionF position, WorldLockManager &lock_manager) const
{
    return world.addEntity(Entities::builtin::FallingSand::descriptor(), position, VectorF(0), lock_manager);
}
void Gravel::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        if(std::uniform_int_distribution<int>(0, 9)(world.getRandomGenerator()) <= 0)
            ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Flint::descriptor())), bi.position() + VectorF(0.5));
        else
            ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Gravel::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}
Entity *Gravel::makeFallingBlockEntity(World &world, PositionF position, WorldLockManager &lock_manager) const
{
    return world.addEntity(Entities::builtin::FallingGravel::descriptor(), position, VectorF(0), lock_manager);
}
}
}

namespace Items
{
namespace builtin
{
Sand::Sand()
    : ItemBlock(L"builtin.sand", TextureAtlas::Sand.td(), Blocks::builtin::Sand::descriptor())
{
}
Gravel::Gravel()
    : ItemBlock(L"builtin.gravel", TextureAtlas::Gravel.td(), Blocks::builtin::Gravel::descriptor())
{
}
}
}

namespace Entities
{
namespace builtin
{
Block FallingSand::getPlacedBlock() const
{
    return Block(Blocks::builtin::Sand::descriptor());
}
Item FallingSand::getDroppedItem() const
{
    return Item(Items::builtin::Sand::descriptor());
}
FallingSand::FallingSand()
    : FallingBlock(L"builtin.falling_sand", TextureAtlas::Sand.td())
{
}
Block FallingGravel::getPlacedBlock() const
{
    return Block(Blocks::builtin::Gravel::descriptor());
}
Item FallingGravel::getDroppedItem() const
{
    return Item(Items::builtin::Gravel::descriptor());
}
FallingGravel::FallingGravel()
    : FallingBlock(L"builtin.falling_gravel", TextureAtlas::Gravel.td())
{
}

}
}

}
}
