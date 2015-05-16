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
#ifndef BLOCK_BEDROCK_H_INCLUDED
#define BLOCK_BEDROCK_H_INCLUDED

#include "block/builtin/stone_block.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class Bedrock final : public StoneBlock
{
    friend class global_instance_maker<Bedrock>;
public:
    static const Bedrock *pointer()
    {
        return global_instance_maker<Bedrock>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Bedrock()
        : StoneBlock(L"builtin.bedrock", TextureAtlas::Bedrock.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return -1;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
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

#endif // BLOCK_BEDROCK_H_INCLUDED
