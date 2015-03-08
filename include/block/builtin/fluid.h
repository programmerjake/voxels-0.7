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
#ifndef FLUID_H_INCLUDED
#define FLUID_H_INCLUDED

#include "block/block.h"
#include <sstream>
#include "util/util.h"
#include "texture/texture_descriptor.h"
#include "block/builtin/air.h"
#include "item/item_struct.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class Fluid : public BlockDescriptor
{
public:
    bool isSource() const
    {
        return level == 0;
    }
protected:
    const unsigned level;
    const bool falling;
    virtual unsigned getMaxLevel(Dimension d) const
    {
        return 7;
    }
    virtual const Fluid *getBlockDescriptorForFluidLevel(unsigned newLevel, bool falling) const = 0;
    virtual bool isSameKind(const Fluid *other) const = 0;
    virtual bool canCreateSourceBlocks() const
    {
        return false;
    }
public:
    virtual float getSideHeight(Dimension d) const
    {
        return interpolate<float>((float)level / (float)getMaxLevel(d), 0.95f, 0.01f);
    }
    virtual Item getFilledBucket() const = 0;
private:
    TextureDescriptor td;
    static std::wstring makeName(std::wstring baseName, unsigned level, bool falling)
    {
        std::wostringstream ss;
        ss << baseName << L"(level=" << level << L",falling=" << std::boolalpha << falling << L")";
        return ss.str();
    }
protected:
    const BlockUpdateKind fluidUpdateKind;
protected:
    Fluid(std::wstring baseName, unsigned level, bool falling, LightProperties lightProperties, TextureDescriptor td, BlockUpdateKind fluidUpdateKind)
        : BlockDescriptor(makeName(baseName, level, falling), BlockShape(), lightProperties, RayCasting::BlockCollisionMaskFluid, false, false, false, false, false, false), level(level), falling(falling), td(td), fluidUpdateKind(fluidUpdateKind)
    {
    }
private:
    void getCornersLiquidHeight(float &nxnz, float &nxpz, float &pxnz, float &pxpz, BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
        checked_array<checked_array<checked_array<float, 3>, 2>, 3> fluidHeights;
        for(int dx = -1; dx <= 1; dx++)
        {
            for(int dy = 0; dy <= 1; dy++)
            {
                for(int dz = -1; dz <= 1; dz++)
                {
                    BlockIterator bi = blockIterator;
                    bi.moveBy(VectorI(dx, dy, dz));
                    Block b = bi.get(lock_manager);
                    const Fluid *descriptor = dynamic_cast<const Fluid *>(b.descriptor);
                    if(descriptor == nullptr || !isSameKind(descriptor))
                        fluidHeights[dx + 1][dy][dz + 1] = -1;
                    else
                        fluidHeights[dx + 1][dy][dz + 1] = descriptor->getSideHeight(blockIterator.position().d);
                }
            }
        }
        for(int dx = -1; dx <= 1; dx += 2)
        {
            for(int dz = -1; dz <= 1; dz += 2)
            {
                if(fluidHeights[dx + 1][0][1] < 0 && fluidHeights[1][0][dz + 1] < 0)
                {
                    fluidHeights[dx + 1][0][dz + 1] = -1;
                    fluidHeights[dx + 1][1][dz + 1] = -1;
                }
            }
        }
        checked_array<checked_array<float *, 2>, 2> outputs;
        outputs[0][0] = &nxnz;
        outputs[0][1] = &nxpz;
        outputs[1][0] = &pxnz;
        outputs[1][1] = &pxpz;
        for(int ox = 0; ox <= 1; ox++)
        {
            for(int oz = 0; oz <= 1; oz++)
            {
                float &retval = *outputs[ox][oz];
                retval = 0;
                for(int dx = 0; dx <= 1; dx++)
                {
                    for(int dy = 0; dy <= 1; dy++)
                    {
                        for(int dz = 0; dz <= 1; dz++)
                        {
                            float currentHeight = fluidHeights[ox + dx][dy][oz + dz];
                            if(currentHeight < 0)
                                continue;
                            if(dy > 0)
                            {
                                retval = 1;
                                goto foundHeight;
                            }
                            if(retval < currentHeight)
                                retval = currentHeight;
                        }
                    }
                }
foundHeight:
                ;
            }
        }
    }
    bool shouldDrawBlockFace(BlockIterator blockIterator, WorldLockManager &lock_manager, BlockFace bf) const
    {
        BlockIterator bi = blockIterator;
        bi.moveToward(bf);
        Block b = bi.get(lock_manager);
        if(!b.good())
            return false;
        if(bf == BlockFace::PY)
        {
            float nxnz, nxpz, pxnz, pxpz;
            getCornersLiquidHeight(nxnz, nxpz, pxnz, pxpz, blockIterator, lock_manager);
            if(nxnz < 1 || nxpz < 1 || pxnz < 1 || pxpz < 1)
                return true;
        }
        if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)])
            return false;
        const Fluid *descriptor = dynamic_cast<const Fluid *>(b.descriptor);
        if(descriptor == nullptr || !isSameKind(descriptor))
            return true;
        return false;
    }
