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
#include "util/checked_array.h"
#include "util/linked_map.h"

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
    static constexpr std::size_t width = 3, height = 3;
    typedef checked_array<checked_array<Item, height>, width> ItemsArrayType;
private:
    ItemsArrayType items;
    Item recipeBlock; // ex. crafting table, furnace, or nothing for inventory
    linked_map<Item, std::size_t> itemCounts;
public:
    const ItemsArrayType &getItems() const
    {
        return items;
    }
    const Item &getRecipeBlock() const
    {
        return recipeBlock;
    }
    const linked_map<Item, std::size_t> &getItemCounts() const
    {
        return itemCounts;
    }
    RecipeInput()
        : items(), recipeBlock(), itemCounts()
    {
    }
    RecipeInput(const ItemsArrayType &itemsIn, Item recipeBlock)
        : items(itemsIn), recipeBlock(recipeBlock), itemCounts()
    {
        for(std::size_t x = 0; x < width; x++)
        {
            for(std::size_t y = 0; y < height; y++)
            {
                if(items[x][y].good())
                {
                    auto iter = itemCounts.find(items[x][y]);
                    if(iter == itemCounts.end())
                    {
                        itemCounts[items[x][y]] = 1;
                    }
                    else
                    {
                        std::get<1>(*iter)++;
                    }
                }
            }
        }
        if(empty())
            return;
        for(std::size_t x = 0; x < width; x++)
        {
            bool canShift = true;
            for(std::size_t y = 0; y < height; y++)
            {
                if(items[0][y].good())
                {
                    canShift = false;
                    break;
                }
            }
            if(canShift)
                shiftLeft();
            else
                break;
        }
        for(std::size_t y = 0; y < height; y++)
        {
            bool canShift = true;
            for(std::size_t x = 0; x < width; x++)
            {
                if(items[x][0].good())
                {
                    canShift = false;
                    break;
                }
            }
            if(canShift)
                shiftUp();
            else
                break;
        }
    }
private:
    void shiftLeft()
    {
        for(std::size_t x = 0; x < width - 1; x++)
        {
            for(std::size_t y = 0; y < height; y++)
            {
                items[x][y] = std::move(items[x + 1][y]);
            }
        }
        const std::size_t x = width - 1;
        for(std::size_t y = 0; y < height; y++)
        {
            items[x][y] = Item();
        }
    }
    void shiftUp()
    {
        for(std::size_t x = 0; x < width; x++)
        {
            for(std::size_t y = 0; y < height - 1; y++)
            {
                items[x][y] = std::move(items[x][y + 1]);
            }
            const std::size_t y = height - 1;
            items[x][y] = Item();
        }
    }
public:
    bool empty() const
    {
        return itemCounts.empty();
    }
};

struct RecipeOutput final
{
    checked_array<checked_array<Item, 3>, 3> items;
    ItemStack output;
    bool good() const
    {
        return output.good();
    }
    RecipeOutput()
        : items(), output()
    {
    }
    RecipeOutput(checked_array<checked_array<Item, 3>, 3> items, ItemStack output)
        : items(items), output(output)
    {
    }
    RecipeOutput(ItemStack output)
        : items(), output(output)
    {
    }
    void clear()
    {
        *this = RecipeOutput();
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
    typedef ListType::const_iterator iterator;
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
