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
#ifndef ITEM_TOOL_H_INCLUDED
#define ITEM_TOOL_H_INCLUDED

#include "item/builtin/image.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
namespace tools
{
class Tool : public ItemImage
{
protected:
    Tool(std::wstring name, Mesh faceMesh, const Entities::builtin::EntityItem *entity)
        : ItemImage(name, faceMesh, nullptr, entity)
    {
    }
    Tool(std::wstring name, TextureDescriptor td, const Entities::builtin::EntityItem *entity)
        : ItemImage(name, td, nullptr, entity)
    {
    }
    virtual Item getAfterPlaceItem() const final
    {
        return Item();
    }
    struct ToolData
    {
        const unsigned damage;
        ToolData(unsigned damage)
            : damage(damage)
        {
        }
    };
public:
    virtual unsigned maxDamage() const = 0;
    virtual Item setDamage(Item item, unsigned newDamage) const
    {
        if(newDamage == 0)
        {
            return Item(item.descriptor);
        }
        else
        {
            return Item(item.descriptor, std::static_pointer_cast<void>(std::make_shared<ToolData>(newDamage)));
        }
    }
    virtual unsigned getDamage(Item item) const
    {
        if(item.data == nullptr)
            return 0;
        const ToolData *toolData = static_cast<const ToolData *>(item.data.get());
        return toolData->damage;
    }
    virtual void render(Item item, Mesh &dest, float minX, float maxX, float minY, float maxY) const override
    {
        ItemImage::render(item, dest, minX, maxX, minY, maxY);
        Matrix damageTransform()
        #error finish
        dest.append(transform())
    }
    virtual bool dataEqual(std::shared_ptr<void> data1, std::shared_ptr<void> data2) const override
    {
        const ToolData *toolData1 = static_cast<const ToolData *>(data1.get());
        const ToolData *toolData2 = static_cast<const ToolData *>(data2.get());
        return toolData1->damage == toolData2->damage;
    }
    virtual unsigned getMaxStackCount() const
    {
        return 1;
    }
};
}
}
}
}
}

#endif // ITEM_TOOL_H_INCLUDED
