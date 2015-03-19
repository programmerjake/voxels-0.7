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
#ifndef BLOCK_TORCH_H_INCLUDED
#define BLOCK_TORCH_H_INCLUDED

#include "block/builtin/attached_block.h"
#include "texture/texture_atlas.h"
#include "render/generate.h"
#include "util/math_constants.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{

class GenericTorch : public AttachedBlock
{
public:
    const float torchHeight, torchWidth;
    static Mesh makeMesh(TextureDescriptor bottomTexture, TextureDescriptor sideTexture, TextureDescriptor topTexture, float torchHeight = 10.0f / 16.0f, float torchWidth = 2.0f / 16.0f)
    {
        Mesh mesh = transform(Matrix::translate(-0.5f, 0, -0.5f).concat(Matrix::scale(torchWidth, torchHeight, torchWidth)),
                              Generate::unitBox(TextureDescriptor(), TextureDescriptor(),
                                                bottomTexture, topTexture,
                                                TextureDescriptor(), TextureDescriptor()));
        mesh.append(transform(Matrix::translate(-0.5f * torchWidth, 0, -0.5f),
                              Generate::unitBox(sideTexture, TextureDescriptor(),
                                                TextureDescriptor(), TextureDescriptor(),
                                                TextureDescriptor(), TextureDescriptor())));
        mesh.append(transform(Matrix::translate(-1.0f + 0.5f * torchWidth, 0, -0.5f),
                              Generate::unitBox(TextureDescriptor(), sideTexture,
                                                TextureDescriptor(), TextureDescriptor(),
                                                TextureDescriptor(), TextureDescriptor())));
        mesh.append(transform(Matrix::translate(-0.5f, 0, -0.5f * torchWidth),
                              Generate::unitBox(TextureDescriptor(), TextureDescriptor(),
                                                TextureDescriptor(), TextureDescriptor(),
                                                sideTexture, TextureDescriptor())));
        mesh.append(transform(Matrix::translate(-0.5f, 0, -1.0f + 0.5f * torchWidth),
                              Generate::unitBox(TextureDescriptor(), TextureDescriptor(),
                                                TextureDescriptor(), TextureDescriptor(),
                                                TextureDescriptor(), sideTexture)));
        return std::move(mesh);
    }
    static Matrix makeTorchBlockTransform(BlockFace attachedToFace, float torchHeight = 10.0f / 16.0f, float torchWidth = 2.0f / 16.0f)
    {
        const double rotateAngle = 1.2 - M_PI / 2.0;
        switch(attachedToFace)
        {
        case BlockFace::NX:
            return Matrix::rotateZ(rotateAngle).concat(Matrix::translate(0, 0.5f - 0.5f * torchHeight, 0.5f));
        case BlockFace::PX:
            return Matrix::rotateZ(-rotateAngle).concat(Matrix::translate(1, 0.5f - 0.5f * torchHeight, 0.5f));
        case BlockFace::PZ:
            return Matrix::rotateX(rotateAngle).concat(Matrix::translate(0.5f, 0.5f - 0.5f * torchHeight, 1));
        case BlockFace::NZ:
            return Matrix::rotateX(-rotateAngle).concat(Matrix::translate(0.5f, 0.5f - 0.5f * torchHeight, 0));
        case BlockFace::PY:
            return Matrix::rotateX(M_PI).concat(Matrix::translate(0.5f, 1.0f, 0.5f));
        default: // NY
            return Matrix::translate(0.5f, 0, 0.5f);
        }
    }
    static VectorF makeTorchHeadPosition(float torchHeight = 10.0f / 16.0f, float torchWidth = 2.0f / 16.0f)
    {
        return VectorF(0, torchHeight, 0);
    }
    static VectorF makeTorchBlockHeadPosition(BlockFace attachedToFace, float torchHeight = 10.0f / 16.0f, float torchWidth = 2.0f / 16.0f)
    {
        return transform(makeTorchBlockTransform(attachedToFace, torchHeight, torchWidth), makeTorchHeadPosition(torchHeight, torchWidth));
    }
    static Mesh makeBlockMesh(TextureDescriptor bottomTexture, TextureDescriptor sideTexture, TextureDescriptor topTexture, BlockFace attachedToFace, float torchHeight = 10.0f / 16.0f, float torchWidth = 2.0f / 16.0f)
    {
        Mesh mesh = makeMesh(bottomTexture, sideTexture, topTexture, torchHeight, torchWidth);
        Matrix tform = makeTorchBlockTransform(attachedToFace, torchHeight, torchWidth);
        return transform(tform, std::move(mesh));
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
        case BlockFace::PX:
            retval += L"PX";
        case BlockFace::NZ:
            retval += L"NZ";
        case BlockFace::PZ:
            retval += L"PZ";
        case BlockFace::PY:
            retval += L"PY";
        default: //     NY
            retval += L"NY";
        }
        return retval + L")";
    }
    const std::pair<VectorF, VectorF> rayCollisionBox;
    static std::pair<VectorF, VectorF> makeRayCollisionBox(Matrix torchBlockTransform, float torchHeight, float torchWidth)
    {
        float minX = -0.5f * torchWidth;
        float maxX = 0.5f * torchWidth;
        float minZ = -0.5f * torchWidth;
        float maxZ = 0.5f * torchWidth;
        float minY = 0;
        float maxY = torchHeight;
        checked_array<VectorF, 8> points =
        {
            VectorF(minX, minY, minZ),
            VectorF(minX, minY, maxZ),
            VectorF(minX, maxY, minZ),
            VectorF(minX, maxY, maxZ),
            VectorF(maxX, minY, minZ),
            VectorF(maxX, minY, maxZ),
            VectorF(maxX, maxY, minZ),
            VectorF(maxX, maxY, maxZ),
        };
        for(VectorF &p : points)
        {
            p = transform(torchBlockTransform, p);
        }
        std::pair<VectorF, VectorF> retval = std::pair<VectorF, VectorF>(points[0], points[0]);
        for(std::size_t i = 1; i < points.size(); i++)
        {
            VectorF p = points[i];
            if(p.x < std::get<0>(retval).x)
                std::get<0>(retval).x = p.x;
            if(p.y < std::get<0>(retval).y)
                std::get<0>(retval).y = p.y;
            if(p.z < std::get<0>(retval).z)
                std::get<0>(retval).z = p.z;
            if(p.x > std::get<1>(retval).x)
                std::get<1>(retval).x = p.x;
            if(p.y > std::get<1>(retval).y)
                std::get<1>(retval).y = p.y;
            if(p.z > std::get<1>(retval).z)
                std::get<1>(retval).z = p.z;
        }
        return retval;
    }
    const VectorF headPosition;
