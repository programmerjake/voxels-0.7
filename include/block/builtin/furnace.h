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
#ifndef BLOCK_FURNACE_H_INCLUDED
#define BLOCK_FURNACE_H_INCLUDED

#include "block/builtin/full_block.h"
#include "item/item.h"
#include "entity/builtin/tile.h"
#include "util/lock.h"
#include "texture/texture_atlas.h"
#include <cassert>
#include "util/util.h"
#include "util/global_instance_maker.h"
#include "item/builtin/tools/tools.h"
#include "util/logging.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
namespace builtin
{
class FurnaceUi;
}
}

namespace Blocks
{
namespace builtin
{

class Furnace final : public FullBlock
{
    Furnace(const Furnace &) = delete;
    Furnace &operator =(const Furnace &) = delete;
    friend class ui::builtin::FurnaceUi;
private:
    struct FurnaceData final
    {
        std::atomic_bool hasEntity;
        RecursiveMutex lock;
        ItemStack outputStack;
        ItemStack inputStack;
        ItemStack fuelStack;
        float burnTimeLeft = 0.0f;
        float currentElapsedSmeltTime = 0.0f;
        FurnaceData()
            : hasEntity(false),
            lock(),
            outputStack(),
            inputStack(),
            fuelStack()
        {
        }
        unsigned transferToFuelStack(ItemStack &sourceStack, unsigned transferCount)
        {
            if(!sourceStack.good())
                return 0;
            unsigned maxStackCount = sourceStack.item.descriptor->getMaxFurnaceFuelStackCount();
            if(fuelStack.count >= maxStackCount)
                return 0;
            unsigned maxTransferCount = maxStackCount - fuelStack.count;
            if(transferCount > maxTransferCount)
                transferCount = maxTransferCount;
            return fuelStack.transfer(sourceStack, transferCount);
        }
        unsigned transferFromFuelStack(ItemStack &destStack, unsigned transferCount)
        {
            return destStack.transfer(fuelStack, transferCount);
        }
        bool consumeFuel()
        {
            if(!fuelStack.good())
                return false;
            float burnTime = fuelStack.item.descriptor->getFurnaceBurnTime();
            if(burnTime <= 0)
                return false;
            ItemStack temp;
            if(0 == temp.transfer(fuelStack, 1))
                return false;
            burnTimeLeft += burnTime;
            if(!fuelStack.good())
            {
                fuelStack = temp.item.descriptor->getItemAfterBurnInFurnace();
            }
            //getDebugLog() << L"consumeFuel" << postnl;
            return true;
        }
        bool canProgressSmeltIgnoringFuel()
        {
            if(!inputStack.good())
                return false;
            Item smeltedItem = inputStack.item.descriptor->getSmeltedItem(inputStack.item);
            if(!smeltedItem.good())
                return false;
            ItemStack temp = outputStack;
            if(0 == temp.insert(smeltedItem))
                return false;
            return true;
        }
        bool canSmeltIgnoringFuel()
        {
            if(!canProgressSmeltIgnoringFuel())
                return false;
            if(currentElapsedSmeltTime + eps >= inputStack.item.descriptor->getSmeltTime())
                return true;
            return false;
        }
        void smeltItem()
        {
            if(!inputStack.good())
                return;
            Item smeltedItem = inputStack.item.descriptor->getSmeltedItem(inputStack.item);
            if(!smeltedItem.good())
                return;
            if(0 == outputStack.insert(smeltedItem))
                return;
            currentElapsedSmeltTime -= inputStack.item.descriptor->getSmeltTime();
            inputStack.remove(inputStack.item);
            if(!inputStack.good())
                currentElapsedSmeltTime = 0;
            //getDebugLog() << L"smeltItem" << postnl;
        }
        static std::shared_ptr<FurnaceData> read(stream::Reader &reader)
        {
            std::shared_ptr<FurnaceData> retval = std::make_shared<FurnaceData>();
            retval->outputStack = stream::read<ItemStack>(reader);
            retval->inputStack = stream::read<ItemStack>(reader);
            retval->fuelStack = stream::read<ItemStack>(reader);
            retval->burnTimeLeft = stream::read_limited<float32_t>(reader, 0, 100000);
            retval->currentElapsedSmeltTime = stream::read_limited<float32_t>(reader, 0, 100);
            return retval;
        }
        void write(stream::Writer &writer)
        {
            std::unique_lock<RecursiveMutex> lockIt(lock);
            ItemStack outputStack = this->outputStack;
            ItemStack inputStack = this->inputStack;
            ItemStack fuelStack = this->fuelStack;
            float burnTimeLeft = this->burnTimeLeft;
            float currentElapsedSmeltTime = this->currentElapsedSmeltTime;
            lockIt.unlock();

            stream::write<ItemStack>(writer, outputStack);
            stream::write<ItemStack>(writer, inputStack);
            stream::write<ItemStack>(writer, fuelStack);
            stream::write<float32_t>(writer, burnTimeLeft);
            stream::write<float32_t>(writer, currentElapsedSmeltTime);
        }
    };
    struct FurnaceBlockData final : BlockData
    {
        std::shared_ptr<FurnaceData> data;
        FurnaceBlockData(std::shared_ptr<FurnaceData> data)
            : data(data)
        {
        }
    };
    const Entities::builtin::TileEntity *tileEntity;
    const bool burning;
public:
    const BlockFace facing;
private:
    static TextureDescriptor getFaceTexture(BlockFace facing, bool burning, BlockFace face)
    {
        if(facing == face)
        {
            if(burning)
                return TextureAtlas::FurnaceFrontOn.td();
            return TextureAtlas::FurnaceFrontOff.td();
        }
        if(getBlockFaceOutDirection(face).y == 0)
            return TextureAtlas::FurnaceSide.td();
        return TextureAtlas::DispenserDropperPistonFurnaceFrame.td();
    }
    static std::wstring makeName(BlockFace facing, bool burning)
    {
        std::wstring retval = L"builtin.furnace(facing=";
        switch(facing)
        {
        case BlockFace::NX:
            retval += L"NX";
            break;
        case BlockFace::PX:
            retval += L"PX";
            break;
        case BlockFace::NY:
            retval += L"NY";
            break;
        case BlockFace::PY:
            retval += L"PY";
            break;
        case BlockFace::NZ:
            retval += L"NZ";
            break;
        default:
            retval += L"PZ";
            break;
        }
        retval += L",burning=";
        if(burning)
            retval += L"true";
        else
            retval += L"false";
        retval += L")";
        return retval;
    }
    Furnace(BlockFace facing, bool burning)
        : FullBlock(makeName(facing, burning), LightProperties(burning ? Lighting::Lighting::makeArtificialLighting(13) : Lighting(), Lighting::makeMaxLight()), RayCasting::BlockCollisionMaskGround,
                    true, true, true, true, true, true,
                    getFaceTexture(facing, burning, BlockFace::NX), getFaceTexture(facing, burning, BlockFace::PX),
                    getFaceTexture(facing, burning, BlockFace::NY), getFaceTexture(facing, burning, BlockFace::PY),
                    getFaceTexture(facing, burning, BlockFace::NZ), getFaceTexture(facing, burning, BlockFace::PZ),
                    RenderLayer::Opaque),
                    tileEntity(nullptr), burning(burning), facing(facing)
    {
        tileEntity = new Entities::builtin::TileEntity(this, (Entities::builtin::TileEntity::MoveHandlerType)&Furnace::entityMove, (Entities::builtin::TileEntity::AttachHandlerType)&Furnace::entityAttach);
    }
    ~Furnace()
    {
        delete tileEntity;
    }
    void handleSmelt(std::shared_ptr<FurnaceData> data, std::unique_lock<RecursiveMutex> &lockIt, World &world, WorldLockManager &lock_manager, double deltaTime, bool hasFuel) const
    {
        if(!data->canProgressSmeltIgnoringFuel())
        {
            data->currentElapsedSmeltTime = 0;
            return;
        }
        //getDebugLog() << L"currentElapsedSmeltTime = " << data->currentElapsedSmeltTime << L", burnTimeLeft = " << data->burnTimeLeft << postnl;
        if(!hasFuel)
        {
            data->currentElapsedSmeltTime -= static_cast<float>(2 * deltaTime);
            if(data->currentElapsedSmeltTime < 0)
                data->currentElapsedSmeltTime = 0;
            return;
        }
        data->currentElapsedSmeltTime += static_cast<float>(deltaTime);
        while(data->canSmeltIgnoringFuel())
        {
            data->smeltItem();
        }
    }
    void entityMove(Block b, BlockIterator bi, World &world, WorldLockManager &lock_manager, double deltaTime) const
    {
        assert(b.data != nullptr);
        std::shared_ptr<FurnaceData> data = static_cast<FurnaceBlockData *>(b.data.get())->data;
        lock_manager.clear();
        std::unique_lock<RecursiveMutex> lockIt(data->lock);
        if(data->burnTimeLeft < deltaTime)
        {
            double deltaTimeLeft = deltaTime;
            while(data->burnTimeLeft < deltaTimeLeft)
            {
                if(data->burnTimeLeft > 0)
                    handleSmelt(data, lockIt, world, lock_manager, data->burnTimeLeft, true);
                deltaTimeLeft -= data->burnTimeLeft;
                data->burnTimeLeft = 0;
                if(!data->canProgressSmeltIgnoringFuel())
                    break;
                if(!data->consumeFuel())
                    break;
            }
            if(data->burnTimeLeft < deltaTimeLeft)
                handleSmelt(data, lockIt, world, lock_manager, deltaTimeLeft - data->burnTimeLeft, false);
        }
        else
        {
            data->burnTimeLeft -= static_cast<float>(deltaTime);
            handleSmelt(data, lockIt, world, lock_manager, deltaTime, true);
        }
        bool newBurning = (data->burnTimeLeft > 0);
        if(burning != newBurning)
        {
            world.setBlock(bi, lock_manager, Block(descriptor(facing, newBurning), b.lighting, b.data));
        }
    }
    std::atomic_bool *entityAttach(Block b, BlockIterator bi, World &world, WorldLockManager &lock_manager) const
    {
        assert(b.data != nullptr);
        std::shared_ptr<FurnaceData> data = static_cast<FurnaceBlockData *>(b.data.get())->data;
        data->hasEntity = true;
        return &data->hasEntity;
    }
    struct FurnaceMaker final
    {
        enum_array<enum_array<Furnace *, bool>, BlockFace> furnaces;
        FurnaceMaker()
            : furnaces()
        {
            for(BlockFace facing : enum_traits<BlockFace>())
            {
                for(bool v : enum_traits<bool>())
                {
                    furnaces[facing][v] = nullptr;
                    if(getBlockFaceOutDirection(facing).y == 0)
                        furnaces[facing][v] = new Furnace(facing, v);
                }
            }
        }
        ~FurnaceMaker()
        {
            for(auto &i : furnaces)
                for(auto v : i)
                    delete v;
        }
    };
    static const Furnace *pointer(BlockFace facing, bool burning)
    {
        return global_instance_maker<FurnaceMaker>::getInstance()->furnaces[facing][burning];
    }
    static BlockDescriptorPointer descriptor(BlockFace facing, bool burning)
    {
        return pointer(facing, burning);
    }
public:
    static const Furnace *pointer(BlockFace facing = BlockFace::NX)
    {
        return pointer(facing, false);
    }
    static BlockDescriptorPointer descriptor(BlockFace facing = BlockFace::NX)
    {
        return pointer(facing);
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Pickaxe *>(tool.descriptor) != nullptr;
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual bool onUse(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const override;
    virtual float getHardness() const override
    {
        return 3.5f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Wood;
    }
    virtual void tick(World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind kind) const override
    {
        if(block.data == nullptr)
        {
            world.setBlock(blockIterator, lock_manager, Block(this, block.lighting, BlockDataPointer<FurnaceBlockData>(new FurnaceBlockData(std::make_shared<FurnaceData>()))));
            return;
        }
        std::shared_ptr<FurnaceData> data = static_cast<FurnaceBlockData *>(block.data.get())->data;
        if(tileEntity && !data->hasEntity.exchange(true))
        {
            tileEntity->addToWorld(world, lock_manager, blockIterator.position(), block, &data->hasEntity);
        }
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> blockData) const override
    {
        stream::write<bool>(writer, blockData == nullptr);
        if(blockData == nullptr)
            return;
        std::shared_ptr<FurnaceData> data = static_cast<FurnaceBlockData *>(blockData.get())->data;
        data->write(writer);
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        bool isNull = stream::read<bool>(reader);
        if(isNull)
            return nullptr;
        std::shared_ptr<FurnaceData> furnaceData = FurnaceData::read(reader);
        assert(furnaceData);
        return BlockDataPointer<FurnaceBlockData>(new FurnaceBlockData(furnaceData));
    }
};

}
}
}
}

#endif // BLOCK_FURNACE_H_INCLUDED
