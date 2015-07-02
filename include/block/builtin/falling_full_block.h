/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef FALLING_FULL_BLOCK_H_INCLUDED
#define FALLING_FULL_BLOCK_H_INCLUDED

#include "block/builtin/falling_block.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class FallingFullBlock : public FallingBlock
{
protected:
    static Mesh makeFaceMeshNX(TextureDescriptor td)
    {
        return Generate::unitBox(td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshPX(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshNY(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshPY(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshNZ(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor());
    }
    static Mesh makeFaceMeshPZ(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td);
    }
protected:
    FallingFullBlock(std::wstring name, LightProperties lightProperties,
              RayCasting::BlockCollisionMask blockRayCollisionMask,
              bool isFaceBlockedNX, bool isFaceBlockedPX,
              bool isFaceBlockedNY, bool isFaceBlockedPY,
              bool isFaceBlockedNZ, bool isFaceBlockedPZ)
        : FallingBlock(name, BlockShape(VectorF(0.5f), VectorF(0.5f)),
                          lightProperties, blockRayCollisionMask,
                          isFaceBlockedNX, isFaceBlockedPX,
                          isFaceBlockedNY, isFaceBlockedPY,
                          isFaceBlockedNZ, isFaceBlockedPZ)
    {
    }
    FallingFullBlock(std::wstring name, LightProperties lightProperties, RayCasting::BlockCollisionMask blockRayCollisionMask, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY, bool isFaceBlockedNZ, bool isFaceBlockedPZ, TextureDescriptor tdNX, TextureDescriptor tdPX, TextureDescriptor tdNY, TextureDescriptor tdPY, TextureDescriptor tdNZ, TextureDescriptor tdPZ, RenderLayer rl)
        : FallingBlock(name, BlockShape(VectorF(0.5f), VectorF(0.5f)), lightProperties, blockRayCollisionMask, true, isFaceBlockedNX, isFaceBlockedPX, isFaceBlockedNY, isFaceBlockedPY, isFaceBlockedNZ, isFaceBlockedPZ, Mesh(), makeFaceMeshNX(tdNX), makeFaceMeshPX(tdPX), makeFaceMeshNY(tdNY), makeFaceMeshPY(tdPY), makeFaceMeshNZ(tdNZ), makeFaceMeshPZ(tdPZ), rl)
    {
    }
    FallingFullBlock(std::wstring name, LightProperties lightProperties, RayCasting::BlockCollisionMask blockRayCollisionMask, bool areFacesBlocked, TextureDescriptor td, RenderLayer rl = RenderLayer::Opaque)
        : FallingFullBlock(name, lightProperties, blockRayCollisionMask, areFacesBlocked, areFacesBlocked, areFacesBlocked, areFacesBlocked, areFacesBlocked, areFacesBlocked, td, td, td, td, td, td, rl)
    {
    }
public:
    virtual RayCasting::Collision getRayCollision(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, World &world, RayCasting::Ray ray) const override
    {
        if(ray.dimension() != blockIterator.position().d)
            return RayCasting::Collision(world);
        std::tuple<bool, float, BlockFace> collision = ray.getAABoxEnterFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1));
        if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
        {
            collision = ray.getAABoxExitFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1));
            if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
                return RayCasting::Collision(world);
            std::get<1>(collision) = RayCasting::Ray::eps;
            return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), BlockFaceOrNone::None);
        }
        return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), toBlockFaceOrNone(std::get<2>(collision)));
    }
    virtual bool isGroundBlock() const override
    {
        return true;
    }
    virtual bool canAttachBlock(Block b, BlockFace attachingFace, Block attachingBlock) const override
    {
        return true;
    }
};
}
}
}
}

#endif // FALLING_FULL_BLOCK_H_INCLUDED
