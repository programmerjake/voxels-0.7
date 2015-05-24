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
#ifndef ITEM_LADDER_H_INCLUDED
#define ITEM_LADDER_H_INCLUDED

#include "item/builtin/image.h"
#include "block/builtin/ladder.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{

class GenericLadder : public ItemImage
{
public:
    GenericLadder(std::wstring name, TextureDescriptor td, const Blocks::builtin::GenericLadder *block)
        : ItemImage(name, td, block)
    {
    }
protected:
    const Blocks::builtin::GenericLadder *getBlock() const
    {
        return dynamic_cast<const Blocks::builtin::GenericLadder *>(ItemImage::getBlock());
    }
public:
    virtual Item onUse(Item item, World &world, WorldLockManager &lock_manager, Player &player) const override
    {
        const Blocks::builtin::GenericLadder *block = getBlock();
        if(block == nullptr)
            return item;
        RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
        if(c.valid())
        {
            if(c.type == RayCasting::Collision::Type::Block && c.blockFace != BlockFaceOrNone::None)
            {
                BlockDescriptorPointer newDescriptor = block->getDescriptor(getOppositeBlockFace(toBlockFace(c.blockFace)));
                if(newDescriptor != nullptr)
                {
                    if(player.placeBlock(c, world, lock_manager, Block(newDescriptor)))
                        return getAfterPlaceItem();
                }
            }
        }
        return item;
    }
};

class Ladder final : public GenericLadder
{
    friend class global_instance_maker<Ladder>;
private:
    Ladder()
        : GenericLadder(L"builtin.ladder", TextureAtlas::Ladder.td(), Blocks::builtin::Ladder::pointer())
    {
    }
public:
    static const Ladder *pointer()
    {
        return global_instance_maker<Ladder>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
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

#endif // ITEM_LADDER_H_INCLUDED
