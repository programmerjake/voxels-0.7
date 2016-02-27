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
#ifndef BLOCK_GLASS_H_INCLUDED
#define BLOCK_GLASS_H_INCLUDED

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
class Glass final : public FullBlock
{
    friend class global_instance_maker<Glass>;

private:
    Glass()
        : FullBlock(L"builtin.glass",
                    LightProperties(),
                    RayCasting::BlockCollisionMaskGround,
                    false,
                    false,
                    false,
                    false,
                    false,
                    false)
    {
        meshFace[BlockFace::NX] = makeFaceMeshNX(TextureAtlas::Glass.td());
        meshFace[BlockFace::PX] = makeFaceMeshPX(TextureAtlas::Glass.td());
        meshFace[BlockFace::NY] = makeFaceMeshNY(TextureAtlas::Glass.td());
        meshFace[BlockFace::PY] = makeFaceMeshPY(TextureAtlas::Glass.td());
        meshFace[BlockFace::NZ] = makeFaceMeshNZ(TextureAtlas::Glass.td());
        meshFace[BlockFace::PZ] = makeFaceMeshPZ(TextureAtlas::Glass.td());
    }

public:
    static const Glass *pointer()
    {
        return global_instance_maker<Glass>::getInstance();
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
    virtual void onBreak(World &world,
                         Block b,
                         BlockIterator bi,
                         WorldLockManager &lock_manager,
                         Item &tool) const override;
    virtual float getHardness() const override
    {
        return 0.3f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return true;
    }
    virtual void writeBlockData(stream::Writer &writer,
                                BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void renderDynamic(
        const Block &block,
        Mesh &dest,
        BlockIterator blockIterator,
        WorldLockManager &lock_manager,
        RenderLayer rl,
        const enum_array<BlockLighting, BlockFaceOrNone> &lighting) const override
    {
        if(rl != RenderLayer::Opaque)
        {
            return;
        }
        bool drewAny = false;
        Transform tform = Transform::translate((VectorF)blockIterator.position());
        Mesh &blockMesh = getTempRenderMesh(lock_manager.tls);
        Mesh &faceMesh = getTempRenderMesh2(lock_manager.tls);
        blockMesh.clear();
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            BlockIterator i = blockIterator;
            i.moveToward(bf, lock_manager.tls);
            Block b = i.get(lock_manager);

            if(!b)
            {
                continue;
            }

            if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)] || b.descriptor == this)
            {
                continue;
            }
            drewAny = true;
            if(meshFace[bf].empty())
                continue;
            faceMesh.clear();
            faceMesh.append(meshFace[bf]);
            lightMesh(faceMesh, lighting[toBlockFaceOrNone(bf)], getBlockFaceInDirection(bf));
            blockMesh.append(faceMesh);
        }

        if(drewAny)
        {
            dest.append(transform(tform, blockMesh));
        }
    }
    virtual bool drawsAnythingDynamic(const Block &block,
                                      BlockIterator blockIterator,
                                      WorldLockManager &lock_manager) const override
    {
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            BlockIterator i = blockIterator;
            i.moveToward(bf, lock_manager.tls);
            Block b = i.get(lock_manager);

            if(!b)
            {
                continue;
            }

            if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)] || b.descriptor == this)
            {
                continue;
            }

            return true;
        }
        return false;
    }
};
}
}
}
}

#endif // BLOCK_GLASS_H_INCLUDED
