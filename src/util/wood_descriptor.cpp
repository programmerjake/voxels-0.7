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
#include "item/builtin/chest.h"

namespace programmerjake
{
namespace voxels
{
bool Tree::placeInWorld(PositionI generatePosition, World &world, WorldLockManager &lock_manager) const
{
    if(!good())
        return false;
    Tree placedTree(*this);
    BlockIterator originBlockIterator = world.getBlockIterator(generatePosition, lock_manager.tls);
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
                    bi.moveBy(rpos, lock_manager.tls);
                    Block worldBlock = bi.get(lock_manager);
                    Block selectedBlock = descriptor->selectBlock(worldBlock, placedTree.getBlock(rpos), true);
                    selectedBlock.lighting = worldBlock.lighting;
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
                bi.moveBy(rpos, lock_manager.tls);
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

void WoodDescriptor::makeLeavesDrops(World &world, BlockIterator bi, WorldLockManager &lock_manager, Item tool) const
{
    FIXME_MESSAGE(check for shears)
    if(std::uniform_int_distribution<>(0, 19)(world.getRandomGenerator()) == 0)
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(getSaplingItemDescriptor())), bi.position() + VectorF(0.5));
    }
}

linked_map<std::wstring, WoodDescriptorPointer> *WoodDescriptors_t::pNameMap = nullptr;

namespace Recipes
{
namespace builtin
{
class CraftingTableRecipe : PatternRecipe<2, 2>
{
    CraftingTableRecipe(CraftingTableRecipe &) = delete;
    CraftingTableRecipe &operator =(CraftingTableRecipe &) = delete;
private:
    WoodDescriptorPointer woodDescriptor;
public:
    CraftingTableRecipe(WoodDescriptorPointer woodDescriptor)
        : PatternRecipe(checked_array<Item, 2 * 2>
                        {
                            Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor()),
                            Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor())
                        }), woodDescriptor(woodDescriptor)
    {
    }
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::CraftingTable::descriptor()), 1));
        return true;
    }
};

class ChestRecipe : PatternRecipe<3, 3>
{
    ChestRecipe(ChestRecipe &) = delete;
    ChestRecipe &operator =(ChestRecipe &) = delete;
private:
    WoodDescriptorPointer woodDescriptor;
public:
    ChestRecipe(WoodDescriptorPointer woodDescriptor)
        : PatternRecipe(checked_array<Item, 3 * 3>
                        {
                            Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor()),
                            Item(woodDescriptor->getPlanksItemDescriptor()), Item(), Item(woodDescriptor->getPlanksItemDescriptor()),
                            Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor()), Item(woodDescriptor->getPlanksItemDescriptor())
                        }), woodDescriptor(woodDescriptor)
    {
    }
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::Chest::descriptor()), 1));
        return true;
    }
};

class WoodToPlanksRecipe final : public UnorderedRecipe
{
    WoodToPlanksRecipe(WoodToPlanksRecipe &) = delete;
    WoodToPlanksRecipe &operator =(WoodToPlanksRecipe &) = delete;
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
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(woodDescriptor->getPlanksItemDescriptor()), 4));
        return true;
    }
};

class PlanksToSticksRecipe final : public PatternRecipe<1, 2>
{
    PlanksToSticksRecipe(PlanksToSticksRecipe &) = delete;
    PlanksToSticksRecipe &operator =(PlanksToSticksRecipe &) = delete;
private:
    WoodDescriptorPointer woodDescriptor;
public:
    PlanksToSticksRecipe(WoodDescriptorPointer woodDescriptor)
        : PatternRecipe(checked_array<Item, 1 * 2>
                        {
                            Item(woodDescriptor->getPlanksItemDescriptor()),
                            Item(woodDescriptor->getPlanksItemDescriptor())
                        }), woodDescriptor(woodDescriptor)
    {
    }
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
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
    checked_array<BlockDescriptorPointer, 2> saplingBlockDescriptors;
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
    for(unsigned i = 0; i < saplingBlockDescriptors.size(); i++)
    {
        saplingBlockDescriptors[i] = new Blocks::builtin::Sapling(this, i);
    }
    logItemDescriptor = new Items::builtin::WoodLog(this, logBlockDescriptors[LogOrientation::Y]);
    planksItemDescriptor = new Items::builtin::WoodPlanks(this, planksBlockDescriptor);
    leavesItemDescriptor = new Items::builtin::WoodLeaves(this, leavesBlockDescriptors[false]);
    saplingItemDescriptor = new Items::builtin::Sapling(this, saplingBlockDescriptors[0]);
    setDescriptors(logBlockDescriptors,
                   planksBlockDescriptor,
                   saplingBlockDescriptors,
                   leavesBlockDescriptors,
                   logItemDescriptor,
                   planksItemDescriptor,
                   saplingItemDescriptor,
                   leavesItemDescriptor);
    new Recipes::builtin::WoodToPlanksRecipe(this);
    new Recipes::builtin::PlanksToSticksRecipe(this);
    new Recipes::builtin::CraftingTableRecipe(this);
    new Recipes::builtin::ChestRecipe(this);
}
}
}
}
}
