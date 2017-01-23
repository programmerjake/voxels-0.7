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
#include "block/builtin/ladder.h"
#include "item/builtin/ladder.h"
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
class LadderRecipe final : public PatternRecipe<3, 3>
{
    friend class global_instance_maker<LadderRecipe>;

protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::Ladder::descriptor()), 3));
        return true;
    }

private:
    LadderRecipe()
        : PatternRecipe(checked_array<Item, 3 * 3>{
              Item(Items::builtin::Stick::descriptor()),
              Item(),
              Item(Items::builtin::Stick::descriptor()),
              Item(Items::builtin::Stick::descriptor()),
              Item(Items::builtin::Stick::descriptor()),
              Item(Items::builtin::Stick::descriptor()),
              Item(Items::builtin::Stick::descriptor()),
              Item(),
              Item(Items::builtin::Stick::descriptor()),
          })
    {
    }

public:
    static const LadderRecipe *pointer()
    {
        return global_instance_maker<LadderRecipe>::getInstance();
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
