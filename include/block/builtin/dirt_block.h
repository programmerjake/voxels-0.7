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
#ifndef DIRT_BLOCK_H_INCLUDED
#define DIRT_BLOCK_H_INCLUDED

#include "block/builtin/full_block.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class DirtBlock : public FullBlock
{
private:
    enum_array<Mesh, BlockFace> meshGrassFace;
    Mesh meshGrassCenter;
protected:
    DirtBlock(std::wstring name,
              bool isFaceBlockedNX, bool isFaceBlockedPX,
              bool isFaceBlockedNY, bool isFaceBlockedPY,
              bool isFaceBlockedNZ, bool isFaceBlockedPZ,
              TextureDescriptor nxDirt, TextureDescriptor pxDirt,
              TextureDescriptor nyDirt, TextureDescriptor pyDirt,
              TextureDescriptor nzDirt, TextureDescriptor pzDirt,
              TextureDescriptor nxGrass, TextureDescriptor pxGrass,
              TextureDescriptor nyGrass, TextureDescriptor pyGrass,
              TextureDescriptor nzGrass, TextureDescriptor pzGrass)
        : FullBlock(name, LightProperties(Lighting(), Lighting::makeMaxLight()),
                    RayCasting::BlockCollisionMaskGround,
                    isFaceBlockedNX, isFaceBlockedPX,
                    isFaceBlockedNY, isFaceBlockedPY,
                    isFaceBlockedNZ, isFaceBlockedPZ)
    {
        meshFace[BlockFace::NX] = makeFaceMeshNX(nxDirt);
        meshFace[BlockFace::PX] = makeFaceMeshPX(pxDirt);
        meshFace[BlockFace::NY] = makeFaceMeshNY(nyDirt);
        meshFace[BlockFace::PY] = makeFaceMeshPY(pyDirt);
        meshFace[BlockFace::NZ] = makeFaceMeshNZ(nzDirt);
        meshFace[BlockFace::PZ] = makeFaceMeshPZ(pzDirt);
        meshGrassFace[BlockFace::NX] = makeFaceMeshNX(nxGrass);
        meshGrassFace[BlockFace::PX] = makeFaceMeshPX(pxGrass);
        meshGrassFace[BlockFace::NY] = makeFaceMeshNY(nyGrass);
        meshGrassFace[BlockFace::PY] = makeFaceMeshPY(pyGrass);
        meshGrassFace[BlockFace::NZ] = makeFaceMeshNZ(nzGrass);
        meshGrassFace[BlockFace::PZ] = makeFaceMeshPZ(pzGrass);
    }
    DirtBlock(std::wstring name,
              bool areFacesBlocked,
              TextureDescriptor nxDirt, TextureDescriptor pxDirt,
              TextureDescriptor nyDirt, TextureDescriptor pyDirt,
              TextureDescriptor nzDirt, TextureDescriptor pzDirt,
              TextureDescriptor nxGrass, TextureDescriptor pxGrass,
              TextureDescriptor nyGrass, TextureDescriptor pyGrass,
              TextureDescriptor nzGrass, TextureDescriptor pzGrass)
        : DirtBlock(name,
                    areFacesBlocked, areFacesBlocked,
                    areFacesBlocked, areFacesBlocked,
                    areFacesBlocked, areFacesBlocked,
                    nxDirt, pxDirt,
                    nyDirt, pyDirt,
                    nzDirt, pzDirt,
                    nxGrass, pxGrass,
                    nyGrass, pyGrass,
                    nzGrass, pzGrass)
    {
    }
    virtual ColorF getGrassShading(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
        return blockIterator.getBiomeProperties(lock_manager).getGrassColor();
    }
    virtual void renderDynamic(const Block &block, Mesh &dest, BlockIterator blockIterator, WorldLockManager &lock_manager, RenderLayer rl, const BlockLighting &lighting) const override
    {
        if(rl != RenderLayer::Opaque)
        {
            return;
        }
        bool drewAny = false;
        Matrix tform = Matrix::translate((VectorF)blockIterator.position());
        Mesh &blockMesh = getTempRenderMesh();
        blockMesh.clear();
        ColorF grassShading;
        bool gotGrassShading = false;
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            BlockIterator i = blockIterator;
            i.moveToward(bf);
            Block b = i.get(lock_manager);

            if(!b)
            {
                continue;
            }

            if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)])
            {
                continue;
            }
            if(!gotGrassShading)
            {
                gotGrassShading = true;
                grassShading = getGrassShading(block, blockIterator, lock_manager);
            }
            blockMesh.append(meshFace[bf]);
            blockMesh.append(colorize(grassShading, meshGrassFace[bf]));
            drewAny = true;
        }

        if(drewAny)
        {
            assert(gotGrassShading);
            blockMesh.append(meshCenter);
            blockMesh.append(colorize(grassShading, meshGrassCenter));
            lightMesh(blockMesh, lighting);
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

            if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)])
            {
                continue;
            }

            if(meshFace[bf].size() == 0 && meshGrassFace[bf].size() == 0)
                continue;

            return true;
        }
        return false;
    }
};
}
}
}
}

#endif // DIRT_BLOCK_H_INCLUDED
