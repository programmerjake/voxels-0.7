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
#ifndef UNORDERED_RECIPE_H_INCLUDED
#define UNORDERED_RECIPE_H_INCLUDED

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
class UnorderedRecipe : public RecipeDescriptor
{
protected:
    linked_map<Item, std::size_t> itemCounts;
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const = 0;
public:
    UnorderedRecipe(std::initializer_list<std::pair<Item, std::size_t>> elements)
        : RecipeDescriptor()
    {
        for(const std::pair<Item, std::size_t> &v : elements)
        {
            itemCounts[std::get<0>(v)] = std::get<1>(v);
        }
    }
    virtual bool matches(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getItemCounts().size() != itemCounts.size())
            return false;
        for(const std::pair<Item, std::size_t> &v : itemCounts)
        {
            auto iter = input.getItemCounts().find(std::get<0>(v));
            if(iter == input.getItemCounts().end())
                return false;
            if(std::get<1>(v) != std::get<1>(*iter))
                return false;
        }
        return fillOutput(input, output);
    }
};
}
}
}
}


#endif // UNORDERED_RECIPE_H_INCLUDED
