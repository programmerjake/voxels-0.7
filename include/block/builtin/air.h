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
#ifndef AIR_H_INCLUDED
#define AIR_H_INCLUDED

#include "block/block.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class Air final : public BlockDescriptor
{
    friend class global_instance_maker<Air>;
public:
    static const Air *pointer()
    {
        return global_instance_maker<Air>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual bool isReplaceable() const override
    {
        return true;
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override
    {
    }
    virtual float getHardness() const override
    {
        return -1;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return true;
    }
private:
    Air()
        : BlockDescriptor(L"builtin.air", BlockShape(nullptr), LightProperties(), (RayCasting::BlockCollisionMask)0, true, false, false, false, false, false, false, Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), RenderLayer::Opaque)
    {
    }
};
}
}
}
}

#endif // AIR_H_INCLUDED
