/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef ITEM_TOOL_H_INCLUDED
#define ITEM_TOOL_H_INCLUDED

#include "item/builtin/image.h"
#include "util/tool_level.h"
#include "block/block.h"

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
    Tool(std::wstring name, Mesh faceMesh, Mesh entityMesh)
        : ItemImage(name, faceMesh, entityMesh, nullptr)
    {
    }
    Tool(std::wstring name, TextureDescriptor td)
        : ItemImage(name, td, nullptr)
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
            return Item(item.descriptor);
        else if(newDamage >= maxDamage())
            return Item();
        else
            return Item(item.descriptor, std::static_pointer_cast<void>(std::make_shared<ToolData>(newDamage)));
    }
    virtual Item addDamage(Item item, unsigned additionalDamage) const
    {
        return setDamage(item, getDamage(item) + additionalDamage);
    }
    virtual unsigned getDamage(Item item) const
    {
        if(item.data == nullptr)
            return 0;
        const ToolData *toolData = static_cast<const ToolData *>(item.data.get());
        return toolData->damage;
    }
    virtual float getRelativeDamage(Item item) const
    {
        return (float)getDamage(item) / (float)maxDamage();
    }
    virtual void render(Item item, Mesh &dest, float minX, float maxX, float minY, float maxY) const override
    {
        ItemImage::render(item, dest, minX, maxX, minY, maxY);
        float damage = getRelativeDamage(item);
        if(damage > 0)
        {
            Matrix damageTransform = Matrix::scale(maxX - minX, maxY - minY, 1).concat(Matrix::translate(minX, minY, -1)).concat(Matrix::scale(0.5f * (minRenderZ + maxRenderZ)));
            dest.append(transform(damageTransform, Generate::itemDamage(damage)));
        }
    }
    virtual bool dataEqual(std::shared_ptr<void> data1, std::shared_ptr<void> data2) const override
    {
        const ToolData *toolData1 = static_cast<const ToolData *>(data1.get());
        const ToolData *toolData2 = static_cast<const ToolData *>(data2.get());
        if(toolData1 == toolData2)
            return true;
        if(toolData1 == nullptr || toolData2 == nullptr)
            return false;
        return toolData1->damage == toolData2->damage;
    }
    virtual unsigned getMaxStackCount() const
    {
        return 1;
    }
    virtual float getMineDurationFactor(Item item) const = 0;
    virtual ToolLevel getToolLevel() const = 0;
protected:
    virtual Item incrementDamage(Item tool, bool toolKindMatches, bool toolCanMine, bool minedInstantly) const = 0;
public:
    static Item incrementDamage(Item tool, BlockDescriptorPointer blockDescriptor)
    {
        if(!tool.good())
            return tool;
        const Tool *descriptor = dynamic_cast<const Tool *>(tool.descriptor);
        if(descriptor == nullptr)
            return tool;
        return descriptor->incrementDamage(tool, blockDescriptor->isHelpingToolKind(tool), blockDescriptor->isMatchingTool(tool), blockDescriptor->getBreakDuration(tool) == 0);
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        assert(static_cast<std::uint16_t>(maxDamage()) == maxDamage());
        std::uint16_t damage = stream::read_limited<std::uint16_t>(reader, 0, maxDamage());
        if(damage == 0)
            return nullptr;
        return std::static_pointer_cast<void>(std::make_shared<ToolData>(damage));
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
        assert(static_cast<std::uint16_t>(maxDamage()) == maxDamage());
        std::uint16_t damage = 0;
        if(data != nullptr)
            damage = static_cast<const ToolData *>(data.get())->damage;
        stream::write<std::uint16_t>(writer, damage);
    }
};
}
}
}
}
}

#endif // ITEM_TOOL_H_INCLUDED