protected:
    virtual ColorF getColorizeColor(BlockIterator bi, WorldLockManager &lock_manager) const
    {
        return colorizeIdentity();
    }
    /** generate dynamic mesh
     the generated mesh is at the absolute position of the block
     */
    virtual void renderDynamic(const Block &block, Mesh &dest, BlockIterator blockIterator, WorldLockManager &lock_manager, RenderLayer rl, const enum_array<BlockLighting, BlockFaceOrNone> &lighting) const override
    {
        if(rl != RenderLayer::Translucent)
        {
            return;
        }

        bool drewAny = false;
        ColorF colorizeColor;
        float nxnz = 1, nxpz = 1, pxnz = 1, pxpz = 1;
        Matrix tform = Matrix::translate((VectorF)blockIterator.position());
        Mesh &blockMesh = getTempRenderMesh();
        blockMesh.clear();
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            if(shouldDrawBlockFace(blockIterator, lock_manager, bf))
            {
                if(!drewAny)
                {
                    colorizeColor = getColorizeColor(blockIterator, lock_manager);
                    getCornersLiquidHeight(nxnz, nxpz, pxnz, pxpz, blockIterator, lock_manager);
                }
                drewAny = true;
                const VectorF p0 = VectorF(0, 0, 0);
                const VectorF p1 = VectorF(1, 0, 0);
                const VectorF p2 = VectorF(0, nxnz, 0);
                const VectorF p3 = VectorF(1, pxnz, 0);
                const VectorF p4 = VectorF(0, 0, 1);
                const VectorF p5 = VectorF(1, 0, 1);
                const VectorF p6 = VectorF(0, nxpz, 1);
                const VectorF p7 = VectorF(1, pxpz, 1);
                blockMesh.triangles.reserve(12);
                TextureDescriptor nx = td;
                TextureDescriptor px = td;
                TextureDescriptor ny = td;
                TextureDescriptor py = td;
                TextureDescriptor nz = td;
                TextureDescriptor pz = td;
                switch(bf)
                {
                case BlockFace::NX:
                    blockMesh.append(Generate::quadrilateral(nx,
                                                p0, colorizeColor,
                                                p4, colorizeColor,
                                                p6, colorizeColor,
                                                p2, colorizeColor));
                    break;
                case BlockFace::PX:
                    blockMesh.append(Generate::quadrilateral(px,
                                                p5, colorizeColor,
                                                p1, colorizeColor,
                                                p3, colorizeColor,
                                                p7, colorizeColor));
                    break;
                case BlockFace::NY:
                    blockMesh.append(Generate::quadrilateral(ny,
                                                p0, colorizeColor,
                                                p1, colorizeColor,
                                                p5, colorizeColor,
                                                p4, colorizeColor));
                    break;
                case BlockFace::PY:
                {
                    blockMesh.append(Generate::quadrilateral(py,
                                                p6, colorizeColor,
                                                p7, colorizeColor,
                                                p3, colorizeColor,
                                                p2, colorizeColor));
                    break;
                }
                case BlockFace::NZ:
                    blockMesh.append(Generate::quadrilateral(nz,
                                                p1, colorizeColor,
                                                p0, colorizeColor,
                                                p2, colorizeColor,
                                                p3, colorizeColor));
                    break;
                case BlockFace::PZ:
                    blockMesh.append(Generate::quadrilateral(pz,
                                                p4, colorizeColor,
                                                p5, colorizeColor,
                                                p7, colorizeColor,
                                                p6, colorizeColor));
                    break;
                }
            }
        }
        blockMesh.append(reverse(blockMesh));
        if(drewAny)
        {
            blockMesh.append(meshCenter);
            lightMesh(blockMesh, lighting[BlockFaceOrNone::None], VectorF(0));
            dest.append(transform(tform, blockMesh));
        }
    }
    virtual bool drawsAnythingDynamic(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager) const override
    {
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            if(shouldDrawBlockFace(blockIterator, lock_manager, bf))
                return true;
        }
        return false;
    }
