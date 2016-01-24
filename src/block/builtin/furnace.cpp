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
#include "block/builtin/furnace.h"
#include "item/builtin/furnace.h"
#include "ui/player_dialog.h"
#include "recipe/builtin/pattern.h"
#include "item/builtin/cobblestone.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
namespace builtin
{
class FurnaceUi final : public PlayerDialog
{
    FurnaceUi(const FurnaceUi &) = delete;
    FurnaceUi &operator =(const FurnaceUi &) = delete;
private:
    std::shared_ptr<Blocks::builtin::Furnace::FurnaceData> furnaceData;
    PositionI blockPosition;
    World *pworld = nullptr;
    WorldLockManager *plock_manager = nullptr;
public:
    FurnaceUi(std::shared_ptr<Player> player, std::shared_ptr<Blocks::builtin::Furnace::FurnaceData> furnaceData, PositionI blockPosition)
        : PlayerDialog(player, TextureAtlas::FurnaceUI.td()), furnaceData(furnaceData), blockPosition(blockPosition)
    {
    }
protected:
    virtual unsigned transferItems(std::shared_ptr<ItemStack> sourceItemStack, std::shared_ptr<ItemStack> destItemStack, unsigned transferCount) override
    {
        ItemStack *inputStack = &furnaceData->inputStack;
        ItemStack *outputStack = &furnaceData->outputStack;
        ItemStack *fuelStack = &furnaceData->fuelStack;
        if(destItemStack.get() == outputStack)
            return 0;
        unsigned retval;
        if(destItemStack.get() == fuelStack)
            retval = furnaceData->transferToFuelStack(*sourceItemStack, transferCount);
        else if(destItemStack.get() == inputStack)
            retval = destItemStack->transfer(*sourceItemStack, transferCount);
        else if(sourceItemStack.get() == inputStack)
            retval = destItemStack->transfer(*sourceItemStack, transferCount);
        else if(sourceItemStack.get() == fuelStack)
            retval = furnaceData->transferFromFuelStack(*destItemStack, transferCount);
        else if(sourceItemStack.get() == outputStack)
            retval = destItemStack->transfer(*sourceItemStack, transferCount);
        else
            return PlayerDialog::transferItems(sourceItemStack, destItemStack, transferCount);
        assert(plock_manager != nullptr);
        assert(pworld != nullptr);
        WorldLockManager &lock_manager = *plock_manager;
        World &world = *pworld;
        BlockIterator bi = world.getBlockIterator(blockPosition, lock_manager.tls);
        Block b = bi.get(lock_manager);
        if(!b.good() || dynamic_cast<const Blocks::builtin::Furnace *>(b.descriptor) == nullptr)
        {
            lock_manager.clear();
            return retval;
        }
        world.rescheduleBlockUpdate(bi, lock_manager, BlockUpdateKind::General, 0);
        lock_manager.clear();
        return retval;
    }
    virtual void addElements() override
    {
        std::shared_ptr<Player> player = this->player;
        add(std::make_shared<UiItem>(imageGetPositionX(59), imageGetPositionX(59 + 16),
                                     imageGetPositionY(91), imageGetPositionY(91 + 16),
                                     std::shared_ptr<ItemStack>(furnaceData, &furnaceData->fuelStack), &furnaceData->lock));
        add(std::make_shared<UiItem>(imageGetPositionX(59), imageGetPositionX(59 + 16),
                                     imageGetPositionY(127), imageGetPositionY(127 + 16),
                                     std::shared_ptr<ItemStack>(furnaceData, &furnaceData->inputStack), &furnaceData->lock));
        add(std::make_shared<UiItem>(imageGetPositionX(101), imageGetPositionX(101 + 16),
                                     imageGetPositionY(109), imageGetPositionY(109 + 16),
                                     std::shared_ptr<ItemStack>(furnaceData, &furnaceData->outputStack), &furnaceData->lock));
    }
    virtual void setWorldAndLockManager(World &world, WorldLockManager &lock_manager) override
    {
        pworld = &world;
        plock_manager = &lock_manager;
        PlayerDialog::setWorldAndLockManager(world, lock_manager);
    }
public:
    virtual void move(double deltaTime) override
    {
        assert(plock_manager != nullptr);
        assert(pworld != nullptr);
        WorldLockManager &lock_manager = *plock_manager;
        World &world = *pworld;
        BlockIterator bi = world.getBlockIterator(blockPosition, lock_manager.tls);
        Block b = bi.get(lock_manager);
        if(!b.good() || dynamic_cast<const Blocks::builtin::Furnace *>(b.descriptor) == nullptr)
            quit();
        lock_manager.clear();
        PlayerDialog::move(deltaTime);
    }
};
}
}

namespace Blocks
{
namespace builtin
{

void Furnace::onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        if(b.data != nullptr)
        {
            std::shared_ptr<FurnaceData> data = static_cast<FurnaceBlockData *>(b.data.get())->data;
            lock_manager.clear();
            std::unique_lock<std::recursive_mutex> lockIt(data->lock);
            ItemStack inputStack = data->inputStack;
            data->inputStack = ItemStack();
            ItemStack outputStack = data->outputStack;
            data->outputStack = ItemStack();
            ItemStack fuelStack = data->fuelStack;
            data->fuelStack = ItemStack();
            lockIt.unlock();
            if(inputStack.good())
                ItemDescriptor::addToWorld(world, lock_manager, inputStack, bi.position() + VectorF(0.5));
            if(outputStack.good())
                ItemDescriptor::addToWorld(world, lock_manager, outputStack, bi.position() + VectorF(0.5));
            if(fuelStack.good())
                ItemDescriptor::addToWorld(world, lock_manager, fuelStack, bi.position() + VectorF(0.5));
        }
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(Items::builtin::Furnace::descriptor())), bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}
bool Furnace::onUse(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const
{
    if(b.data == nullptr)
        return false;
    std::shared_ptr<FurnaceData> data = static_cast<FurnaceBlockData *>(b.data.get())->data;
    return player->setDialog(std::make_shared<ui::builtin::FurnaceUi>(player, data, bi.position()));
}

}
}

namespace Recipes
{
namespace builtin
{
class FurnaceRecipe : public PatternRecipe<3, 3>
{
    friend class global_instance_maker<FurnaceRecipe>;
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && !input.getRecipeBlock().descriptor->isToolForCrafting())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::Furnace::descriptor()), 1));
        return true;
    }
private:
    FurnaceRecipe()
        : PatternRecipe(checked_array<Item, 3 * 3>
        {
            Item(Items::builtin::Cobblestone::descriptor()), Item(Items::builtin::Cobblestone::descriptor()), Item(Items::builtin::Cobblestone::descriptor()),
            Item(Items::builtin::Cobblestone::descriptor()), Item(), Item(Items::builtin::Cobblestone::descriptor()),
            Item(Items::builtin::Cobblestone::descriptor()), Item(Items::builtin::Cobblestone::descriptor()), Item(Items::builtin::Cobblestone::descriptor()),
        })
    {
    }
public:
    static const FurnaceRecipe *pointer()
    {
        return global_instance_maker<FurnaceRecipe>::getInstance();
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
