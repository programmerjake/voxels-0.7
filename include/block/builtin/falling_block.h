/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef FALLING_BLOCK_H_INCLUDED
#define FALLING_BLOCK_H_INCLUDED

#include "block/block.h"
#include "entity/builtin/falling_block.h"
#include "block/builtin/air.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class FallingBlock : public BlockDescriptor
{
public:
    FallingBlock(std::wstring name,
                 BlockShape blockShape,
                 LightProperties lightProperties,
                 RayCasting::BlockCollisionMask blockRayCollisionMask,
                 bool isStaticMesh,
                 bool isFaceBlockedNX,
                 bool isFaceBlockedPX,
                 bool isFaceBlockedNY,
                 bool isFaceBlockedPY,
                 bool isFaceBlockedNZ,
                 bool isFaceBlockedPZ,
                 Mesh meshCenter,
                 Mesh meshFaceNX,
                 Mesh meshFacePX,
                 Mesh meshFaceNY,
                 Mesh meshFacePY,
                 Mesh meshFaceNZ,
                 Mesh meshFacePZ,
                 RenderLayer staticRenderLayer)
        : BlockDescriptor(std::move(name),
                          blockShape,
                          lightProperties,
                          blockRayCollisionMask,
                          isStaticMesh,
                          isFaceBlockedNX,
                          isFaceBlockedPX,
                          isFaceBlockedNY,
                          isFaceBlockedPY,
                          isFaceBlockedNZ,
                          isFaceBlockedPZ,
                          std::move(meshCenter),
                          std::move(meshFaceNX),
                          std::move(meshFacePX),
                          std::move(meshFaceNY),
                          std::move(meshFacePY),
                          std::move(meshFaceNZ),
                          std::move(meshFacePZ),
                          staticRenderLayer)
    {
        handledUpdateKinds[BlockUpdateKind::UpdateNotify] = true;
    }
    FallingBlock(std::wstring name,
                 BlockShape blockShape,
                 LightProperties lightProperties,
                 RayCasting::BlockCollisionMask blockRayCollisionMask,
                 bool isFaceBlockedNX,
                 bool isFaceBlockedPX,
                 bool isFaceBlockedNY,
                 bool isFaceBlockedPY,
                 bool isFaceBlockedNZ,
                 bool isFaceBlockedPZ)
        : BlockDescriptor(std::move(name),
                          blockShape,
                          lightProperties,
                          blockRayCollisionMask,
                          isFaceBlockedNX,
                          isFaceBlockedPX,
                          isFaceBlockedNY,
                          isFaceBlockedPY,
                          isFaceBlockedNZ,
                          isFaceBlockedPZ)
    {
        handledUpdateKinds[BlockUpdateKind::UpdateNotify] = true;
    }

protected:
    virtual Entity *makeFallingBlockEntity(World &world,
                                           PositionF position,
                                           WorldLockManager &lock_manager) const = 0;

public:
    virtual void tick(World &world,
                      const Block &block,
                      BlockIterator blockIterator,
                      WorldLockManager &lock_manager,
                      BlockUpdateKind kind) const
    {
        if(kind == BlockUpdateKind::UpdateNotify)
        {
            BlockIterator bi = blockIterator;
            bi.moveTowardNY(lock_manager);
            Block b = bi.get(lock_manager);
            if(b.good() && b.descriptor->isReplaceableByFallingBlock())
            {
                world.setBlock(blockIterator, lock_manager, Block(Air::descriptor()));
                makeFallingBlockEntity(
                    world, blockIterator.position() + VectorF(0.5f), lock_manager);
            }
        }
    }
};
}
}
}
}

#endif // FALLING_BLOCK_H_INCLUDED
