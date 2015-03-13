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
#ifndef RECIPE_H_INCLUDED
#define RECIPE_H_INCLUDED

#include "item/item.h"
#include <vector>

namespace programmerjake
{
namespace voxels
{
class RecipeDescriptor;
typedef const RecipeDescriptor *RecipeDescriptorPointer;

struct Recipe final
{
    RecipeDescriptorPointer descriptor;
    constexpr Recipe(RecipeDescriptorPointer descriptor = nullptr)
        : descriptor(descriptor)
    {
    }
    constexpr bool good() const
    {
        return descriptor != nullptr;
    }
};

struct RecipeInput final
{
    ItemStackArray<3, 3> itemStacks;
    Item recipeBlock; // ex. crafting table, furnace, or nothing for inventory
};

struct RecipeOutput final
{
    ItemStackArray<3, 3> itemStacks;
    ItemStack output;
    bool good() const
    {
        return output.good();
    }
    RecipeOutput()
    {
    }
    RecipeOutput(ItemStackArray<3, 3> itemStacks, ItemStack output)
        : itemStacks(itemStacks), output(output)
    {
    }
};

class RecipeDescriptor
{
    RecipeDescriptor(const RecipeDescriptor &) = delete;
    const RecipeDescriptor &operator =(const RecipeDescriptor &) = delete;
protected:
    RecipeDescriptor();
public:
    virtual ~RecipeDescriptor() = default;
    virtual bool matches(const RecipeInput &input, RecipeOutput &output) const = 0;
};

class RecipeDescriptors_t final
{
    friend class RecipeDescriptor;
private:
    typedef std::vector<RecipeDescriptorPointer> ListType;
    static ListType *list;
    static void add(RecipeDescriptorPointer descriptor)
    {
        if(list == nullptr)
        {
            list = new ListType;
        }
        list->push_back(descriptor);
    }
public:
    typedef typename ListType::const_iterator iterator;
    iterator begin() const
    {
        assert(list != nullptr);
        return list->cbegin();
    }
    iterator end() const
    {
        assert(list != nullptr);
        return list->cend();
    }
    std::size_t size() const
    {
        assert(list != nullptr);
        return list->size();
    }
    bool getFirstMatch(const RecipeInput &input, RecipeOutput &output) const
    {
        for(RecipeDescriptorPointer descriptor : *this)
        {
            if(descriptor->matches(input, output))
                return true;
        }
        return false;
    }
};

static constexpr RecipeDescriptors_t RecipeDescriptors{};

inline RecipeDescriptor::RecipeDescriptor()
{
    RecipeDescriptors.add(this);
}
}
}

#endif // RECIPE_H_INCLUDED
