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
#include "recipe/builtin/pattern.h"
#include "item/builtin/minerals.h" // iron ingot
#include "item/builtin/crafting_table.h"

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
        Block b = world.getBlockIterator(c.blockPosition, lock_manager.tls).get(lock_manager);
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

namespace Recipes
{
namespace builtin
{
class BucketRecipe final : public PatternRecipe<3, 2>
{
    friend class global_instance_maker<BucketRecipe>;
protected:
    virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
    {
        if(input.getRecipeBlock().good() && input.getRecipeBlock().descriptor != Items::builtin::CraftingTable::descriptor())
            return false;
        output = RecipeOutput(ItemStack(Item(Items::builtin::Bucket::descriptor()), 1));
        return true;
    }
private:
    BucketRecipe()
        : PatternRecipe(checked_array<Item, 3 * 2>
        {
            Item(Items::builtin::IronIngot::descriptor()), Item(), Item(Items::builtin::IronIngot::descriptor()),
            Item(), Item(Items::builtin::IronIngot::descriptor()), Item(),
        })
    {
    }
public:
    static const BucketRecipe *pointer()
    {
        return global_instance_maker<BucketRecipe>::getInstance();
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