public:
    virtual RayCasting::Collision getRayCollision(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, World &world, RayCasting::Ray ray) const
    {
        float height = getSideHeight(blockIterator.position().d);
        if(ray.dimension() != blockIterator.position().d)
            return RayCasting::Collision(world);
        std::tuple<bool, float, BlockFace> collision = ray.getAABoxEnterFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1, height, 1));
        if(!std::get<0>(collision))
            return RayCasting::Collision(world);
        if(std::get<1>(collision) < RayCasting::Ray::eps)
        {
            collision = ray.getAABoxExitFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1, height, 1));
            if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
                return RayCasting::Collision(world);
            std::get<1>(collision) = RayCasting::Ray::eps;
            return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), BlockFaceOrNone::None);
        }
        return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), toBlockFaceOrNone(std::get<2>(collision)));
    }
private:
    void setBlock(BlockUpdateSet &blockUpdateSet, BlockIterator blockIterator, WorldLockManager &lock_manager, Block newBlock) const
    {
        Block prevBlock = blockIterator.get(lock_manager);
        newBlock.lighting = prevBlock.lighting;
        if(prevBlock != newBlock)
        {
            blockUpdateSet.emplace_back(blockIterator.position(), newBlock);
        }
    }
    bool getIsFalling(BlockIterator blockIterator, WorldLockManager &lock_manager, unsigned newLevel) const
    {
        BlockIterator bi = blockIterator;
        bi.moveTowardNY();
        Block b = bi.get(lock_manager);
        if(!b.good())
            return false;
        const Fluid *bd = dynamic_cast<const Fluid *>(b.descriptor);
        if(bd != nullptr)
        {
            if(isSameKind(bd))
            {
                return bd->level > 0 || newLevel > 0;
            }
            return false;
        }
        return b.descriptor->isReplaceable();
    }
