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
#include "item/builtin/chest.h"
#include "block/builtin/chest.h"
#include "texture/texture_atlas.h"
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
Chest::Chest()
    : ItemBlock(L"builtin.chest",
                TextureAtlas::ChestSide.td(), TextureAtlas::ChestSide.td(),
                TextureAtlas::ChestTop.td(), TextureAtlas::ChestTop.td(),
                TextureAtlas::ChestSide.td(), TextureAtlas::ChestFront.td(),
                Blocks::builtin::Chest::descriptor())
{
}
Item Chest::onUse(Item item, World &world, WorldLockManager &lock_manager, Player &player) const
{
    RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
    if(c.valid())
    {
        if(player.placeBlock(c, world, lock_manager, Block(Blocks::builtin::Chest::descriptor(player.getViewDirectionXZBlockFaceIn()))))
            return getAfterPlaceItem();
    }
    return item;
}
}
}
}
}
