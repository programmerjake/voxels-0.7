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
#ifndef PATTERN_RECIPE_H_INCLUDED
#define PATTERN_RECIPE_H_INCLUDED

#include "recipe/recipe.h"
#include "util/linked_map.h"
#include <initializer_list>
#include <tuple>

namespace programmerjake
{
namespace voxels
{
namespace Recipes
{
namespace builtin
{
template <std::size_t W, std::size_t H>
class PatternRecipe : public RecipeDescriptor
{
    static_assert(W <= RecipeInput::width && H <= RecipeInput::height, "Pattern too big");
protected:
    checked_array<Item, W * H> pattern;
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const = 0;
    virtual bool itemsMatch(const Item &patternItem, const Item &inputItem) const
    {
        return patternItem == inputItem;
    }
public:
    PatternRecipe(checked_array<Item, W * H> pattern)
        : RecipeDescriptor(), pattern(pattern)
    {
    }
    virtual bool matches(const RecipeInput &input, RecipeOutput &output) const override
    {
        for(std::size_t x = 0; x < RecipeInput::width; x++)
        {
            for(std::size_t y = 0; y < RecipeInput::height; y++)
            {
                Item patternItem = Item();
                if(x < W && y < H)
                {
                    patternItem = pattern[x + y * W];
                }
                if(!itemsMatch(patternItem, input.getItems()[x][y]))
                    return false;
            }
        }
        return fillOutput(input, output);
    }
};
}
}
}
}

#endif // PATTERN_RECIPE_H_INCLUDED
