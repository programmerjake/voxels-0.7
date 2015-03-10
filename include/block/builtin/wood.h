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
#ifndef WOOD_BLOCK_H_INCLUDED
#define WOOD_BLOCK_H_INCLUDED

#include "block/builtin/full_block.h"
#include "util/wood_descriptor.h"
#include "world/world.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class WoodLog : public FullBlock
{
private:
    const WoodDescriptorPointer woodDescriptor;
    const LogOrientation logOrientation;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor, LogOrientation logOrientation)
    {
        std::wstring retval = L"builtin.wood_log(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L",logOrientation=";
        switch(logOrientation)
        {
        case LogOrientation::AllBark:
            retval += L"AllBark";
            break;
        case LogOrientation::X:
            retval += L"X";
            break;
        case LogOrientation::Y:
            retval += L"Y";
            break;
        case LogOrientation::Z:
            retval += L"Z";
            break;
        }
        retval += L")";
        return retval;
    }
    static TextureDescriptor getTextureDescriptor(WoodDescriptorPointer woodDescriptor, LogOrientation logOrientation, BlockFace bf)
    {
        VectorI faceVector = getBlockFaceOutDirection(bf);
        VectorI logVector;
        switch(logOrientation)
        {
        case LogOrientation::AllBark:
            logVector = VectorI(0);
            break;
        case LogOrientation::X:
            logVector = VectorI(1, 0, 0);
            break;
        case LogOrientation::Y:
            logVector = VectorI(0, 1, 0);
            break;
        case LogOrientation::Z:
            logVector = VectorI(0, 0, 1);
            break;
        }
        if(dot(faceVector, logVector) == 0)
            return woodDescriptor->getLogSideTexture();
        return woodDescriptor->getLogTopTexture();
    }
public:
    WoodLog(WoodDescriptorPointer woodDescriptor, LogOrientation logOrientation)
        : FullBlock(makeName(woodDescriptor, logOrientation), LightProperties(Lighting(), Lighting::makeMaxLight()), RayCasting::BlockCollisionMaskGround,
                    true, true, true, true, true, true,
                    getTextureDescriptor(woodDescriptor, logOrientation, BlockFace::NX),
                    getTextureDescriptor(woodDescriptor, logOrientation, BlockFace::PX),
                    getTextureDescriptor(woodDescriptor, logOrientation, BlockFace::NY),
                    getTextureDescriptor(woodDescriptor, logOrientation, BlockFace::PY),
                    getTextureDescriptor(woodDescriptor, logOrientation, BlockFace::NZ),
                    getTextureDescriptor(woodDescriptor, logOrientation, BlockFace::PZ),
                    RenderLayer::Opaque), woodDescriptor(woodDescriptor), logOrientation(logOrientation)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
    LogOrientation getLogOrientation() const
    {
        return logOrientation;
    }
    virtual bool isReplaceable() const override
    {
        return false;
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const override
    {
        world.addEntity(woodDescriptor->getLogEntityDescriptor(), bi.position() + VectorF(0.5), VectorF(0), lock_manager);
    }
};
}
}
}
}

#endif // WOOD_BLOCK_H_INCLUDED
