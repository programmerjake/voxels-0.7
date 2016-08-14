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
#ifndef ITEM_CHEST_H_INCLUDED
#define ITEM_CHEST_H_INCLUDED

#include "item/builtin/block.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
class Chest final : public ItemBlock
{
    friend class global_instance_maker<Chest>;

private:
    Chest();

public:
    static const Chest *pointer()
    {
        return global_instance_maker<Chest>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item onUse(Item item,
                       World &world,
                       WorldLockManager &lock_manager,
                       Player &player) const override;
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

#endif // ITEM_CHEST_H_INCLUDED
