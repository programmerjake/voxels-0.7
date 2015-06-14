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
#ifndef BLOCK_CHEST_H_INCLUDED
#define BLOCK_CHEST_H_INCLUDED

#include "block/builtin/full_block.h"
#include "item/item.h"
#include <mutex>
#include "texture/texture_atlas.h"
#include <cassert>
#include "util/util.h"
#include "util/global_instance_maker.h"
#include "item/builtin/tools/tools.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
namespace builtin
{
class ChestUi;
}
}

namespace Blocks
{
namespace builtin
{

class Chest final : public FullBlock
{
    Chest(const Chest &) = delete;
    Chest &operator =(const Chest &) = delete;
    friend class ui::builtin::ChestUi;
private:
    struct ChestData final
    {
        std::recursive_mutex lock;
        typedef ItemStackArray<9, 3> ItemsType;
        ItemsType items;
        ChestData()
            : lock(),
            items()
        {
        }
        static std::shared_ptr<ChestData> read(stream::Reader &reader)
        {
            std::shared_ptr<ChestData> retval = std::make_shared<ChestData>();
            retval->items = stream::read<ItemStackArray<9, 3>>(reader);
            return retval;
        }
        void write(stream::Writer &writer)
        {
            std::unique_lock<std::recursive_mutex> lockIt(lock);
            auto items = this->items;
            lockIt.unlock();

            stream::write<ItemStackArray<9, 3>>(writer, items);
        }
    };
    struct ChestBlockData final : BlockData
    {
        std::shared_ptr<ChestData> data;
        ChestBlockData(std::shared_ptr<ChestData> data)
            : data(data)
        {
        }
    };
public:
    const BlockFace facing;
private:
    static TextureDescriptor getFaceTexture(BlockFace facing, BlockFace face)
    {
        if(facing == face)
            return TextureAtlas::ChestFront.td();
        if(getBlockFaceOutDirection(face).y == 0)
            return TextureAtlas::ChestSide.td();
        return TextureAtlas::ChestTop.td();
    }
    static std::wstring makeName(BlockFace facing)
    {
        std::wstring retval = L"builtin.chest(facing=";
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
        retval += L")";
        return retval;
    }
    Chest(BlockFace facing)
        : FullBlock(makeName(facing), LightProperties(Lighting(), Lighting::makeMaxLight()), RayCasting::BlockCollisionMaskGround,
                    true, true, true, true, true, true,
                    getFaceTexture(facing, BlockFace::NX), getFaceTexture(facing, BlockFace::PX),
                    getFaceTexture(facing, BlockFace::NY), getFaceTexture(facing, BlockFace::PY),
                    getFaceTexture(facing, BlockFace::NZ), getFaceTexture(facing, BlockFace::PZ),
                    RenderLayer::Opaque),
                    facing(facing)
    {
    }
    ~Chest()
    {
    }
    struct ChestMaker final
    {
        enum_array<Chest *, BlockFace> chests;
        ChestMaker()
            : chests()
        {
            for(BlockFace facing : enum_traits<BlockFace>())
            {
                chests[facing] = nullptr;
                if(getBlockFaceOutDirection(facing).y == 0)
                    chests[facing] = new Chest(facing);
            }
        }
        ~ChestMaker()
        {
            for(auto v : chests)
                delete v;
        }
    };
public:
    static const Chest *pointer(BlockFace facing = BlockFace::NX)
    {
        return global_instance_maker<ChestMaker>::getInstance()->chests[facing];
    }
    static BlockDescriptorPointer descriptor(BlockFace facing = BlockFace::NX)
    {
        return pointer(facing);
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Axe *>(tool.descriptor) != nullptr;
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual bool onUse(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const override;
    virtual float getHardness() const override
    {
        return 2.5f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual void tick(World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind kind) const override
    {
        if(block.data == nullptr)
        {
            world.setBlock(blockIterator, lock_manager, Block(this, block.lighting, BlockDataPointer<ChestBlockData>(new ChestBlockData(std::make_shared<ChestData>()))));
            return;
        }
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> blockData) const override
    {
        stream::write<bool>(writer, blockData == nullptr);
        if(blockData == nullptr)
            return;
        std::shared_ptr<ChestData> data = static_cast<ChestBlockData *>(blockData.get())->data;
        data->write(writer);
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        bool isNull = stream::read<bool>(reader);
        if(isNull)
            return nullptr;
        std::shared_ptr<ChestData> chestData = ChestData::read(reader);
        assert(chestData);
        return BlockDataPointer<ChestBlockData>(new ChestBlockData(chestData));
    }
};

}
}
}
}

#endif // BLOCK_CHEST_H_INCLUDED
