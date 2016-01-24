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
#ifndef ITEM_ORES_H_INCLUDED
#define ITEM_ORES_H_INCLUDED

#include "block/builtin/ores.h"
#include "texture/texture_atlas.h"
#include "item/builtin/block.h"
#include "item/builtin/minerals.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{

class CoalOre final : public ItemBlock
{
    friend class global_instance_maker<CoalOre>;
private:
    CoalOre()
        : ItemBlock(L"builtin.coal_ore", TextureAtlas::CoalOre.td(), Blocks::builtin::CoalOre::descriptor())
    {
    }
public:
    static const CoalOre *pointer()
    {
        return global_instance_maker<CoalOre>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item getSmeltedItem(Item item) const
    {
        return Item(Coal::descriptor());
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class IronOre final : public ItemBlock
{
    friend class global_instance_maker<IronOre>;
private:
    IronOre()
        : ItemBlock(L"builtin.iron_ore", TextureAtlas::IronOre.td(), Blocks::builtin::IronOre::descriptor())
    {
    }
public:
    static const IronOre *pointer()
    {
        return global_instance_maker<IronOre>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item getSmeltedItem(Item item) const
    {
        return Item(IronIngot::descriptor());
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class GoldOre final : public ItemBlock
{
    friend class global_instance_maker<GoldOre>;
private:
    GoldOre()
        : ItemBlock(L"builtin.gold_ore", TextureAtlas::GoldOre.td(), Blocks::builtin::GoldOre::descriptor())
    {
    }
public:
    static const GoldOre *pointer()
    {
        return global_instance_maker<GoldOre>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item getSmeltedItem(Item item) const
    {
        return Item(GoldIngot::descriptor());
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class LapisLazuliOre final : public ItemBlock
{
    friend class global_instance_maker<LapisLazuliOre>;
private:
    LapisLazuliOre()
        : ItemBlock(L"builtin.lapis_lazuli_ore", TextureAtlas::LapisLazuliOre.td(), Blocks::builtin::LapisLazuliOre::descriptor())
    {
    }
public:
    static const LapisLazuliOre *pointer()
    {
        return global_instance_maker<LapisLazuliOre>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item getSmeltedItem(Item item) const
    {
        return Item(LapisLazuli::descriptor());
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class DiamondOre final : public ItemBlock
{
    friend class global_instance_maker<DiamondOre>;
private:
    DiamondOre()
        : ItemBlock(L"builtin.diamond_ore", TextureAtlas::DiamondOre.td(), Blocks::builtin::DiamondOre::descriptor())
    {
    }
public:
    static const DiamondOre *pointer()
    {
        return global_instance_maker<DiamondOre>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item getSmeltedItem(Item item) const
    {
        return Item(Diamond::descriptor());
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class RedstoneOre final : public ItemBlock
{
    friend class global_instance_maker<RedstoneOre>;
private:
    RedstoneOre()
        : ItemBlock(L"builtin.redstone_ore", TextureAtlas::RedstoneOre.td(), Blocks::builtin::RedstoneOre::descriptor())
    {
    }
public:
    static const RedstoneOre *pointer()
    {
        return global_instance_maker<RedstoneOre>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item getSmeltedItem(Item item) const
    {
        return Item(RedstoneDust::descriptor());
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class EmeraldOre final : public ItemBlock
{
    friend class global_instance_maker<EmeraldOre>;
private:
    EmeraldOre()
        : ItemBlock(L"builtin.emerald_ore", TextureAtlas::EmeraldOre.td(), Blocks::builtin::EmeraldOre::descriptor())
    {
    }
public:
    static const EmeraldOre *pointer()
    {
        return global_instance_maker<EmeraldOre>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item getSmeltedItem(Item item) const
    {
        return Item(Emerald::descriptor());
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};


}
}
}
}

#endif // ITEM_ORES_H_INCLUDED