public:
    virtual void tick(BlockUpdateSet &blockUpdateSet, World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind kind) const
    {
        if(kind == BlockUpdateKind::FluidUpdateNotify)
        {
            for(BlockUpdateIterator i = blockIterator.updatesBegin(lock_manager); i != blockIterator.updatesEnd(lock_manager); i++)
            {
                if(i->getKind() == fluidUpdateKind)
                {
                    if(i->getTimeLeft() < BlockUpdateKindDefaultPeriod(fluidUpdateKind))
                    {
                        return;
                    }
                }
            }
            world.rescheduleBlockUpdate(blockIterator, lock_manager, fluidUpdateKind, BlockUpdateKindDefaultPeriod(fluidUpdateKind));
            return;
        }
        if(kind == fluidUpdateKind)
        {
            unsigned maxLevel = getMaxLevel(blockIterator.position().d);
            bool isFalling = getIsFalling(blockIterator, lock_manager, level);
            BlockIterator bi = blockIterator;
            Block b;
            unsigned newLevel = level;
            if(level > 0)
            {
                bi = blockIterator;
                bi.moveTowardPY();
                b = bi.get(lock_manager);
                if(isSameKind(dynamic_cast<const Fluid *>(b.descriptor)))
                {
                    newLevel = 1;
                    setBlock(blockUpdateSet, blockIterator, lock_manager, Block(getBlockDescriptorForFluidLevel(newLevel, isFalling)));
                }
                else
                {
                    unsigned minLevel = maxLevel;
                    const Fluid *bd;
                    bi = blockIterator;
                    bi.moveTowardNX();
                    b = bi.get(lock_manager);
                    bd = dynamic_cast<const Fluid *>(b.descriptor);
                    if(b.good() && isSameKind(bd))
                    {
                        if(!bd->falling && bd->level < minLevel)
                        {
                            minLevel = bd->level;
                        }
                    }
                    bi = blockIterator;
                    bi.moveTowardPX();
                    b = bi.get(lock_manager);
                    bd = dynamic_cast<const Fluid *>(b.descriptor);
                    if(b.good() && isSameKind(bd))
                    {
                        if(!bd->falling && bd->level < minLevel)
                        {
                            minLevel = bd->level;
                        }
                    }
                    bi = blockIterator;
                    bi.moveTowardNZ();
                    b = bi.get(lock_manager);
                    bd = dynamic_cast<const Fluid *>(b.descriptor);
                    if(b.good() && isSameKind(bd))
                    {
                        if(!bd->falling && bd->level < minLevel)
                        {
                            minLevel = bd->level;
                        }
                    }
                    bi = blockIterator;
                    bi.moveTowardPZ();
                    b = bi.get(lock_manager);
                    bd = dynamic_cast<const Fluid *>(b.descriptor);
                    if(b.good() && isSameKind(bd))
                    {
                        if(!bd->falling && bd->level < minLevel)
                        {
                            minLevel = bd->level;
                        }
                    }
                    newLevel = minLevel + 1;
                    if(newLevel > maxLevel)
                        setBlock(blockUpdateSet, blockIterator, lock_manager, Block(Air::descriptor()));
                    else
                        setBlock(blockUpdateSet, blockIterator, lock_manager, Block(getBlockDescriptorForFluidLevel(newLevel, isFalling)));
                }
            }
            else
            {
                setBlock(blockUpdateSet, blockIterator, lock_manager, Block(getBlockDescriptorForFluidLevel(newLevel, isFalling)));
            }
            if(!isFalling && newLevel < maxLevel)
            {
                bi = blockIterator;
                bi.moveTowardNX();
                b = bi.get(lock_manager);
                if(b.good() && dynamic_cast<const Fluid *>(b.descriptor) == nullptr && b.descriptor->isReplaceable())
                {
                    setBlock(blockUpdateSet, bi, lock_manager, Block(getBlockDescriptorForFluidLevel(newLevel + 1, getIsFalling(bi, lock_manager, newLevel + 1))));
                }
                bi = blockIterator;
                bi.moveTowardPX();
                b = bi.get(lock_manager);
                if(b.good() && dynamic_cast<const Fluid *>(b.descriptor) == nullptr && b.descriptor->isReplaceable())
                {
                    setBlock(blockUpdateSet, bi, lock_manager, Block(getBlockDescriptorForFluidLevel(newLevel + 1, getIsFalling(bi, lock_manager, newLevel + 1))));
                }
                bi = blockIterator;
                bi.moveTowardNZ();
                b = bi.get(lock_manager);
                if(b.good() && dynamic_cast<const Fluid *>(b.descriptor) == nullptr && b.descriptor->isReplaceable())
                {
                    setBlock(blockUpdateSet, bi, lock_manager, Block(getBlockDescriptorForFluidLevel(newLevel + 1, getIsFalling(bi, lock_manager, newLevel + 1))));
                }
                bi = blockIterator;
                bi.moveTowardPZ();
                b = bi.get(lock_manager);
                if(b.good() && dynamic_cast<const Fluid *>(b.descriptor) == nullptr && b.descriptor->isReplaceable())
                {
                    setBlock(blockUpdateSet, bi, lock_manager, Block(getBlockDescriptorForFluidLevel(newLevel + 1, getIsFalling(bi, lock_manager, newLevel + 1))));
                }
            }
            bi = blockIterator;
            bi.moveTowardNY();
            b = bi.get(lock_manager);
            if(b.good() && dynamic_cast<const Fluid *>(b.descriptor) == nullptr && b.descriptor->isReplaceable())
            {
                setBlock(blockUpdateSet, bi, lock_manager, Block(getBlockDescriptorForFluidLevel(1, getIsFalling(bi, lock_manager, 1))));
            }
        }
    }
    virtual bool isReplaceable() const override
    {
        return true;
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
};
}
}
}
}

#endif // FLUID_H_INCLUDED
