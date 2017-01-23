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
#ifndef BLOCK_LADDER_H_INCLUDED
#define BLOCK_LADDER_H_INCLUDED

#include "block/builtin/attached_block.h"
#include "block/builtin/attached_block.h"
#include "texture/texture_atlas.h"
#include "render/generate.h"
#include "util/math_constants.h"
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
class GenericLadder : public AttachedBlock
{
public:
    static Mesh makeMesh(TextureDescriptor td,
                         BlockFace attachedToFace,
                         float displacementDistance = 0.05f)
    {
        TextureDescriptor nx, px, ny, py, nz, pz;
        VectorF displacement = VectorF(0);
        switch(attachedToFace)
        {
        case BlockFace::NX:
            nx = td;
            displacement.x = displacementDistance;
            break;
        case BlockFace::PX:
            px = td;
            displacement.x = -displacementDistance;
            break;
        case BlockFace::NY:
            ny = td;
            displacement.y = displacementDistance;
            break;
        case BlockFace::PY:
            py = td;
            displacement.y = -displacementDistance;
            break;
        case BlockFace::NZ:
            nz = td;
            displacement.z = displacementDistance;
            break;
        case BlockFace::PZ:
            pz = td;
            displacement.z = -displacementDistance;
            break;
        }
        Mesh mesh = transform(Transform::translate(displacement),
                              Generate::unitBox(nx, px, ny, py, nz, pz));
        mesh.append(reverse(mesh));
        return std::move(mesh);
    }

protected:
    static std::wstring makeNameWithBlockFace(std::wstring baseName, BlockFace attachedToFace)
    {
        std::wstring retval = baseName;
        retval += L"(attachedToFace=";
        switch(attachedToFace)
        {
        case BlockFace::NX:
            retval += L"NX";
            break;
        case BlockFace::PX:
            retval += L"PX";
            break;
        case BlockFace::NZ:
            retval += L"NZ";
            break;
        case BlockFace::PZ:
            retval += L"PZ";
            break;
        case BlockFace::PY:
            retval += L"PY";
            break;
        default: //     NY
            retval += L"NY";
            break;
        }
        return retval + L")";
    }
    static BlockShape makeShape(BlockFace attachedToFace, float size)
    {
        VectorF minV = VectorF(0), maxV = VectorF(1);
        switch(attachedToFace)
        {
        case BlockFace::NX:
            maxV.x = size;
            break;
        case BlockFace::PX:
            minV.x = 1 - size;
            break;
        case BlockFace::NY:
            maxV.y = size;
            break;
        case BlockFace::PY:
            minV.y = 1 - size;
            break;
        case BlockFace::NZ:
            maxV.z = size;
            break;
        case BlockFace::PZ:
            minV.z = 1 - size;
            break;
        }
        return BlockShape(0.5f * (minV + maxV), 0.5f * (maxV - minV));
    }
    const BlockShape defaultEffectShape;

public:
    GenericLadder(
        std::wstring name,
        BlockShape blockShape,
        TextureDescriptor td,
        BlockFace attachedToFace,
        LightProperties lightProperties = LightProperties(),
        RayCasting::BlockCollisionMask blockRayCollisionMask = RayCasting::BlockCollisionMaskGround)
        : AttachedBlock(name,
                        attachedToFace,
                        blockShape,
                        lightProperties,
                        blockRayCollisionMask,
                        true,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        makeMesh(td, attachedToFace),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        RenderLayer::Opaque),
          defaultEffectShape(makeShape(attachedToFace, 0.7f))
    {
    }
    virtual RayCasting::Collision getRayCollision(const Block &block,
                                                  BlockIterator blockIterator,
                                                  WorldLockManager &lock_manager,
                                                  World &world,
                                                  RayCasting::Ray ray) const override
    {
        if(ray.dimension() != blockIterator.position().d)
            return RayCasting::Collision(world);
        std::tuple<bool, float, BlockFace> collision = ray.getAABoxExitFace(
            (VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1));
        if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
            return RayCasting::Collision(world);
        if(std::get<2>(collision) != attachedToFace)
            return RayCasting::Collision(world);
        return RayCasting::Collision(world,
                                     std::get<1>(collision),
                                     blockIterator.position(),
                                     toBlockFaceOrNone(std::get<2>(collision)));
    }
    virtual Transform getSelectionBoxTransform(const Block &block) const override
    {
        BlockShape shape = makeShape(attachedToFace, 0.1f);
        return Transform::scale(2 * shape.extents)
            .concat(Transform::translate(shape.offset - shape.extents));
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual bool canAttachBlock(Block b,
                                BlockFace attachingFace,
                                Block attachingBlock) const override
    {
        return false;
    }
    virtual float getHardness() const override
    {
        return 0.4f;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Axe *>(tool.descriptor) != nullptr;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual BlockShape getEffectShape(BlockIterator blockIterator,
                                      WorldLockManager &lock_manager) const override
    {
        return defaultEffectShape;
    }
    virtual BlockEffects getEffects() const override
    {
        return BlockEffects::makeClimbable();
    }
};

class Ladder final : public GenericLadder
{
private:
    class LadderInstanceMaker final
    {
        LadderInstanceMaker(const LadderInstanceMaker &) = delete;
        const LadderInstanceMaker &operator=(const LadderInstanceMaker &) = delete;

    private:
        enum_array<Ladder *, BlockFace> ladders;

    public:
        LadderInstanceMaker() : ladders()
        {
            for(BlockFace bf : enum_traits<BlockFace>())
            {
                ladders[bf] = nullptr;
                if(bf != BlockFace::PY && bf != BlockFace::NY)
                    ladders[bf] = new Ladder(bf);
            }
        }
        ~LadderInstanceMaker()
        {
            for(auto v : ladders)
                delete v;
        }
        const Ladder *get(BlockFace bf) const
        {
            return ladders[bf];
        }
    };

private:
    Ladder(BlockFace attachedToFace)
        : GenericLadder(makeNameWithBlockFace(L"builtin.ladder", attachedToFace),
                        BlockShape(),
                        TextureAtlas::Ladder.td(),
                        attachedToFace)
    {
    }

public:
    virtual const AttachedBlock *getDescriptor(BlockFace attachedToFaceIn)
        const override /// @return nullptr if block can't attach to attachedToFaceIn
    {
        return global_instance_maker<LadderInstanceMaker>::getInstance()->get(attachedToFaceIn);
    }
    static const Ladder *pointer(BlockFace attachedToFaceIn = BlockFace::NX)
    {
        return global_instance_maker<LadderInstanceMaker>::getInstance()->get(attachedToFaceIn);
    }
    static BlockDescriptorPointer descriptor(BlockFace attachedToFaceIn = BlockFace::NX)
    {
        return pointer(attachedToFaceIn);
    }
    virtual void onBreak(World &world,
                         Block b,
                         BlockIterator bi,
                         WorldLockManager &lock_manager,
                         Item &tool) const override;
    virtual void onReplace(World &world,
                           Block b,
                           BlockIterator bi,
                           WorldLockManager &lock_manager) const override;

protected:
    virtual void onDisattach(World &world,
                             const Block &block,
                             BlockIterator blockIterator,
                             WorldLockManager &lock_manager,
                             BlockUpdateKind blockUpdateKind) const override;

public:
    virtual void writeBlockData(stream::Writer &writer,
                                BlockDataPointer<BlockData> data) const override
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

#endif // BLOCK_LADDER_H_INCLUDED
