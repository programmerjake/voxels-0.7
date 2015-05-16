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
#ifndef CRAFTING_TABLE_H_INCLUDED
#define CRAFTING_TABLE_H_INCLUDED

#include "block/builtin/full_block.h"
#include "texture/texture_atlas.h"
#include "util/global_instance_maker.h"
#include "item/builtin/tools/tools.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class CraftingTable final : public FullBlock
{
    friend class global_instance_maker<CraftingTable>;
private:
    CraftingTable()
        : FullBlock(L"builtin.crafting_table", LightProperties(Lighting(), Lighting::makeMaxLight()), RayCasting::BlockCollisionMaskGround,
                    true, true, true, true, true, true,
                    TextureAtlas::WorkBenchSide0.td(), TextureAtlas::WorkBenchSide0.td(),
                    TextureAtlas::OakPlank.td(), TextureAtlas::WorkBenchTop.td(),
                    TextureAtlas::WorkBenchSide1.td(), TextureAtlas::WorkBenchSide1.td(),
                    RenderLayer::Opaque)
    {
    }
public:
    static const CraftingTable *pointer()
    {
        return global_instance_maker<CraftingTable>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual bool isReplaceable() const override
    {
        return false;
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual bool onUse(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const override;
    virtual float getHardness() const override
    {
        return 2.5f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Axe *>(tool.descriptor) != nullptr;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};
}
}
}
}

#endif // CRAFTING_TABLE_H_INCLUDED
