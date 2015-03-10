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
#ifndef WOOD_ENTITY_ITEM_H_INCLUDED
#define WOOD_ENTITY_ITEM_H_INCLUDED

#include "util/wood_descriptor.h"
#include "player/player.h"
#include "entity/builtin/block_item.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
namespace items
{
class WoodLog final : public BlockItem
{
private:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor)
    {
        std::wstring retval = L"builtin.items.wood_log(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L")";
        return retval;
    }
public:
    explicit WoodLog(WoodDescriptorPointer woodDescriptor)
        : BlockItem(makeName(woodDescriptor),
                    woodDescriptor->getLogSideTexture(), woodDescriptor->getLogSideTexture(),
                    woodDescriptor->getLogTopTexture(), woodDescriptor->getLogTopTexture(),
                    woodDescriptor->getLogSideTexture(), woodDescriptor->getLogSideTexture(),
                    RenderLayer::Opaque), woodDescriptor(woodDescriptor)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
protected:
    virtual void onGiveToPlayer(Player &player) const override
    {
        player.addItem(Item(woodDescriptor->getLogItemDescriptor()));
    }
};
class WoodPlanks final : public BlockItem
{
private:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor)
    {
        std::wstring retval = L"builtin.items.wood_planks(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L")";
        return retval;
    }
public:
    explicit WoodPlanks(WoodDescriptorPointer woodDescriptor)
        : BlockItem(makeName(woodDescriptor), woodDescriptor->getPlanksTexture(), RenderLayer::Opaque), woodDescriptor(woodDescriptor)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
protected:
    virtual void onGiveToPlayer(Player &player) const override
    {
        player.addItem(Item(woodDescriptor->getPlanksItemDescriptor()));
    }
};
class WoodLeaves final : public BlockItem
{
private:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor)
    {
        std::wstring retval = L"builtin.items.wood_leaves(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L")";
        return retval;
    }
    static Mesh makeMesh(WoodDescriptorPointer woodDescriptor)
    {
        return colorize(BiomeDescriptor::makeBiomeLeavesColor(0.5f, 0.5f), Generate::unitBox(woodDescriptor->getLeavesTexture(), woodDescriptor->getLeavesTexture(),
                                                                                             woodDescriptor->getLeavesTexture(), woodDescriptor->getLeavesTexture(),
                                                                                             woodDescriptor->getLeavesTexture(), woodDescriptor->getLeavesTexture()));
    }
public:
    explicit WoodLeaves(WoodDescriptorPointer woodDescriptor)
        : BlockItem(makeName(woodDescriptor),
                    makeMesh(woodDescriptor), RenderLayer::Opaque), woodDescriptor(woodDescriptor)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
protected:
    virtual void onGiveToPlayer(Player &player) const override
    {
        player.addItem(Item(woodDescriptor->getLeavesItemDescriptor()));
    }
};
}
}
}
}
}

#endif // WOOD_ENTITY_ITEM_H_INCLUDED
