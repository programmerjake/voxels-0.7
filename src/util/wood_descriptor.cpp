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
#include "util/wood_descriptor.h"
#include "block/builtin/wood.h"
#include "entity/builtin/item.h"
#include "item/builtin/wood.h"
#include "world/world.h"
#include "recipe/builtin/unordered.h"
#include "recipe/builtin/pattern.h"
#include "item/builtin/crafting_table.h"

namespace programmerjake
{
namespace voxels
{
bool Tree::placeInWorld(PositionI generatePosition, World &world, WorldLockManager &lock_manager) const
{
    if(!good())
        return false;
    Tree placedTree(*this);
    BlockIterator originBlockIterator = world.getBlockIterator(generatePosition);
    try
    {
        for(VectorI rpos = getArrayMin(); rpos.x < getArrayEnd().x; rpos.x++)
        {
            for(rpos.y = getArrayMin().y; rpos.y < getArrayEnd().y; rpos.y++)
            {
                for(rpos.z = getArrayMin().z; rpos.z < getArrayEnd().z; rpos.z++)
                {
                    if(!placedTree.getBlock(rpos).good())
                        continue;
                    BlockIterator bi = originBlockIterator;
                    bi.moveBy(rpos);
                    Block worldBlock = bi.get(lock_manager);
                    Block selectedBlock = descriptor->selectBlock(worldBlock, placedTree.getBlock(rpos), true);
                    placedTree.setBlock(rpos, selectedBlock);
                }
            }
        }
    }
    catch(TreeDoesNotFitException &)
    {
        return false;
    }
    for(VectorI rpos = getArrayMin(); rpos.x < getArrayEnd().x; rpos.x++)
    {
        for(rpos.y = getArrayMin().y; rpos.y < getArrayEnd().y; rpos.y++)
        {
            for(rpos.z = getArrayMin().z; rpos.z < getArrayEnd().z; rpos.z++)
            {
                Block b = placedTree.getBlock(rpos);
                if(!b.good())
                    continue;
                BlockIterator bi = originBlockIterator;
                bi.moveBy(rpos);
                world.setBlock(bi, lock_manager, b);
            }
        }
    }
    return true;
}

Block TreeDescriptor::selectBlock(Block originalWorldBlock, Block treeBlock, bool canThrow) const
{
    if(!treeBlock.good())
        return Block();
    if(!originalWorldBlock.good() || originalWorldBlock.descriptor->isGroundBlock())
    {
        if(!canThrow)
            return Block();
        throw TreeDoesNotFitException();
    }
    return treeBlock;
}


linked_map<std::wstring, WoodDescriptorPointer> *WoodDescriptors_t::pNameMap = nullptr;

namespace Recipes
{
namespace builtin
{
class CraftingTableRecipe : PatternRecipe<2, 2>
{
private:
    WoodDescriptorPointer woodDescriptor;
public:
    CraftingTableRecipe(WoodDescriptorPointer woodDescriptor)
        : PatternRecipe(checked_array<checked_array<Item, 2>, 2>
                        {
                            Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor()),
                            Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor())
                        }), woodDescriptor(woodDescriptor)
    {
    }
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && input.getRecipeBlock().descriptor != Items::builtin::CraftingTable::descriptor())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::CraftingTable::descriptor()), 1));
        return true;
    }
};

class WoodToPlanksRecipe final : public UnorderedRecipe
{
private:
    WoodDescriptorPointer woodDescriptor;
public:
    WoodToPlanksRecipe(WoodDescriptorPointer woodDescriptor)
        : UnorderedRecipe{std::pair<Item, std::size_t>(Item(woodDescriptor->getLogItemDescriptor()), 1)}, woodDescriptor(woodDescriptor)
    {
    }
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && input.getRecipeBlock().descriptor != Items::builtin::CraftingTable::descriptor())
            return false;
        output = RecipeOutput(ItemStack(Item(woodDescriptor->getPlanksItemDescriptor()), 4));
        return true;
    }
};

class PlanksToSticksRecipe final : public PatternRecipe<1, 2>
{
private:
    WoodDescriptorPointer woodDescriptor;
public:
    PlanksToSticksRecipe(WoodDescriptorPointer woodDescriptor)
        : PatternRecipe(checked_array<checked_array<Item, 2>, 1>
                        {
                            Item(woodDescriptor->getPlanksItemDescriptor()),
                            Item(woodDescriptor->getPlanksItemDescriptor())
                        }), woodDescriptor(woodDescriptor)
    {
    }
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && input.getRecipeBlock().descriptor != Items::builtin::CraftingTable::descriptor())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::Stick::descriptor()), 4));
        return true;
    }
};
}
}

namespace Woods
{
namespace builtin
{
SimpleWood::SimpleWood(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves, TextureDescriptor blockedLeaves, std::vector<TreeDescriptorPointer> trees)
    : WoodDescriptor(name, logTop, logSide, planks, sapling, leaves, blockedLeaves, trees)
{
    enum_array<BlockDescriptorPointer, LogOrientation> logBlockDescriptors;
    BlockDescriptorPointer planksBlockDescriptor = nullptr;
    BlockDescriptorPointer saplingBlockDescriptor = nullptr;
    enum_array<BlockDescriptorPointer, bool> leavesBlockDescriptors;
    ItemDescriptorPointer logItemDescriptor = nullptr;
    ItemDescriptorPointer planksItemDescriptor = nullptr;
    ItemDescriptorPointer saplingItemDescriptor = nullptr;
    ItemDescriptorPointer leavesItemDescriptor = nullptr;
    for(LogOrientation logOrientation : enum_traits<LogOrientation>())
    {
        logBlockDescriptors[logOrientation] = new Blocks::builtin::WoodLog(this, logOrientation);
    }
    planksBlockDescriptor = new Blocks::builtin::WoodPlanks(this);
    for(bool canDecay : enum_traits<bool>())
    {
        leavesBlockDescriptors[canDecay] = new Blocks::builtin::WoodLeaves(this, canDecay);
    }
    logItemDescriptor = new Items::builtin::WoodLog(this, logBlockDescriptors[LogOrientation::Y]);
    planksItemDescriptor = new Items::builtin::WoodPlanks(this, planksBlockDescriptor);
    leavesItemDescriptor = new Items::builtin::WoodLeaves(this, leavesBlockDescriptors[false]);
    setDescriptors(logBlockDescriptors,
                   planksBlockDescriptor,
                   saplingBlockDescriptor,
                   leavesBlockDescriptors,
                   logItemDescriptor,
                   planksItemDescriptor,
                   saplingItemDescriptor,
                   leavesItemDescriptor);
    new Recipes::builtin::WoodToPlanksRecipe(this);
    new Recipes::builtin::PlanksToSticksRecipe(this);
    new Recipes::builtin::CraftingTableRecipe(this);
}
}
}
}
}
