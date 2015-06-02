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
#include "block/builtin/plant.h"
#include <cassert>

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
    WoodLog(const WoodLog &) = delete;
    WoodLog &operator =(const WoodLog &) = delete;
protected:
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
    void transformBlock(Matrix tform)
    {
        tform = Matrix::translate(-0.5f, -0.5f, -0.5f).concat(tform).concat(Matrix::translate(0.5f, 0.5f, 0.5f));
        enum_array<Mesh, BlockFace> newMeshFaces;
        for(BlockFace srcBF : enum_traits<BlockFace>())
        {
            BlockFace destBF = getBlockFaceFromOutVector(tform.applyNoTranslate(getBlockFaceOutDirection(srcBF)));
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
        handleToolDamage(tool);
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
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class WoodPlanks : public FullBlock
{
    WoodPlanks(const WoodPlanks &) = delete;
    WoodPlanks &operator =(const WoodPlanks &) = delete;
protected:
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
        handleToolDamage(tool);
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
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class WoodLeaves : public FullBlock
{
    WoodLeaves(const WoodLeaves &) = delete;
    WoodLeaves &operator =(const WoodLeaves &) = delete;
protected:
    const WoodDescriptorPointer woodDescriptor;
    const bool canDecay;
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
    virtual bool isConnectedToTree(BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
        const int checkDistance = 4;
        checked_array<checked_array<checked_array<checked_array<int, 2 * checkDistance + 1>, 2 * checkDistance + 1>, 2 * checkDistance + 1>, 2> supportedArrays;
        for(auto &i : supportedArrays)
        {
            for(auto &j : i)
            {
                for(auto &k : j)
                {
                    for(auto &v : k)
                    {
                        v = -1;
                    }
                }
            }
        }
        checked_array<checked_array<checked_array<int, 2 * checkDistance + 1>, 2 * checkDistance + 1>, 2 * checkDistance + 1> propagates;
        for(auto &i : propagates)
        {
            for(auto &j : i)
            {
                for(auto &v : j)
                {
                    v = -1;
                }
            }
        }
        int currentSupportedArrayIndex = 0;
        for(int dx = -checkDistance; dx <= checkDistance; dx++)
        {
            for(int dy = -checkDistance; dy <= checkDistance; dy++)
            {
                for(int dz = -checkDistance; dz <= checkDistance; dz++)
                {
                    BlockIterator bi = blockIterator;
                    bi.moveBy(VectorI(dx, dy, dz), lock_manager.tls);
                    Block b = bi.get(lock_manager);
                    int &currentSupported = supportedArrays[currentSupportedArrayIndex][dx + checkDistance][dy + checkDistance][dz + checkDistance];
                    int &currentPropagates = propagates[dx + checkDistance][dy + checkDistance][dz + checkDistance];
                    currentSupported = 0;
                    currentPropagates = 0;
                    if(!b.good()) // might be log
                    {
                        currentSupported = 1;
                        currentPropagates = 1;
                    }
                    else if(dynamic_cast<const WoodLeaves *>(b.descriptor) != nullptr)
                    {
                        currentPropagates = 1;
                    }
                    else if(dynamic_cast<const WoodLog *>(b.descriptor) != nullptr)
                    {
                        currentPropagates = 1;
                        currentSupported = 1;
                    }
                }
            }
        }
        for(int i = 0; i < checkDistance; i++)
        {
            currentSupportedArrayIndex = 1 - currentSupportedArrayIndex;
            int currentCheckDistance = checkDistance - i - 1;
            for(int x = -currentCheckDistance; x <= currentCheckDistance; x++)
            {
                for(int y = -currentCheckDistance; y <= currentCheckDistance; y++)
                {
                    for(int z = -currentCheckDistance; z <= currentCheckDistance; z++)
                    {
                        int &currentSupported = supportedArrays[currentSupportedArrayIndex][x + checkDistance][y + checkDistance][z + checkDistance];
                        currentSupported = supportedArrays[1 - currentSupportedArrayIndex][x + checkDistance][y + checkDistance][z + checkDistance];
                        assert(currentSupported != -1);
                        int currentPropagetes = propagates[x + checkDistance][y + checkDistance][z + checkDistance];
                        assert(currentPropagetes != -1);
                        if(!currentPropagetes)
                            continue;
                        if(currentSupported)
                            continue;
                        for(BlockFace bf : enum_traits<BlockFace>())
                        {
                            VectorI delta = getBlockFaceOutDirection(bf);
                            VectorI p = VectorI(x, y, z) + delta;
                            int lastSupported = supportedArrays[1 - currentSupportedArrayIndex][p.x + checkDistance][p.y + checkDistance][p.z + checkDistance];
                            assert(lastSupported != -1);
                            if(lastSupported)
                            {
                                currentSupported = 1;
                                break;
                            }
                        }
                    }
                }
            }
        }
        int currentSupported = supportedArrays[currentSupportedArrayIndex][checkDistance][checkDistance][checkDistance];
        assert(currentSupported != -1);
        return currentSupported != 0;
    }
public:
    WoodLeaves(WoodDescriptorPointer woodDescriptor, bool canDecay)
        : FullBlock(makeName(woodDescriptor, canDecay),
                    LightProperties(Lighting(),
                                    Lighting::makeDirectOnlyLighting()),
                    RayCasting::BlockCollisionMaskGround,
                    false, false, false, false, false, false),
        woodDescriptor(woodDescriptor),
        canDecay(canDecay),
        meshBlockedFace()
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
        woodDescriptor->makeLeavesDrops(world, bi, lock_manager, tool);
        handleToolDamage(tool);
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
        Mesh &blockMesh = getTempRenderMesh(lock_manager.tls);
        Mesh &faceMesh = getTempRenderMesh2(lock_manager.tls);
        blockMesh.clear();
        const enum_array<Mesh, BlockFace> &currentMeshFace = *(globalRenderSettings.useFancyLeaves ? &meshFace : &meshBlockedFace);
        for(BlockFace bf : enum_traits<BlockFace>())
        {
            BlockIterator i = blockIterator;
            i.moveToward(bf, lock_manager.tls);
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
            i.moveToward(bf, lock_manager.tls);
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
    virtual void randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const override
    {
        if(!canDecay)
            return;
        if(isConnectedToTree(blockIterator, lock_manager))
            return;
        woodDescriptor->makeLeavesDrops(world, blockIterator, lock_manager, Item());
        world.setBlock(blockIterator, lock_manager, Block(Air::descriptor()));
    }
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
        FIXME_MESSAGE(add check for shears)
    }
    virtual bool canAttachBlock(Block b, BlockFace attachingFace, Block attachingBlock) const override
    {
        return false;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class Sapling : public Plant
{
    Sapling(const Sapling &) = delete;
    Sapling &operator =(const Sapling &) = delete;
protected:
    const WoodDescriptorPointer woodDescriptor;
    static std::wstring makeName(WoodDescriptorPointer woodDescriptor, unsigned frame)
    {
        std::wstring retval = L"builtin.sapling(woodDescriptor=";
        retval += woodDescriptor->name;
        retval += L",growthStage=";
        retval += (frame != 0 ? L"1" : L"0");
        retval += L")";
        return retval;
    }
    virtual const Plant *getPlantFrame(unsigned frame) const override
    {
        BlockDescriptorPointer retval = woodDescriptor->getSaplingBlockDescriptor(frame);
        assert(dynamic_cast<const Plant *>(retval) != nullptr);
        return static_cast<const Plant *>(retval);
    }
    virtual void dropItems(BlockIterator bi, Block b, World &world, WorldLockManager &lock_manager, Item tool) const override
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(Item(woodDescriptor->getSaplingItemDescriptor())), bi.position() + VectorF(0.5));
    }
    virtual bool isPositionValid(BlockIterator blockIterator, Block block, WorldLockManager &lock_manager) const override
    {
        BlockIterator bi = blockIterator;
        bi.moveTowardNY(lock_manager.tls);
        Block b = bi.get(lock_manager);
        if(!b.good())
            return true;
        return b.descriptor->canAttachBlock(b, BlockFace::PY, block);
    }
    int getTreeBaseSize(int maxTreeBaseSize, BlockIterator &blockIterator, WorldLockManager &lock_manager, bool adjustSaplingPosition) const
    {
        if(maxTreeBaseSize < 2)
            return maxTreeBaseSize;
        assert(maxTreeBaseSize == 2);
        checked_array<checked_array<bool, 3>, 3> hasSapling;
        for(int dx = -1; dx <= 1; dx++)
        {
            for(int dz = -1; dz <= 1; dz++)
            {
                BlockIterator bi = blockIterator;
                bi.moveBy(VectorI(dx, 0, dz), lock_manager.tls);
                Block b = bi.get(lock_manager);
                hasSapling[dx + 1][dz + 1] = false;
                if(!b.good())
                    continue;
                const Sapling *sapling = dynamic_cast<const Sapling *>(b.descriptor);
                if(sapling == nullptr)
                    continue;
                if(sapling->woodDescriptor == woodDescriptor)
                    hasSapling[dx + 1][dz + 1] = true;
            }
        }
        for(int growDX = 0; growDX >= -1; growDX--)
        {
            for(int growDZ = 0; growDZ >= -1; growDZ--)
            {
                bool good = true;
                for(int x = 1; x <= 2; x++)
                {
                    for(int z = 1; z <= 2; z++)
                    {
                        if(!hasSapling[growDX + x][growDZ + z])
                        {
                            good = false;
                            break;
                        }
                    }
                    if(!good)
                        break;
                }
                if(good)
                {
                    if(!adjustSaplingPosition && growDX != 0 && growDZ != 0)
                        return 0;
                    blockIterator.moveBy(VectorI(growDX, 0, growDZ), lock_manager.tls);
                    return 2;
                }
            }
        }
        assert(hasSapling[1][1]);
        return 1;
    }
    bool tryGrowTree(World &world, BlockIterator blockIterator, WorldLockManager &lock_manager, bool adjustSaplingPosition) const
    {
        int maxTreeBaseSize = 0;
        for(TreeDescriptorPointer treeDescriptor : woodDescriptor->trees)
        {
            assert(treeDescriptor);
            if(!treeDescriptor->canGenerateFromSapling)
                continue;
            if(maxTreeBaseSize < treeDescriptor->baseSize)
                maxTreeBaseSize = treeDescriptor->baseSize;
        }
        int treeBaseSize = getTreeBaseSize(maxTreeBaseSize, blockIterator, lock_manager, adjustSaplingPosition);
        if(treeBaseSize == 0)
            return false;
        Block block = blockIterator.get(lock_manager);
        if(block.lighting.toFloat(world.getLighting(blockIterator.position().d)) < 9 / 16.0f)
            return false;
        BlockIterator bi = blockIterator;
        bi.moveTowardNY(lock_manager.tls);
        Block b = bi.get(lock_manager);
        if(!b.good())
            return false;
        const DirtBlock *dirtDescriptor = dynamic_cast<const DirtBlock *>(b.descriptor);
        if(dirtDescriptor == nullptr)
            return false;
        if(!dirtDescriptor->canGrowTreeOn())
            return false;
        std::size_t treeGenerateChanceSum = 0;
        for(TreeDescriptorPointer treeDescriptor : woodDescriptor->trees)
        {
            if(!treeDescriptor->canGenerateFromSapling)
                continue;
            if(treeDescriptor->baseSize != treeBaseSize)
                continue;
            treeGenerateChanceSum += treeDescriptor->generateChance;
        }
        if(treeGenerateChanceSum == 0)
            return false;
        std::size_t treeIndex = std::uniform_int_distribution<std::size_t>(0, treeGenerateChanceSum - 1)(world.getRandomGenerator());
        TreeDescriptorPointer treeDescriptor = nullptr;
        for(TreeDescriptorPointer td : woodDescriptor->trees)
        {
            if(!td->canGenerateFromSapling)
                continue;
            if(td->baseSize != treeBaseSize)
                continue;
            if(treeIndex < td->generateChance)
            {
                treeDescriptor = td;
                break;
            }
            treeIndex -= td->generateChance;
        }
        assert(treeDescriptor);
        Tree tree = treeDescriptor->generateTree(std::uniform_int_distribution<std::uint32_t>()(world.getRandomGenerator()));
        if(!tree.placeInWorld(blockIterator.position(), world, lock_manager))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
public:
    Sapling(WoodDescriptorPointer woodDescriptor, unsigned frame)
        : Plant(makeName(woodDescriptor, frame), makeDiagonalCrossMesh(woodDescriptor->getSaplingTexture()), frame, 2, 1.0f), woodDescriptor(woodDescriptor)
    {
    }
    virtual void randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const override
    {
        if(animationFrame + 1 < animationFrameCount)
        {
            Plant::randomTick(block, world, blockIterator, lock_manager);
            return;
        }
        tryGrowTree(world, blockIterator, lock_manager, false);
    }
    WoodDescriptorPointer getWoodDescriptor() const
    {
        return woodDescriptor;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
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

#endif // WOOD_BLOCK_H_INCLUDED
