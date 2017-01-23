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
#ifndef ATTACHED_BLOCK_H_INCLUDED
#define ATTACHED_BLOCK_H_INCLUDED

#include "block/block.h"
#include "util/enum_traits.h"
#include "util/block_face.h"
#include "util/three_state_bool.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class AttachedBlock : public BlockDescriptor
{
public:
    const BlockFace attachedToFace;
    virtual const AttachedBlock *getDescriptor(BlockFace attachedToFaceIn)
        const = 0; /// @return nullptr if block can't attach to attachedToFaceIn
    AttachedBlock(std::wstring name,
                  BlockFace attachedToFace,
                  BlockShape blockShape,
                  LightProperties lightProperties,
                  RayCasting::BlockCollisionMask blockRayCollisionMask,
                  bool isFaceBlockedNX,
                  bool isFaceBlockedPX,
                  bool isFaceBlockedNY,
                  bool isFaceBlockedPY,
                  bool isFaceBlockedNZ,
                  bool isFaceBlockedPZ)
        : BlockDescriptor(name,
                          blockShape,
                          lightProperties,
                          blockRayCollisionMask,
                          isFaceBlockedNX,
                          isFaceBlockedPX,
                          isFaceBlockedNY,
                          isFaceBlockedPY,
                          isFaceBlockedNZ,
                          isFaceBlockedPZ),
          attachedToFace(attachedToFace)
    {
    }
    AttachedBlock(std::wstring name,
                  BlockFace attachedToFace,
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
        : BlockDescriptor(name,
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
                          meshCenter,
                          meshFaceNX,
                          meshFacePX,
                          meshFaceNY,
                          meshFacePY,
                          meshFaceNZ,
                          meshFacePZ,
                          staticRenderLayer),
          attachedToFace(attachedToFace)
    {
        handledUpdateKinds[BlockUpdateKind::UpdateNotify] = true;
    }

protected:
    ThreeStateBool isAttached(Block b, BlockIterator bi, WorldLockManager &lock_manager) const
    {
        bi.moveToward(attachedToFace, lock_manager);
        Block attachedToBlock = bi.get(lock_manager);
        if(!attachedToBlock.good())
            return ThreeStateBool::Unknown;
        return toThreeStateBool(attachedToBlock.descriptor->canAttachBlock(
            attachedToBlock, getOppositeBlockFace(attachedToFace), b));
    }
    virtual void onDisattach(World &world,
                             const Block &block,
                             BlockIterator blockIterator,
                             WorldLockManager &lock_manager,
                             BlockUpdateKind blockUpdateKind) const = 0;

public:
    virtual void tick(World &world,
                      const Block &block,
                      BlockIterator blockIterator,
                      WorldLockManager &lock_manager,
                      BlockUpdateKind kind) const override
    {
        if(kind == BlockUpdateKind::UpdateNotify
           && !toBool(isAttached(block, blockIterator, lock_manager), true))
        {
            onDisattach(world, block, blockIterator, lock_manager, kind);
        }
    }
    virtual bool canPlace(Block b, BlockIterator bi, WorldLockManager &lock_manager) const override
    {
        return toBool(isAttached(b, bi, lock_manager), false);
    }
};
}
}
}
}


#endif // ATTACHED_BLOCK_H_INCLUDED