public:
    GenericTorch(std::wstring name,
                 TextureDescriptor bottomTexture, TextureDescriptor sideTexture, TextureDescriptor topTexture,
                 BlockFace attachedToFace, LightProperties lightProperties,
                 RayCasting::BlockCollisionMask blockRayCollisionMask = RayCasting::BlockCollisionMaskGround,
                 float torchHeight = 10.0f / 16.0f, float torchWidth = 2.0f / 16.0f)
        : AttachedBlock(name, attachedToFace, BlockShape(),
                        lightProperties, blockRayCollisionMask, true,
                        false, false,
                        false, false,
                        false, false,
                        makeBlockMesh(bottomTexture, sideTexture, topTexture, attachedToFace, torchHeight, torchWidth),
                        Mesh(), Mesh(),
                        Mesh(), Mesh(),
                        Mesh(), Mesh(),
                        RenderLayer::Opaque),
                        torchHeight(torchHeight), torchWidth(torchWidth),
                        rayCollisionBox(makeRayCollisionBox(makeTorchBlockTransform(attachedToFace, torchHeight, torchWidth), torchHeight, torchWidth)),
                        headPosition(makeTorchBlockHeadPosition(attachedToFace, torchHeight, torchWidth))
    {
    }
    virtual RayCasting::Collision getRayCollision(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, World &world, RayCasting::Ray ray) const override
    {
        if(ray.dimension() != blockIterator.position().d)
            return RayCasting::Collision(world);
        std::pair<VectorF, VectorF> rayCollisionBox = this->rayCollisionBox;
        std::get<0>(rayCollisionBox) += (VectorF)blockIterator.position();
        std::get<1>(rayCollisionBox) += (VectorF)blockIterator.position();
        std::tuple<bool, float, BlockFace> collision = ray.getAABoxEnterFace(std::get<0>(rayCollisionBox), std::get<1>(rayCollisionBox));
        if(std::get<0>(collision))
            collision = ray.getAABoxEnterFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1));
        if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
        {
            collision = ray.getAABoxExitFace(std::get<0>(rayCollisionBox), std::get<1>(rayCollisionBox));
            if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
                return RayCasting::Collision(world);
            std::get<1>(collision) = RayCasting::Ray::eps;
            return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), BlockFaceOrNone::None);
        }
        return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), toBlockFaceOrNone(std::get<2>(collision)));
    }
    virtual Matrix getSelectionBoxTransform(const Block &block) const override
    {
        return Matrix::scale(std::get<1>(rayCollisionBox) - std::get<0>(rayCollisionBox)).concat(Matrix::translate(std::get<0>(rayCollisionBox)));
    }
    virtual bool isReplaceableByFluid() const override
    {
        return true;
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual bool canAttachBlock(Block b, BlockFace attachingFace, Block attachingBlock) const override
    {
        return false;
    }
    virtual float getHardness() const override
    {
        return 0;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return true;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
};

class Torch final : public GenericTorch
{
private:
    class TorchInstanceMaker final
    {
        TorchInstanceMaker(const TorchInstanceMaker &) = delete;
        const TorchInstanceMaker &operator =(const TorchInstanceMaker &) = delete;
    private:
        enum_array<Torch *, BlockFace> torches;
    public:
        TorchInstanceMaker()
        {
            for(BlockFace bf : enum_traits<BlockFace>())
            {
                torches[bf] = nullptr;
                if(bf != BlockFace::PY)
                    torches[bf] = new Torch(bf);
            }
        }
        ~TorchInstanceMaker()
        {
            for(auto v : torches)
                delete v;
        }
        const Torch *get(BlockFace bf) const
        {
            return torches[bf];
        }
    };
private:
    Torch(BlockFace attachedToFace)
        : GenericTorch(makeNameWithBlockFace(L"builtin.torch", attachedToFace), TextureAtlas::TorchBottom.td(), TextureAtlas::TorchSide.td(), TextureAtlas::TorchTop.td(), attachedToFace, LightProperties(Lighting::makeArtificialLighting(14)))
    {
    }
public:
    virtual const AttachedBlock *getDescriptor(BlockFace attachedToFaceIn) const override /// @return nullptr if block can't attach to attachedToFaceIn
    {
        return global_instance_maker<TorchInstanceMaker>::getInstance()->get(attachedToFaceIn);
    }
    static const Torch *pointer(BlockFace attachedToFaceIn = BlockFace::NY)
    {
        return global_instance_maker<TorchInstanceMaker>::getInstance()->get(attachedToFaceIn);
    }
    static BlockDescriptorPointer descriptor(BlockFace attachedToFaceIn = BlockFace::NY)
    {
        return pointer(attachedToFaceIn);
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual void onReplace(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const override;
protected:
    virtual void onDisattach(BlockUpdateSet &blockUpdateSet, World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind blockUpdateKind) const override;
public:
    virtual bool generatesParticles() const override
    {
        return true;
    }
    virtual void generateParticles(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, double currentTime, double deltaTime) const override;
};

}
}
}
}

#endif // BLOCK_TORCH_H_INCLUDED
