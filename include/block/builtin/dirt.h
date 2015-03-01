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
#ifndef DIRT_H_INCLUDED
#define DIRT_H_INCLUDED

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
class Dirt final : public DirtBlock
{
    friend class global_instance_maker<Dirt>;
public:
    static const Dirt *pointer()
    {
        return global_instance_maker<Dirt>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Dirt()
        : DirtBlock(L"builtin.dirt", true,
                    TextureAtlas::Dirt.td(), TextureAtlas::Dirt.td(),
                    TextureAtlas::Dirt.td(), TextureAtlas::Dirt.td(),
                    TextureAtlas::Dirt.td(), TextureAtlas::Dirt.td(),
                    TextureDescriptor(), TextureDescriptor(),
                    TextureDescriptor(), TextureDescriptor(),
                    TextureDescriptor(), TextureDescriptor())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const override;
    virtual void randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const override;
};
}
}
}
}

#endif // DIRT_H_INCLUDED

