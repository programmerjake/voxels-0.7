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
#include "item/builtin/bucket.h"
#include "texture/texture_atlas.h"
#include "player/player.h"
#include "block/builtin/fluid.h"
#include "block/builtin/water.h"
namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{

Bucket::Bucket()
    : ItemImage(L"builtin.bucket", TextureAtlas::Bucket.td(), nullptr)
{
}
Item Bucket::onUse(Item item, World &world, WorldLockManager &lock_manager, Player &player) const
{
    RayCasting::Collision c = player.castRay(world, lock_manager, RayCasting::BlockCollisionMaskEverything);
    if(c.valid())
    {
        if(c.type != RayCasting::Collision::Type::Block)
            return item;
        Block b = world.getBlockIterator(c.blockPosition).get(lock_manager);
        if(!b.good())
            return item;
        const Blocks::builtin::Fluid *fluid = dynamic_cast<const Blocks::builtin::Fluid *>(b.descriptor);
        if(fluid == nullptr)
            return item;
        Item retval = fluid->getFilledBucket();
        if(player.removeBlock(c, world, lock_manager))
            return retval;
    }
    return item;
}
WaterBucket::WaterBucket()
    : ItemImage(L"builtin.water_bucket", TextureAtlas::WaterBucket.td(), Blocks::builtin::Water::descriptor())
{
}
}
}
}
}
