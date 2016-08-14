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
#ifndef ITEM_FURNACE_H_INCLUDED
#define ITEM_FURNACE_H_INCLUDED

#include "item/builtin/block.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"
#include "player/player.h"
#include "block/builtin/furnace.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
class Furnace final : public ItemBlock
{
    friend class global_instance_maker<Furnace>;

private:
    Furnace()
        : ItemBlock(L"builtin.furnace",
                    TextureAtlas::FurnaceSide.td(),
                    TextureAtlas::FurnaceSide.td(),
                    TextureAtlas::DispenserDropperPistonFurnaceFrame.td(),
                    TextureAtlas::DispenserDropperPistonFurnaceFrame.td(),
                    TextureAtlas::FurnaceSide.td(),
                    TextureAtlas::FurnaceFrontOff.td(),
                    Blocks::builtin::Furnace::descriptor())
    {
    }

public:
    static const Furnace *pointer()
    {
        return global_instance_maker<Furnace>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item onUse(Item item,
                       World &world,
                       WorldLockManager &lock_manager,
                       Player &player) const override
    {
        RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
        if(c.valid())
        {
            if(player.placeBlock(c,
                                 world,
                                 lock_manager,
                                 Block(Blocks::builtin::Furnace::descriptor(
                                     player.getViewDirectionXZBlockFaceIn()))))
                return getAfterPlaceItem();
        }
        return item;
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};
}
}
}
}

#endif // ITEM_FURNACE_H_INCLUDED
