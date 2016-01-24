/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
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
#include "block/builtin/torch.h"
#include "item/builtin/torch.h"
#include "item/builtin/minerals.h" // coal redstone
#include "item/builtin/wood.h" // stick
#include "recipe/builtin/pattern.h"

namespace programmerjake
{
namespace voxels
{
namespace Recipes
{
namespace builtin
{

class TorchRecipe final : public PatternRecipe<1, 2>
{
    friend class global_instance_maker<TorchRecipe>;
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::Torch::descriptor()), 4));
        return true;
    }
private:
    TorchRecipe()
        : PatternRecipe(checked_array<Item, 1 * 2>
        {
            Item(Items::builtin::Coal::descriptor()),
            Item(Items::builtin::Stick::descriptor()),
        })
    {
    }
public:
    static const TorchRecipe *pointer()
    {
        return global_instance_maker<TorchRecipe>::getInstance();
    }
    static RecipeDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class TorchRecipe2 final : public PatternRecipe<1, 2>
{
    friend class global_instance_maker<TorchRecipe2>;
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::Torch::descriptor()), 4));
        return true;
    }
private:
    TorchRecipe2()
        : PatternRecipe(checked_array<Item, 1 * 2>
        {
            Item(Items::builtin::Charcoal::descriptor()),
            Item(Items::builtin::Stick::descriptor()),
        })
    {
    }
public:
    static const TorchRecipe2 *pointer()
    {
        return global_instance_maker<TorchRecipe2>::getInstance();
    }
    static RecipeDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class RedstoneTorchRecipe final : public PatternRecipe<1, 2>
{
    friend class global_instance_maker<RedstoneTorchRecipe>;
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::RedstoneTorch::descriptor()), 1));
        return true;
    }
private:
    RedstoneTorchRecipe()
        : PatternRecipe(checked_array<Item, 1 * 2>
        {
            Item(Items::builtin::RedstoneDust::descriptor()),
            Item(Items::builtin::Stick::descriptor()),
        })
    {
    }
public:
    static const RedstoneTorchRecipe *pointer()
    {
        return global_instance_maker<RedstoneTorchRecipe>::getInstance();
    }
    static RecipeDescriptorPointer descriptor()
    {
        return pointer();
    }
};

}
}
}
}
