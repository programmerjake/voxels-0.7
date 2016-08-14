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
#include "block/builtin/chest.h"
#include "item/builtin/chest.h"
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
class ChestUi final : public PlayerDialog
{
    ChestUi(const ChestUi &) = delete;
    ChestUi &operator=(const ChestUi &) = delete;

private:
    std::shared_ptr<Blocks::builtin::Chest::ChestData> chestData;
    PositionI blockPosition;
    World *pworld = nullptr;
    WorldLockManager *plock_manager = nullptr;

public:
    ChestUi(std::shared_ptr<Player> player,
            std::shared_ptr<Blocks::builtin::Chest::ChestData> chestData,
            PositionI blockPosition)
        : PlayerDialog(player, TextureAtlas::ChestUI.td()),
          chestData(chestData),
          blockPosition(blockPosition)
    {
    }

protected:
    virtual unsigned transferItems(std::shared_ptr<ItemStack> sourceItemStack,
                                   std::shared_ptr<ItemStack> destItemStack,
                                   unsigned transferCount) override
    {
        Blocks::builtin::Chest::ChestData::ItemsType *items = &chestData->items;
        unsigned retval = 0;
        bool found = false;
        for(auto &i : items->itemStacks)
        {
            for(ItemStack &v : i)
            {
                if(&v == sourceItemStack.get() || &v == destItemStack.get())
                {
                    retval = destItemStack->transfer(*sourceItemStack, transferCount);
                    found = true;
                    break;
                }
            }
            if(found)
                break;
        }
        if(!found)
            return PlayerDialog::transferItems(sourceItemStack, destItemStack, transferCount);
        assert(plock_manager != nullptr);
        assert(pworld != nullptr);
        WorldLockManager &lock_manager = *plock_manager;
        World &world = *pworld;
        BlockIterator bi = world.getBlockIterator(blockPosition, lock_manager.tls);
        Block b = bi.get(lock_manager);
        if(!b.good() || dynamic_cast<const Blocks::builtin::Chest *>(b.descriptor) == nullptr)
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
        int X = 5;
        for(auto &i : chestData->items.itemStacks)
        {
            int Y = 94;
            for(ItemStack &v : i)
            {
                add(std::make_shared<UiItem>(imageGetPositionX(X),
                                             imageGetPositionX(X + 16),
                                             imageGetPositionY(Y),
                                             imageGetPositionY(Y + 16),
                                             std::shared_ptr<ItemStack>(chestData, &v),
                                             &chestData->lock));
                Y += 18;
            }
            X += 18;
        }
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
        if(!b.good() || dynamic_cast<const Blocks::builtin::Chest *>(b.descriptor) == nullptr)
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
void Chest::onBreak(
    World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const
{
    if(isMatchingTool(tool))
    {
        if(b.data != nullptr)
        {
            std::shared_ptr<ChestData> data = static_cast<ChestBlockData *>(b.data.get())->data;
            lock_manager.clear();
            std::unique_lock<std::recursive_mutex> lockIt(data->lock);
            Blocks::builtin::Chest::ChestData::ItemsType items = data->items;
            data->items = Blocks::builtin::Chest::ChestData::ItemsType();
            lockIt.unlock();
            for(auto &i : items.itemStacks)
            {
                for(ItemStack &v : i)
                {
                    if(v.good())
                        ItemDescriptor::addToWorld(
                            world, lock_manager, v, bi.position() + VectorF(0.5));
                }
            }
        }
        ItemDescriptor::addToWorld(world,
                                   lock_manager,
                                   ItemStack(Item(Items::builtin::Chest::descriptor())),
                                   bi.position() + VectorF(0.5));
    }
    handleToolDamage(tool);
}
bool Chest::onUse(World &world,
                  Block b,
                  BlockIterator bi,
                  WorldLockManager &lock_manager,
                  std::shared_ptr<Player> player) const
{
    if(b.data == nullptr)
        return false;
    std::shared_ptr<ChestData> data = static_cast<ChestBlockData *>(b.data.get())->data;
    return player->setDialog(std::make_shared<ui::builtin::ChestUi>(player, data, bi.position()));
}
}
}
}
}
