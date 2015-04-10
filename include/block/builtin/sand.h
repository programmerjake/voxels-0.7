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
#ifndef SAND_BLOCK_H_INCLUDED
#define SAND_BLOCK_H_INCLUDED

#include "block/builtin/falling_full_block.h"
#include "item/builtin/tools/tools.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class Sand final : public FallingFullBlock
{
    friend class global_instance_maker<Sand>;
public:
    static const Sand *pointer()
    {
        return global_instance_maker<Sand>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Sand()
        : FallingFullBlock(L"builtin.sand", LightProperties(Lighting(), Lighting::makeMaxLight()), RayCasting::BlockCollisionMaskGround, true, TextureAtlas::Sand.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 0.5f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Shovel *>(tool.descriptor) != nullptr;
    }
protected:
    virtual Entity *makeFallingBlockEntity(World &world, PositionF position, WorldLockManager &lock_manager) const override;
};
class Gravel final : public FallingFullBlock
{
    friend class global_instance_maker<Gravel>;
public:
    static const Gravel *pointer()
    {
        return global_instance_maker<Gravel>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Gravel()
        : FallingFullBlock(L"builtin.gravel", LightProperties(Lighting(), Lighting::makeMaxLight()), RayCasting::BlockCollisionMaskGround, true, TextureAtlas::Gravel.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 0.5f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Shovel *>(tool.descriptor) != nullptr;
    }
protected:
    virtual Entity *makeFallingBlockEntity(World &world, PositionF position, WorldLockManager &lock_manager) const override;
};
}
}
}
}

#endif // SAND_BLOCK_H_INCLUDED
