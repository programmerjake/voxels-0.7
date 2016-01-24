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
#ifndef INVENTORY_H_INCLUDED
#define INVENTORY_H_INCLUDED

#include "ui/player_dialog.h"
#include "texture/texture_atlas.h"
#include "recipe/recipe.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class PlayerInventory final : public PlayerDialog
{
private:
    RecipeInput recipeInput;
    ItemStackArray<2, 2> inputItemStacks;
    ItemStack output;
    RecipeOutput recipeOutput;
public:
    PlayerInventory(std::shared_ptr<Player> player)
        : PlayerDialog(player, TextureAtlas::InventoryUI.td()),
        recipeInput(),
        inputItemStacks(),
        output(),
        recipeOutput()
    {
    }
private:
    void setupRecipeOutput()
    {
        output = ItemStack();
        recipeOutput = RecipeOutput();
        RecipeInput::ItemsArrayType recipeInputItems;
        unsigned recipeCount = 0;
        bool haveRecipeCount = false;
        for(int x = 0; x < (int)inputItemStacks.itemStacks.size(); x++)
        {
            for(int y = 0; y < (int)inputItemStacks.itemStacks[0].size(); y++)
            {
                if(inputItemStacks.itemStacks[x][y].good())
                {
                    recipeInputItems[x][y] = inputItemStacks.itemStacks[x][y].item;
                    unsigned c = inputItemStacks.itemStacks[x][y].count;
                    if(recipeCount > c || !haveRecipeCount)
                        recipeCount = c;
                    haveRecipeCount = true;
                }
            }
        }
        recipeInput = RecipeInput(recipeInputItems, Item());
        if(recipeInput.empty())
            return;
        if(!RecipeDescriptors.getFirstMatch(recipeInput, recipeOutput))
        {
            recipeOutput = RecipeOutput();
        }
        else
        {
            unsigned maxRecipeCount = recipeOutput.output.getMaxCount() / recipeOutput.output.count;
            if(recipeCount > maxRecipeCount)
                recipeCount = maxRecipeCount;
            output = ItemStack(recipeOutput.output.item, recipeCount * recipeOutput.output.count);
        }
    }
protected:
    virtual unsigned transferItems(std::shared_ptr<ItemStack> sourceItemStack, std::shared_ptr<ItemStack> destItemStack, unsigned transferCount) override
    {
        if(destItemStack.get() == &output)
            return 0;
        if(sourceItemStack.get() == &output)
        {
            if(!recipeOutput.good())
                return 0;
            for(int x = 0; x < (int)inputItemStacks.itemStacks.size(); x++)
            {
                for(int y = 0; y < (int)inputItemStacks.itemStacks[0].size(); y++)
                {
                    if(destItemStack.get() == &inputItemStacks.itemStacks[x][y])
                    {
                        return 0;
                    }
                }
            }
            unsigned transferRecipeCount = (transferCount + recipeOutput.output.count - 1) / recipeOutput.output.count;
            unsigned maxRecipeCount = (recipeOutput.output.getMaxCount() > destItemStack->count)
                                        ? (recipeOutput.output.getMaxCount() - destItemStack->count) / recipeOutput.output.count
                                        : 0;
            if(transferRecipeCount > maxRecipeCount)
                transferRecipeCount = maxRecipeCount;
            transferCount = transferRecipeCount * recipeOutput.output.count;
            transferCount = destItemStack->transfer(*sourceItemStack, transferCount);
            if(transferCount == 0)
                return 0;
            for(int x = 0; x < (int)inputItemStacks.itemStacks.size(); x++)
            {
                for(int y = 0; y < (int)inputItemStacks.itemStacks[0].size(); y++)
                {
                    ItemStack destStack;
                    destStack.transfer(inputItemStacks.itemStacks[x][y], transferRecipeCount);
                }
            }
            setupRecipeOutput();
            return transferCount;
        }
        for(int x = 0; x < (int)inputItemStacks.itemStacks.size(); x++)
        {
            for(int y = 0; y < (int)inputItemStacks.itemStacks[0].size(); y++)
            {
                if(sourceItemStack.get() == &inputItemStacks.itemStacks[x][y] || destItemStack.get() == &inputItemStacks.itemStacks[x][y])
                {
                    unsigned retval = destItemStack->transfer(*sourceItemStack, transferCount);
                    setupRecipeOutput();
                    return retval;
                }
            }
        }
        return destItemStack->transfer(*sourceItemStack, transferCount);
    }
    virtual void addElements() override
    {
        std::shared_ptr<Player> player = this->player;
        ItemStack *pOutput = &output;
        auto deleter = [player, pOutput](ItemStack *itemStack)
        {
            if(itemStack != pOutput)
            {
                if(itemStack->good())
                {
                    for(std::size_t i = 0; i < itemStack->count; i++)
                    {
                        if(player->addItem(itemStack->item) < 1)
                        {
                            FIXME_MESSAGE(finish)
                        }
                    }
                }
            }
        };
        for(int x = 0; x < (int)inputItemStacks.itemStacks.size(); x++)
        {
            for(int y = 0; y < (int)inputItemStacks.itemStacks[0].size(); y++)
            {
                add(std::make_shared<UiItem>(imageGetPositionX(48 + 18 * x), imageGetPositionX(48 + 18 * x + 16),
                                             imageGetPositionY(99 + 18 * ((int)inputItemStacks.itemStacks[0].size() - y - 1)), imageGetPositionY(99 + 18 * ((int)inputItemStacks.itemStacks[0].size() - y - 1) + 16),
                                             std::shared_ptr<ItemStack>(&inputItemStacks.itemStacks[x][y], deleter)));
            }
        }
        add(std::make_shared<UiItem>(imageGetPositionX(104), imageGetPositionX(104 + 16),
                                     imageGetPositionY(107), imageGetPositionY(107 + 16),
                                     std::shared_ptr<ItemStack>(pOutput, deleter)));
    }
public:
};
}
}
}

#endif // INVENTORY_H_INCLUDED
