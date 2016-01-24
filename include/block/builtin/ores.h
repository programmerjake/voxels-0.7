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
#ifndef BLOCK_ORES_H_INCLUDED
#define BLOCK_ORES_H_INCLUDED

#include "block/builtin/stone_block.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{

class CoalOre final : public StoneBlock
{
    friend class global_instance_maker<CoalOre>;
public:
    static const CoalOre *pointer()
    {
        return global_instance_maker<CoalOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    CoalOre()
        : StoneBlock(L"builtin.coal_ore", TextureAtlas::CoalOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Wood;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class IronOre final : public StoneBlock
{
    friend class global_instance_maker<IronOre>;
public:
    static const IronOre *pointer()
    {
        return global_instance_maker<IronOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    IronOre()
        : StoneBlock(L"builtin.iron_ore", TextureAtlas::IronOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Stone;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class GoldOre final : public StoneBlock
{
    friend class global_instance_maker<GoldOre>;
public:
    static const GoldOre *pointer()
    {
        return global_instance_maker<GoldOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    GoldOre()
        : StoneBlock(L"builtin.gold_ore", TextureAtlas::GoldOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Iron;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class RedstoneOre final : public StoneBlock
{
    friend class global_instance_maker<RedstoneOre>;
public:
    static const RedstoneOre *pointer()
    {
        return global_instance_maker<RedstoneOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    RedstoneOre()
        : StoneBlock(L"builtin.redstone_ore", TextureAtlas::RedstoneOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual bool onStartAttack(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const override;
    virtual bool onUse(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, std::shared_ptr<Player> player) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Iron;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class LitRedstoneOre final : public StoneBlock
{
    friend class global_instance_maker<LitRedstoneOre>;
public:
    static const LitRedstoneOre *pointer()
    {
        return global_instance_maker<LitRedstoneOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    LitRedstoneOre()
        : StoneBlock(L"builtin.lit_redstone_ore", TextureAtlas::ActiveRedstoneOre.td(), LightProperties(Lighting::makeArtificialLighting(9), Lighting::makeMaxLight()))
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual void randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Iron;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class DiamondOre final : public StoneBlock
{
    friend class global_instance_maker<DiamondOre>;
public:
    static const DiamondOre *pointer()
    {
        return global_instance_maker<DiamondOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    DiamondOre()
        : StoneBlock(L"builtin.diamond_ore", TextureAtlas::DiamondOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Iron;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class EmeraldOre final : public StoneBlock
{
    friend class global_instance_maker<EmeraldOre>;
public:
    static const EmeraldOre *pointer()
    {
        return global_instance_maker<EmeraldOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    EmeraldOre()
        : StoneBlock(L"builtin.emerald_ore", TextureAtlas::EmeraldOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Iron;
    }
    virtual void writeBlockData(stream::Writer &writer, BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};

class LapisLazuliOre final : public StoneBlock
{
    friend class global_instance_maker<LapisLazuliOre>;
public:
    static const LapisLazuliOre *pointer()
    {
        return global_instance_maker<LapisLazuliOre>::getInstance();
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    LapisLazuliOre()
        : StoneBlock(L"builtin.lapis_lazuli_ore", TextureAtlas::LapisLazuliOre.td())
    {
    }
public:
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override;
    virtual float getHardness() const override
    {
        return 3.0f;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_Iron;
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

#endif // BLOCK_ORES_H_INCLUDED
