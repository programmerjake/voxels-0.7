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
#ifndef WOOD_ITEM_H_INCLUDED
#define WOOD_ITEM_H_INCLUDED

#include "item/builtin/block.h"
#include "item/builtin/image.h"
#include "util/wood_descriptor.h"
#include "render/generate.h"
#include "generate/biome/biome_descriptor.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
class Stick final : public ItemImage
{
    friend class global_instance_maker<Stick>;
private:
    Stick()
        : ItemImage(L"builtin.stick", TextureAtlas::Stick.td(), nullptr)
    {
    }
public:
    static const Stick *pointer()
    {
        return global_instance_maker<Stick>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual float getFurnaceBurnTime() const override
    {
        return 5.0f;
    }
};
class WoodLog final : public ItemBlock
{
private:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor)
    {
        std::wstring retval = L"builtin.wood_log(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L")";
        return retval;
    }
public:
    WoodLog(WoodDescriptorPointer woodDescriptor, BlockDescriptorPointer block)
        : ItemBlock(makeName(woodDescriptor),
                    woodDescriptor->getLogSideTexture(), woodDescriptor->getLogSideTexture(),
                    woodDescriptor->getLogTopTexture(), woodDescriptor->getLogTopTexture(),
                    woodDescriptor->getLogSideTexture(), woodDescriptor->getLogSideTexture(),
                    block), woodDescriptor(woodDescriptor)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
    virtual Item onUse(Item item, World &world, WorldLockManager &lock_manager, Player &player) const override
    {
        RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
        if(c.valid())
        {
            LogOrientation logOrientation = LogOrientation::Y;
            switch(c.blockFace)
            {
            case BlockFaceOrNone::None:
                break;
            case BlockFaceOrNone::NX:
            case BlockFaceOrNone::PX:
                logOrientation = LogOrientation::X;
                break;
            case BlockFaceOrNone::NY:
            case BlockFaceOrNone::PY:
                logOrientation = LogOrientation::Y;
                break;
            case BlockFaceOrNone::NZ:
            case BlockFaceOrNone::PZ:
                logOrientation = LogOrientation::Z;
                break;
            }
            if(player.placeBlock(c, world, lock_manager, Block(woodDescriptor->getLogBlockDescriptor(logOrientation))))
                return getAfterPlaceItem();
        }
        return item;
    }
    virtual float getFurnaceBurnTime() const override
    {
        return 15.0f;
    }
};
class WoodPlanks final : public ItemBlock
{
private:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor)
    {
        std::wstring retval = L"builtin.wood_planks(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L")";
        return retval;
    }
public:
    WoodPlanks(WoodDescriptorPointer woodDescriptor, BlockDescriptorPointer block)
        : ItemBlock(makeName(woodDescriptor),
                    woodDescriptor->getPlanksTexture(), block), woodDescriptor(woodDescriptor)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
    virtual float getFurnaceBurnTime() const override
    {
        return 15.0f;
    }
};
class WoodLeaves final : public ItemBlock
{
private:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor)
    {
        std::wstring retval = L"builtin.wood_leaves(woodDescriptor=";
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
    WoodLeaves(WoodDescriptorPointer woodDescriptor, BlockDescriptorPointer block)
        : ItemBlock(makeName(woodDescriptor), makeMesh(woodDescriptor), block), woodDescriptor(woodDescriptor)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
};
}
}
}
}

#endif // WOOD_ITEM_H_INCLUDED
