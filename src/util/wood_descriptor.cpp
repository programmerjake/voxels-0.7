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
#include "entity/builtin/items/wood.h"
#include "item/builtin/wood.h"
#include "world/world.h"
#include "recipe/recipe.h"

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

namespace Woods
{
namespace builtin
{
namespace
{
class WoodToPlanksRecipe final : public RecipeDescriptor
{
private:
    WoodDescriptorPointer woodDescriptor;
public:
    WoodToPlanksRecipe(WoodDescriptorPointer woodDescriptor)
        : RecipeDescriptor(), woodDescriptor(woodDescriptor)
    {
    }
    virtual bool matches(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.recipeBlock.good())
            return false;
        unsigned filledSlotCount = 0;
        const unsigned targetFilledSlotCount = 1;
        for(const auto &i : input.items)
        {
            for(const Item &item : i)
            {
                if(!item.good())
                    continue;
                if(item.descriptor != woodDescriptor->getLogItemDescriptor())
                    return false;
                filledSlotCount++;
                if(filledSlotCount > targetFilledSlotCount)
                    return false;
            }
        }
        if(filledSlotCount != targetFilledSlotCount)
            return false;
        const unsigned outputCount = 1;
        output = RecipeOutput(ItemStack(Item(woodDescriptor->getPlanksItemDescriptor()), outputCount));
        return true;
    }
};
}
SimpleWood::SimpleWood(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves, TextureDescriptor blockedLeaves, std::vector<TreeDescriptorPointer> trees)
    : WoodDescriptor(name, logTop, logSide, planks, sapling, leaves, blockedLeaves, trees)
{
    new WoodToPlanksRecipe(this);
    enum_array<BlockDescriptorPointer, LogOrientation> logBlockDescriptors;
    BlockDescriptorPointer planksBlockDescriptor = nullptr;
    BlockDescriptorPointer saplingBlockDescriptor = nullptr;
    enum_array<BlockDescriptorPointer, bool> leavesBlockDescriptors;
    ItemDescriptorPointer logItemDescriptor = nullptr;
    ItemDescriptorPointer planksItemDescriptor = nullptr;
    ItemDescriptorPointer saplingItemDescriptor = nullptr;
    ItemDescriptorPointer leavesItemDescriptor = nullptr;
    const Entities::builtin::EntityItem *logEntityDescriptor = nullptr;
    const Entities::builtin::EntityItem *planksEntityDescriptor = nullptr;
    const Entities::builtin::EntityItem *saplingEntityDescriptor = nullptr;
    const Entities::builtin::EntityItem *leavesEntityDescriptor = nullptr;
    for(LogOrientation logOrientation : enum_traits<LogOrientation>())
    {
        logBlockDescriptors[logOrientation] = new Blocks::builtin::WoodLog(this, logOrientation);
    }
    planksBlockDescriptor = new Blocks::builtin::WoodPlanks(this);
    for(bool canDecay : enum_traits<bool>())
    {
        leavesBlockDescriptors[canDecay] = new Blocks::builtin::WoodLeaves(this, canDecay);
    }
    logEntityDescriptor = new Entities::builtin::items::WoodLog(this);
    planksEntityDescriptor = new Entities::builtin::items::WoodPlanks(this);
    leavesEntityDescriptor = new Entities::builtin::items::WoodLeaves(this);
    logItemDescriptor = new Items::builtin::WoodLog(this, logBlockDescriptors[LogOrientation::Y], logEntityDescriptor);
    planksItemDescriptor = new Items::builtin::WoodPlanks(this, planksBlockDescriptor, planksEntityDescriptor);
    leavesItemDescriptor = new Items::builtin::WoodLeaves(this, leavesBlockDescriptors[false], leavesEntityDescriptor);
    setDescriptors(logBlockDescriptors,
                   planksBlockDescriptor,
                   saplingBlockDescriptor,
                   leavesBlockDescriptors,
                   logItemDescriptor,
                   planksItemDescriptor,
                   saplingItemDescriptor,
                   leavesItemDescriptor,
                   logEntityDescriptor,
                   planksEntityDescriptor,
                   saplingEntityDescriptor,
                   leavesEntityDescriptor);
}
}
}
}
}
