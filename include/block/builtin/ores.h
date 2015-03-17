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
#ifndef BLOCK_ORES_H_INCLUDED
#define BLOCK_ORES_H_INCLUDED

#include "block/builtin/stone_block.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{

class CoalOre final : public StoneBlock
{
    friend class global_instance_maker<CoalOre>;
public:
    static const CoalOre *pointer()
    {
        return global_instance_maker<CoalOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    CoalOre()
        : StoneBlock(L"builtin.coal_ore", TextureAtlas::CoalOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Wood;
    }
};

class IronOre final : public StoneBlock
{
    friend class global_instance_maker<IronOre>;
public:
    static const IronOre *pointer()
    {
        return global_instance_maker<IronOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    IronOre()
        : StoneBlock(L"builtin.iron_ore", TextureAtlas::IronOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Stone;
    }
};

class GoldOre final : public StoneBlock
{
    friend class global_instance_maker<GoldOre>;
public:
    static const GoldOre *pointer()
    {
        return global_instance_maker<GoldOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    GoldOre()
        : StoneBlock(L"builtin.gold_ore", TextureAtlas::GoldOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Iron;
    }
};

}
}
}
}

#endif // BLOCK_ORES_H_INCLUDED
