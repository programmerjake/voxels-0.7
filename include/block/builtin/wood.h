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
#include "render/render_settings.h"
#include "item/item.h"
#include "item/builtin/tools/tools.h"

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
        case LogOrientation::Y:
        case LogOrientation::Z:
            logVector = VectorI(0, 1, 0);
            break;
        }
        if(dot(faceVector, logVector) == 0)
            return woodDescriptor->getLogSideTexture();
        return woodDescriptor->getLogTopTexture();
    }
    static BlockFace vectorToBlockFace(VectorF v)
    {
        if(std::fabs(v.x) > std::fabs(v.y) && std::fabs(v.x) > std::fabs(v.z))
        {
            if(v.x < 0)
                return BlockFace::NX;
            return BlockFace::PX;
        }
        if(std::fabs(v.y) > std::fabs(v.z))
        {
            if(v.y < 0)
                return BlockFace::NY;
            return BlockFace::PY;
        }
        if(v.z < 0)
            return BlockFace::NZ;
        return BlockFace::PZ;
    }
    void transformBlock(Matrix tform)
    {
        tform = Matrix::translate(-0.5f, -0.5f, -0.5f).concat(tform).concat(Matrix::translate(0.5f, 0.5f, 0.5f));
        enum_array<Mesh, BlockFace> newMeshFaces;
        for(BlockFace srcBF : enum_traits<BlockFace>())
        {
            BlockFace destBF = vectorToBlockFace(tform.applyNoTranslate(getBlockFaceOutDirection(srcBF)));
            newMeshFaces[destBF] = transform(tform, std::move(meshFace[srcBF]));
        }
        meshFace = std::move(newMeshFaces);
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
        switch(logOrientation)
        {
        case LogOrientation::AllBark:
        case LogOrientation::Y:
            break;
        case LogOrientation::X:
            transformBlock(Matrix::rotateZ(M_PI / 2));
            break;
        case LogOrientation::Z:
            transformBlock(Matrix::rotateX(M_PI / 2));
            break;
        }
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
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(woodDescriptor->getLogItemDescriptor())), bi.position() + VectorF(0.5));
    }
    virtual float getHardness() const override
    {
        return 2.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Axe *>(tool.descriptor) != nullptr;
    }
};
class WoodPlanks : public FullBlock
{
private:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor)
    {
        std::wstring retval = L"builtin.wood_planks(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L")";
        return retval;
    }
public:
    WoodPlanks(WoodDescriptorPointer woodDescriptor)
        : FullBlock(makeName(woodDescriptor), LightProperties(Lighting(), Lighting::makeMaxLight()), RayCasting::BlockCollisionMaskGround,
                    true, woodDescriptor->getPlanksTexture(), RenderLayer::Opaque), woodDescriptor(woodDescriptor)
    {
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
    virtual bool isReplaceable() const override
    {
        return false;
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(woodDescriptor->getPlanksItemDescriptor())), bi.position() + VectorF(0.5));
    }
    virtual float getHardness() const override
    {
        return 2.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Axe *>(tool.descriptor) != nullptr;
    }
};
class WoodLeaves : public FullBlock
{
private:
    const WoodDescriptorPointer woodDescriptor;
    const bool canDecay;
protected:
    enum_array<Mesh, BlockFace> meshBlockedFace;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor, bool canDecay)
    {
        std::wstring retval = L"builtin.wood_leaves(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L",canDecay=";
        if(canDecay)
            retval += L"true";
        else
            retval += L"false";
        retval += L")";
        return retval;
    }
public:
    WoodLeaves(WoodDescriptorPointer woodDescriptor, bool canDecay)
        : FullBlock(makeName(woodDescriptor, canDecay), LightProperties(Lighting(), Lighting::makeDirectOnlyLighting()), RayCasting::BlockCollisionMaskGround,
                    false, false, false, false, false, false), woodDescriptor(woodDescriptor), canDecay(canDecay)
    {
        TextureDescriptor td = woodDescriptor->getLeavesTexture();
        meshFace[BlockFace::NX] = makeFaceMeshNX(td);
        meshFace[BlockFace::PX] = makeFaceMeshPX(td);
        meshFace[BlockFace::NY] = makeFaceMeshNY(td);
        meshFace[BlockFace::PY] = makeFaceMeshPY(td);
        meshFace[BlockFace::NZ] = makeFaceMeshNZ(td);
        meshFace[BlockFace::PZ] = makeFaceMeshPZ(td);
        td = woodDescriptor->getBlockedLeavesTexture();
        meshBlockedFace[BlockFace::NX] = makeFaceMeshNX(td);
        meshBlockedFace[BlockFace::PX] = makeFaceMeshPX(td);
        meshBlockedFace[BlockFace::NY] = makeFaceMeshNY(td);
        meshBlockedFace[BlockFace::PY] = makeFaceMeshPY(td);
        meshBlockedFace[BlockFace::NZ] = makeFaceMeshNZ(td);
        meshBlockedFace[BlockFace::PZ] = makeFaceMeshPZ(td);
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
    bool getCanDecay() const
    {
        return canDecay;
    }
    virtual bool isReplaceable() const override
    {
        return false;
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(woodDescriptor->getLeavesItemDescriptor())), bi.position() + VectorF(0.5));
    }
    virtual ColorF getLeavesShading(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
        return blockIterator.getBiomeProperties(lock_manager).getLeavesColor();
    }
    virtual void renderDynamic(const Block &block, Mesh &dest, BlockIterator blockIterator, WorldLockManager &lock_manager, RenderLayer rl, const enum_array<BlockLighting, BlockFaceOrNone> &lighting) const override
    {
        if(rl != RenderLayer::Opaque)
        {
            return;
        }
        bool drewAny = false;
        ColorF leavesShading;
        Matrix tform = Matrix::translate((VectorF)blockIterator.position());
        Mesh &blockMesh = getTempRenderMesh();
        Mesh &faceMesh = getTempRenderMesh2();
        blockMesh.clear();
        const enum_array<Mesh, BlockFace> &currentMeshFace = *(globalRenderSettings.useFancyLeaves ? &meshFace : &meshBlockedFace);
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            BlockIterator i = blockIterator;
            i.moveToward(bf);
            Block b = i.get(lock_manager);

            if(!b)
            {
                continue;
            }

            if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)] || (!globalRenderSettings.useFancyLeaves && b.descriptor == this))
            {
                continue;
            }
            drewAny = true;
            leavesShading = getLeavesShading(block, blockIterator, lock_manager);
            if(currentMeshFace[bf].size() == 0)
                continue;
            faceMesh.clear();
            faceMesh.append(colorize(leavesShading, currentMeshFace[bf]));
            lightMesh(faceMesh, lighting[toBlockFaceOrNone(bf)], getBlockFaceInDirection(bf));
            blockMesh.append(faceMesh);
        }

        if(drewAny)
        {
            dest.append(transform(tform, blockMesh));
        }
    }
    virtual bool drawsAnythingDynamic(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager) const override
    {
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            BlockIterator i = blockIterator;
            i.moveToward(bf);
            Block b = i.get(lock_manager);

            if(!b)
            {
                continue;
            }

            if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)] || (!globalRenderSettings.useFancyLeaves && b.descriptor == this))
            {
                continue;
            }

            return true;
        }
        return false;
    }
    #warning add leaves decaying
    virtual float getHardness() const override
    {
        return 0.2f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return false;
        #warning add check for shears
    }
};
}
}
}
}

#endif // WOOD_BLOCK_H_INCLUDED
