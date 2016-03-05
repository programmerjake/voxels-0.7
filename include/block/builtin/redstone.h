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
#ifndef BLOCK_REDSTONE_H_INCLUDED
#define BLOCK_REDSTONE_H_INCLUDED

#include "block/block.h"
#include "block/builtin/attached_block.h"
#include "block/builtin/torch.h"
#include "texture/texture_atlas.h"
#include "util/enum_traits.h"
#include <sstream>
#include "util/redstone_signal.h"
#include "render/generate.h"
#include "util/global_instance_maker.h"
#include "util/math_constants.h"
#include "world/world.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class RedstoneDust final : public AttachedBlock
{
public:
    enum class EdgeAttachedState : std::uint8_t
    {
        None,
        Bottom,
        BottomAndTop,
        DEFINE_ENUM_LIMITS(None, BottomAndTop)
    };
    const unsigned signalStrength;
    const EdgeAttachedState edgeAttachedStateNX;
    const EdgeAttachedState edgeAttachedStatePX;
    const EdgeAttachedState edgeAttachedStateNZ;
    const EdgeAttachedState edgeAttachedStatePZ;
    bool isOnBlockFace(BlockFace outBlockFace) const
    {
        switch(outBlockFace)
        {
        case BlockFace::NX:
            return edgeAttachedStateNX == EdgeAttachedState::BottomAndTop;
        case BlockFace::PX:
            return edgeAttachedStatePX == EdgeAttachedState::BottomAndTop;
        case BlockFace::NY:
            return true;
        case BlockFace::NZ:
            return edgeAttachedStateNZ == EdgeAttachedState::BottomAndTop;
        case BlockFace::PZ:
            return edgeAttachedStatePZ == EdgeAttachedState::BottomAndTop;
        default: // PY
            return false;
        }
    }
    virtual const AttachedBlock *getDescriptor(BlockFace attachedToFaceIn)
        const override /// @return nullptr if block can't attach to attachedToFaceIn
    {
        if(attachedToFaceIn == BlockFace::NY)
            return this;
        return nullptr;
    }

private:
    static std::wstring makeName(EdgeAttachedState v)
    {
        switch(v)
        {
        case EdgeAttachedState::Bottom:
            return L"Bottom";
        case EdgeAttachedState::BottomAndTop:
            return L"BottomAndTop";
        default: // None
            return L"None";
        }
    }
    static std::wstring makeName(unsigned signalStrength,
                                 EdgeAttachedState edgeAttachedStateNX,
                                 EdgeAttachedState edgeAttachedStatePX,
                                 EdgeAttachedState edgeAttachedStateNZ,
                                 EdgeAttachedState edgeAttachedStatePZ)
    {
        std::wostringstream ss;
        ss << L"builtin.redstone_dust(signalStrength=" << signalStrength;
        ss << L",edgeAttachedStateNX=" << makeName(edgeAttachedStateNX);
        ss << L",edgeAttachedStatePX=" << makeName(edgeAttachedStatePX);
        ss << L",edgeAttachedStateNZ=" << makeName(edgeAttachedStateNZ);
        ss << L",edgeAttachedStatePZ=" << makeName(edgeAttachedStatePZ);
        ss << L")";
        return ss.str();
    }
    static ColorF getColorizeColor(unsigned signalStrength)
    {
        return interpolate((float)signalStrength / (float)RedstoneSignalComponent::maxStrength,
                           GrayscaleF(0.5f),
                           GrayscaleF(1.0f));
    }
    static Mesh makeMesh(unsigned signalStrength,
                         EdgeAttachedState edgeAttachedStateNX,
                         EdgeAttachedState edgeAttachedStatePX,
                         EdgeAttachedState edgeAttachedStateNZ,
                         EdgeAttachedState edgeAttachedStatePZ)
    {
        TextureDescriptor nx;
        TextureDescriptor px;
        TextureDescriptor nz;
        TextureDescriptor pz;
        TextureDescriptor ny;
        const TextureDescriptor py = TextureDescriptor();
        if(edgeAttachedStateNX == EdgeAttachedState::BottomAndTop)
            nx = TextureAtlas::RedstoneDust2Across.td();
        if(edgeAttachedStatePX == EdgeAttachedState::BottomAndTop)
            px = TextureAtlas::RedstoneDust2Across.td();
        if(edgeAttachedStateNZ == EdgeAttachedState::BottomAndTop)
            nz = TextureAtlas::RedstoneDust2Across.td();
        if(edgeAttachedStatePZ == EdgeAttachedState::BottomAndTop)
            pz = TextureAtlas::RedstoneDust2Across.td();
        int edgeBits = 0;
        const int edgeBitNX = 0x1;
        const int edgeBitPX = 0x2;
        const int edgeBitNZ = 0x4;
        const int edgeBitPZ = 0x8;
        if(edgeAttachedStateNX != EdgeAttachedState::None)
            edgeBits |= edgeBitNX;
        if(edgeAttachedStatePX != EdgeAttachedState::None)
            edgeBits |= edgeBitPX;
        if(edgeAttachedStateNZ != EdgeAttachedState::None)
            edgeBits |= edgeBitNZ;
        if(edgeAttachedStatePZ != EdgeAttachedState::None)
            edgeBits |= edgeBitPZ;
        double nyRotateAngle;
        switch(edgeBits)
        {
        case edgeBitNX | edgeBitNZ | edgeBitPX | edgeBitPZ:
            ny = TextureAtlas::RedstoneDust4.td();
            nyRotateAngle = 0;
            break;
        case edgeBitNX | edgeBitNZ | edgeBitPX:
            ny = TextureAtlas::RedstoneDust3.td();
            nyRotateAngle = M_PI;
            break;
        case edgeBitNX | edgeBitNZ | edgeBitPZ:
            ny = TextureAtlas::RedstoneDust3.td();
            nyRotateAngle = -M_PI / 2;
            break;
        case edgeBitNX | edgeBitNZ:
            ny = TextureAtlas::RedstoneDust2Corner.td();
            nyRotateAngle = -M_PI / 2;
            break;
        case edgeBitNX | edgeBitPX | edgeBitPZ:
            ny = TextureAtlas::RedstoneDust3.td();
            nyRotateAngle = 0;
            break;
        case edgeBitNX | edgeBitPX:
            ny = TextureAtlas::RedstoneDust2Across.td();
            nyRotateAngle = -M_PI / 2;
            break;
        case edgeBitNX | edgeBitPZ:
            ny = TextureAtlas::RedstoneDust2Corner.td();
            nyRotateAngle = 0;
            break;
        case edgeBitNX:
            ny = TextureAtlas::RedstoneDust1.td();
            nyRotateAngle = 0;
            break;
        case edgeBitNZ | edgeBitPX | edgeBitPZ:
            ny = TextureAtlas::RedstoneDust3.td();
            nyRotateAngle = M_PI / 2;
            break;
        case edgeBitNZ | edgeBitPX:
            ny = TextureAtlas::RedstoneDust2Corner.td();
            nyRotateAngle = M_PI;
            break;
        case edgeBitNZ | edgeBitPZ:
            ny = TextureAtlas::RedstoneDust2Across.td();
            nyRotateAngle = 0;
            break;
        case edgeBitNZ:
            ny = TextureAtlas::RedstoneDust1.td();
            nyRotateAngle = -M_PI / 2;
            break;
        case edgeBitPX | edgeBitPZ:
            ny = TextureAtlas::RedstoneDust2Corner.td();
            nyRotateAngle = M_PI / 2;
            break;
        case edgeBitPX:
            ny = TextureAtlas::RedstoneDust1.td();
            nyRotateAngle = M_PI;
            break;
        case edgeBitPZ:
            ny = TextureAtlas::RedstoneDust1.td();
            nyRotateAngle = M_PI / 2;
            break;
        default:
            ny = TextureAtlas::RedstoneDust0.td();
            nyRotateAngle = 0;
            break;
        }
        Mesh retval = Generate::unitBox(nx, px, TextureDescriptor(), py, nz, pz);
        retval.append(transform(Transform::translate(-0.5f, -0.5f, -0.5f)
                                    .concat(Transform::rotateY(nyRotateAngle))
                                    .concat(Transform::translate(0.5f, 0.5f, 0.5f)),
                                Generate::unitBox(TextureDescriptor(),
                                                  TextureDescriptor(),
                                                  ny,
                                                  TextureDescriptor(),
                                                  TextureDescriptor(),
                                                  TextureDescriptor())));
        retval.append(reverse(retval));
        const float faceOffset = 1.0f / 64.0f;
        Transform tform = Transform::translate(-0.5f, -0.5f, -0.5f)
                              .concat(Transform::scale(1.0f - faceOffset))
                              .concat(Transform::translate(0.5f, 0.5f, 0.5f));
        return colorize(getColorizeColor(signalStrength), transform(tform, std::move(retval)));
    }
    RedstoneDust(unsigned signalStrength,
                 EdgeAttachedState edgeAttachedStateNX,
                 EdgeAttachedState edgeAttachedStatePX,
                 EdgeAttachedState edgeAttachedStateNZ,
                 EdgeAttachedState edgeAttachedStatePZ)
        : AttachedBlock(makeName(signalStrength,
                                 edgeAttachedStateNX,
                                 edgeAttachedStatePX,
                                 edgeAttachedStateNZ,
                                 edgeAttachedStatePZ),
                        BlockFace::NY,
                        BlockShape(nullptr),
                        LightProperties(),
                        RayCasting::BlockCollisionMaskGround,
                        true,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        makeMesh(signalStrength,
                                 edgeAttachedStateNX,
                                 edgeAttachedStatePX,
                                 edgeAttachedStateNZ,
                                 edgeAttachedStatePZ),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        Mesh(),
                        RenderLayer::Opaque),
          signalStrength(signalStrength),
          edgeAttachedStateNX(edgeAttachedStateNX),
          edgeAttachedStatePX(edgeAttachedStatePX),
          edgeAttachedStateNZ(edgeAttachedStateNZ),
          edgeAttachedStatePZ(edgeAttachedStatePZ)
    {
    }
    class RedstoneDustConstructor final
    {
    private:
        checked_array<enum_array<enum_array<enum_array<enum_array<const RedstoneDust *,
                                                                  EdgeAttachedState>,
                                                       EdgeAttachedState>,
                                            EdgeAttachedState>,
                                 EdgeAttachedState>,
                      RedstoneSignalComponent::maxStrength + 1> descriptors;

    public:
        RedstoneDustConstructor() : descriptors()
        {
            for(unsigned signalStrength = 0; signalStrength <= RedstoneSignalComponent::maxStrength;
                signalStrength++)
            {
                for(EdgeAttachedState nx : enum_traits<EdgeAttachedState>())
                {
                    for(EdgeAttachedState px : enum_traits<EdgeAttachedState>())
                    {
                        for(EdgeAttachedState nz : enum_traits<EdgeAttachedState>())
                        {
                            for(EdgeAttachedState pz : enum_traits<EdgeAttachedState>())
                            {
                                descriptors[signalStrength][nx][px][nz][pz] =
                                    new RedstoneDust(signalStrength, nx, px, nz, pz);
                            }
                        }
                    }
                }
            }
        }
        ~RedstoneDustConstructor()
        {
            for(auto &i : descriptors)
            {
                for(auto &j : i)
                {
                    for(auto &k : j)
                    {
                        for(auto &l : k)
                        {
                            for(const RedstoneDust *p : l)
                            {
                                delete p;
                            }
                        }
                    }
                }
            }
        }
        const RedstoneDust *pointer(unsigned signalStrength,
                                    EdgeAttachedState edgeAttachedStateNX,
                                    EdgeAttachedState edgeAttachedStatePX,
                                    EdgeAttachedState edgeAttachedStateNZ,
                                    EdgeAttachedState edgeAttachedStatePZ) const
        {
            return descriptors[signalStrength][edgeAttachedStateNX][edgeAttachedStatePX]
                              [edgeAttachedStateNZ][edgeAttachedStatePZ];
        }
    };

public:
    static const RedstoneDust *pointer(unsigned signalStrength,
                                       EdgeAttachedState edgeAttachedStateNX,
                                       EdgeAttachedState edgeAttachedStatePX,
                                       EdgeAttachedState edgeAttachedStateNZ,
                                       EdgeAttachedState edgeAttachedStatePZ)
    {
        return global_instance_maker<RedstoneDustConstructor>::getInstance()->pointer(
            signalStrength,
            edgeAttachedStateNX,
            edgeAttachedStatePX,
            edgeAttachedStateNZ,
            edgeAttachedStatePZ);
    }
    static BlockDescriptorPointer descriptor(unsigned signalStrength,
                                             EdgeAttachedState edgeAttachedStateNX,
                                             EdgeAttachedState edgeAttachedStatePX,
                                             EdgeAttachedState edgeAttachedStateNZ,
                                             EdgeAttachedState edgeAttachedStatePZ)
    {
        return pointer(signalStrength,
                       edgeAttachedStateNX,
                       edgeAttachedStatePX,
                       edgeAttachedStateNZ,
                       edgeAttachedStatePZ);
    }
    static const RedstoneDust *pointer()
    {
        return pointer(0,
                       EdgeAttachedState::None,
                       EdgeAttachedState::None,
                       EdgeAttachedState::None,
                       EdgeAttachedState::None);
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }

private:
    static RedstoneSignal calculateRedstoneSignal(BlockFace inputThroughBlockFace,
                                                  BlockIterator blockIterator,
                                                  WorldLockManager &lock_manager)
    {
        blockIterator.moveToward(inputThroughBlockFace, lock_manager.tls);
        Block b = blockIterator.get(lock_manager);
        if(!b.good())
            return RedstoneSignal();
        RedstoneSignal retval =
            b.descriptor->getRedstoneSignal(getOppositeBlockFace(inputThroughBlockFace));
        if(b.descriptor->canTransmitRedstoneSignal())
        {
            for(BlockFace bf : enum_traits<BlockFace>())
            {
                if(bf == inputThroughBlockFace)
                    continue;
                BlockIterator bi = blockIterator;
                bi.moveFrom(bf, lock_manager.tls);
                b = bi.get(lock_manager);
                if(b.good())
                {
                    retval |= b.descriptor->getRedstoneSignal(bf).makeWeaker();
                }
            }
        }
        return retval;
    }
    static std::pair<unsigned, EdgeAttachedState> calcOrientationAndSignalStrengthSide(
        BlockIterator blockIterator,
        WorldLockManager &lock_manager,
        BlockFace side,
        bool blockAboveCutsRedstoneDust)
    {
        EdgeAttachedState edgeAttachedState = EdgeAttachedState::None;
        RedstoneSignal redstoneSignal = calculateRedstoneSignal(side, blockIterator, lock_manager);
        unsigned signal = 0;
        BlockIterator bi = blockIterator;
        bi.moveToward(side, lock_manager.tls);
        Block b = bi.get(lock_manager);
        if(b.good())
        {
            const RedstoneDust *sideDescriptor = dynamic_cast<const RedstoneDust *>(b.descriptor);
            if(sideDescriptor != nullptr)
            {
                if(sideDescriptor->signalStrength > signal + 1)
                    signal = sideDescriptor->signalStrength - 1;
                edgeAttachedState = EdgeAttachedState::Bottom;
            }
            else
            {
                signal = redstoneSignal.weakComponent.strength;
                if(!b.descriptor->breaksRedstoneDust())
                {
                    bi.moveTowardNY(lock_manager.tls);
                    b = bi.get(lock_manager);
                    if(b.good())
                    {
                        const RedstoneDust *downDescriptor =
                            dynamic_cast<const RedstoneDust *>(b.descriptor);
                        if(downDescriptor != nullptr)
                        {
                            if(downDescriptor->signalStrength > signal + 1)
                                signal = downDescriptor->signalStrength - 1;
                            edgeAttachedState = EdgeAttachedState::Bottom;
                        }
                    }
                }
            }
        }
        if(redstoneSignal.weakComponent.connected)
            edgeAttachedState = EdgeAttachedState::Bottom;
        if(!blockAboveCutsRedstoneDust)
        {
            bi = blockIterator;
            bi.moveToward(side, lock_manager.tls);
            b = bi.get(lock_manager);
            if(b.good() && b.descriptor->transmitsRedstoneSignalDown())
            {
                bi.moveTowardPY(lock_manager.tls);
                b = bi.get(lock_manager);
                if(b.good())
                {
                    const RedstoneDust *upDescriptor =
                        dynamic_cast<const RedstoneDust *>(b.descriptor);
                    if(upDescriptor != nullptr)
                    {
                        if(upDescriptor->signalStrength > signal + 1)
                            signal = upDescriptor->signalStrength - 1;
                        edgeAttachedState = EdgeAttachedState::BottomAndTop;
                    }
                }
            }
        }
        return std::pair<unsigned, EdgeAttachedState>(signal, edgeAttachedState);
    }

public:
    static const RedstoneDust *calcOrientationAndSignalStrength(BlockIterator blockIterator,
                                                                WorldLockManager &lock_manager)
    {
        BlockIterator bi = blockIterator;
        bi.moveTowardPY(lock_manager.tls);
        Block b = bi.get(lock_manager);
        bool blockAboveCutsRedstoneDust = true;
        if(b.good())
        {
            blockAboveCutsRedstoneDust = b.descriptor->breaksRedstoneDust();
        }
        std::pair<unsigned, EdgeAttachedState> nxState = calcOrientationAndSignalStrengthSide(
            blockIterator, lock_manager, BlockFace::NX, blockAboveCutsRedstoneDust);
        std::pair<unsigned, EdgeAttachedState> pxState = calcOrientationAndSignalStrengthSide(
            blockIterator, lock_manager, BlockFace::PX, blockAboveCutsRedstoneDust);
        std::pair<unsigned, EdgeAttachedState> nzState = calcOrientationAndSignalStrengthSide(
            blockIterator, lock_manager, BlockFace::NZ, blockAboveCutsRedstoneDust);
        std::pair<unsigned, EdgeAttachedState> pzState = calcOrientationAndSignalStrengthSide(
            blockIterator, lock_manager, BlockFace::PZ, blockAboveCutsRedstoneDust);
        unsigned nyState = calculateRedstoneSignal(BlockFace::NY, blockIterator, lock_manager)
                               .weakComponent.strength;
        unsigned pyState = calculateRedstoneSignal(BlockFace::PY, blockIterator, lock_manager)
                               .weakComponent.strength;
        unsigned signalStrength = std::max(std::get<0>(nxState), std::get<0>(pxState));
        signalStrength = std::max(signalStrength, std::get<0>(nzState));
        signalStrength = std::max(signalStrength, std::get<0>(pzState));
        signalStrength = std::max(signalStrength, nyState);
        signalStrength = std::max(signalStrength, pyState);
        EdgeAttachedState edgeAttachedStateNX = std::get<1>(nxState);
        EdgeAttachedState edgeAttachedStatePX = std::get<1>(pxState);
        EdgeAttachedState edgeAttachedStateNZ = std::get<1>(nzState);
        EdgeAttachedState edgeAttachedStatePZ = std::get<1>(pzState);
        return pointer(signalStrength,
                       edgeAttachedStateNX,
                       edgeAttachedStatePX,
                       edgeAttachedStateNZ,
                       edgeAttachedStatePZ);
    }
    virtual bool canAttachBlock(Block b,
                                BlockFace attachingFace,
                                Block attachingBlock) const override
    {
        return false;
    }
    virtual RedstoneSignal getRedstoneSignal(BlockFace outputThroughBlockFace) const override
    {
        return RedstoneSignal(RedstoneSignalComponent(signalStrength),
                              RedstoneSignalComponent(nullptr));
    }
    virtual bool generatesParticles() const override
    {
        return signalStrength > 0;
    }
    virtual void generateParticles(World &world,
                                   Block b,
                                   BlockIterator bi,
                                   WorldLockManager &lock_manager,
                                   double currentTime,
                                   double deltaTime) const override;
    virtual void onReplace(World &world,
                           Block b,
                           BlockIterator bi,
                           WorldLockManager &lock_manager) const override;
    virtual void onBreak(World &world,
                         Block b,
                         BlockIterator bi,
                         WorldLockManager &lock_manager,
                         Item &tool) const override;
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
    virtual bool isReplaceableByFluid() const
    {
        return true;
    }
    virtual void tick(World &world,
                      const Block &block,
                      BlockIterator blockIterator,
                      WorldLockManager &lock_manager,
                      BlockUpdateKind kind) const
    {
        if(kind == BlockUpdateKind::UpdateNotify)
        {
            world.addBlockUpdate(blockIterator,
                                 lock_manager,
                                 BlockUpdateKind::Redstone,
                                 BlockUpdateKindDefaultPeriod(BlockUpdateKind::Redstone));
        }
        else if(kind == BlockUpdateKind::Redstone || kind == BlockUpdateKind::RedstoneDust)
        {
            const RedstoneDust *newDescriptor =
                calcOrientationAndSignalStrength(blockIterator, lock_manager);
            if(newDescriptor != this)
            {
                world.setBlock(blockIterator, lock_manager, Block(newDescriptor, block.lighting));
                const int distanceI = 2;
                for(VectorI delta = VectorI(-distanceI); delta.x <= distanceI; delta.x++)
                {
                    int yDistance = distanceI - std::abs(delta.x);
                    for(delta.y = -yDistance; delta.y <= yDistance; delta.y++)
                    {
                        int zDistance = yDistance - std::abs(delta.y);
                        for(delta.z = -zDistance; delta.z <= zDistance; delta.z++)
                        {
                            BlockIterator bi = blockIterator;
                            bi.moveBy(delta, lock_manager.tls);
                            world.addBlockUpdate(
                                bi,
                                lock_manager,
                                BlockUpdateKind::Redstone,
                                BlockUpdateKindDefaultPeriod(BlockUpdateKind::Redstone));
                            world.addBlockUpdate(
                                bi, lock_manager, BlockUpdateKind::RedstoneDust, 0);
                        }
                    }
                }
            }
            return;
        }
        AttachedBlock::tick(world, block, blockIterator, lock_manager, kind);
    }
    virtual RayCasting::Collision getRayCollision(const Block &block,
                                                  BlockIterator blockIterator,
                                                  WorldLockManager &lock_manager,
                                                  World &world,
                                                  RayCasting::Ray ray) const
    {
        if(ray.dimension() != blockIterator.position().d)
            return RayCasting::Collision(world);
        std::tuple<bool, float, BlockFace> collision = ray.getAABoxExitFace(
            (VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1));
        if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
            return RayCasting::Collision(world);
        if(!isOnBlockFace(std::get<2>(collision)))
            return RayCasting::Collision(world);
        return RayCasting::Collision(world,
                                     std::get<1>(collision),
                                     blockIterator.position(),
                                     toBlockFaceOrNone(std::get<2>(collision)));
    }
    virtual Transform getSelectionBoxTransform(const Block &block) const override
    {
        return Transform::scale(1, 1.0f / 16, 1);
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual void writeBlockData(stream::Writer &writer,
                                BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }

protected:
    virtual void onDisattach(World &world,
                             const Block &block,
                             BlockIterator blockIterator,
                             WorldLockManager &lock_manager,
                             BlockUpdateKind blockUpdateKind) const override;
};

class RedstoneTorch final : public GenericTorch
{
private:
    class RedstoneTorchInstanceMaker final
    {
        RedstoneTorchInstanceMaker(const RedstoneTorchInstanceMaker &) = delete;
        const RedstoneTorchInstanceMaker &operator=(const RedstoneTorchInstanceMaker &) = delete;

    private:
        enum_array<enum_array<RedstoneTorch *, bool>, BlockFace> torches;

    public:
        RedstoneTorchInstanceMaker() : torches()
        {
            for(BlockFace bf : enum_traits<BlockFace>())
            {
                for(bool isOn : enum_traits<bool>())
                {
                    torches[bf][isOn] = nullptr;
                    if(bf != BlockFace::PY)
                        torches[bf][isOn] = new RedstoneTorch(bf, isOn);
                }
            }
        }
        ~RedstoneTorchInstanceMaker()
        {
            for(auto &i : torches)
                for(auto v : i)
                    delete v;
        }
        const RedstoneTorch *get(BlockFace bf, bool isOn) const
        {
            return torches[bf][isOn];
        }
    };

public:
    const bool isOn;

private:
    static std::wstring makeName(BlockFace attachedToFace, bool isOn)
    {
        std::wstring retval = L"builtin.redstone_torch(attachedToFace=";
        switch(attachedToFace)
        {
        case BlockFace::NX:
            retval += L"NX";
            break;
        case BlockFace::PX:
            retval += L"PX";
            break;
        case BlockFace::PY:
            retval += L"PY";
            break;
        case BlockFace::NZ:
            retval += L"NZ";
            break;
        case BlockFace::PZ:
            retval += L"PZ";
            break;
        default:
            retval += L"NY";
            break;
        }
        retval += L",isOn=";
        if(isOn)
            retval += L"true";
        else
            retval += L"false";
        return retval + L")";
    }
    RedstoneTorch(BlockFace attachedToFace, bool isOn)
        : GenericTorch(
              makeName(attachedToFace, isOn),
              isOn ? TextureAtlas::RedstoneTorchBottomOn.td() :
                     TextureAtlas::RedstoneTorchBottomOff.td(),
              isOn ? TextureAtlas::RedstoneTorchSideOn.td() :
                     TextureAtlas::RedstoneTorchSideOff.td(),
              isOn ? TextureAtlas::RedstoneTorchTopOn.td() : TextureAtlas::RedstoneTorchTopOff.td(),
              attachedToFace,
              LightProperties(isOn ? Lighting::makeArtificialLighting(7) : Lighting())),
          isOn(isOn)
    {
    }

public:
    virtual const AttachedBlock *getDescriptor(BlockFace attachedToFaceIn)
        const override /// @return nullptr if block can't attach to attachedToFaceIn
    {
        return global_instance_maker<RedstoneTorchInstanceMaker>::getInstance()->get(
            attachedToFaceIn, isOn);
    }
    static const RedstoneTorch *pointer(BlockFace attachedToFaceIn = BlockFace::NY,
                                        bool isOn = true)
    {
        return global_instance_maker<RedstoneTorchInstanceMaker>::getInstance()->get(
            attachedToFaceIn, isOn);
    }
    static BlockDescriptorPointer descriptor(BlockFace attachedToFaceIn = BlockFace::NY,
                                             bool isOn = true)
    {
        return pointer(attachedToFaceIn, isOn);
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
    virtual bool generatesParticles() const override
    {
        return isOn;
    }
    virtual void generateParticles(World &world,
                                   Block b,
                                   BlockIterator bi,
                                   WorldLockManager &lock_manager,
                                   double currentTime,
                                   double deltaTime) const override;
    virtual RedstoneSignal getRedstoneSignal(BlockFace outputThroughBlockFace) const
    {
        auto signal = RedstoneSignalComponent(isOn ? RedstoneSignalComponent::maxStrength : 0);
        if(outputThroughBlockFace == attachedToFace)
            return RedstoneSignal(RedstoneSignalComponent(0));
        if(outputThroughBlockFace == BlockFace::PY)
            return RedstoneSignal(signal);
        return RedstoneSignal(signal).makeWeaker();
    }
    static const RedstoneTorch *calcSignalStrength(BlockIterator blockIterator,
                                                   WorldLockManager &lock_manager,
                                                   BlockFace attachedToFace)
    {
        return pointer(attachedToFace,
                       calculateRedstoneSignal(attachedToFace, blockIterator, lock_manager)
                               .weakComponent.strength == 0);
    }
    virtual void tick(World &world,
                      const Block &block,
                      BlockIterator blockIterator,
                      WorldLockManager &lock_manager,
                      BlockUpdateKind kind) const
    {
        if(kind == BlockUpdateKind::UpdateNotify)
        {
            world.addBlockUpdate(blockIterator,
                                 lock_manager,
                                 BlockUpdateKind::Redstone,
                                 BlockUpdateKindDefaultPeriod(BlockUpdateKind::Redstone));
        }
        else if(kind == BlockUpdateKind::Redstone)
        {
            const RedstoneTorch *newDescriptor =
                calcSignalStrength(blockIterator, lock_manager, attachedToFace);
            if(newDescriptor != this)
            {
                world.setBlock(blockIterator, lock_manager, Block(newDescriptor, block.lighting));
                addRedstoneBlockUpdates(world, blockIterator, lock_manager, 2);
            }
            return;
        }
        AttachedBlock::tick(world, block, blockIterator, lock_manager, kind);
    }
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

#endif // BLOCK_REDSTONE_H_INCLUDED
