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
#ifndef GRASS_H_INCLUDED
#define GRASS_H_INCLUDED

#include "block/builtin/dirt_block.h"
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
class Grass final : public DirtBlock
{
    friend class global_instance_maker<Grass>;
public:
    static const Grass *pointer()
    {
        return global_instance_maker<Grass>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Grass()
        : DirtBlock(L"builtin.grass", true,
                    TextureAtlas::DirtMask.td(), TextureAtlas::DirtMask.td(),
                    TextureAtlas::Dirt.td(), TextureDescriptor(),
                    TextureAtlas::DirtMask.td(), TextureAtlas::DirtMask.td(),
                    TextureAtlas::GrassMask.td(), TextureAtlas::GrassMask.td(),
                    TextureDescriptor(), TextureAtlas::GrassTop.td(),
                    TextureAtlas::GrassMask.td(), TextureAtlas::GrassMask.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual void randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const override;
    virtual float getHardness() const override
    {
        return 0.6f;
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

#endif // GRASS_H_INCLUDED
