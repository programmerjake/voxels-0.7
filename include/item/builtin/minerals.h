/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef ITEM_MINERALS_H_INCLUDED
#define ITEM_MINERALS_H_INCLUDED

#include "texture/texture_atlas.h"
#include "item/builtin/image.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
class Coal final : public ItemImage
{
    friend class global_instance_maker<Coal>;

private:
    Coal() : ItemImage(L"builtin.coal", TextureAtlas::Coal.td(), nullptr)
    {
    }

public:
    static const Coal *pointer()
    {
        return global_instance_maker<Coal>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual float getFurnaceBurnTime() const override
    {
        return 80.0f;
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class Charcoal final : public ItemImage
{
    friend class global_instance_maker<Charcoal>;

private:
    Charcoal() : ItemImage(L"builtin.charcoal", TextureAtlas::Charcoal.td(), nullptr)
    {
    }

public:
    static const Charcoal *pointer()
    {
        return global_instance_maker<Charcoal>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual float getFurnaceBurnTime() const override
    {
        return 80.0f;
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class IronIngot final : public ItemImage
{
    friend class global_instance_maker<IronIngot>;

private:
    IronIngot() : ItemImage(L"builtin.iron_ingot", TextureAtlas::IronIngot.td(), nullptr)
    {
    }

public:
    static const IronIngot *pointer()
    {
        return global_instance_maker<IronIngot>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class GoldIngot final : public ItemImage
{
    friend class global_instance_maker<GoldIngot>;

private:
    GoldIngot() : ItemImage(L"builtin.gold_ingot", TextureAtlas::GoldIngot.td(), nullptr)
    {
    }

public:
    static const GoldIngot *pointer()
    {
        return global_instance_maker<GoldIngot>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class Diamond final : public ItemImage
{
    friend class global_instance_maker<Diamond>;

private:
    Diamond() : ItemImage(L"builtin.diamond", TextureAtlas::Diamond.td(), nullptr)
    {
    }

public:
    static const Diamond *pointer()
    {
        return global_instance_maker<Diamond>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class RedstoneDust final : public ItemImage
{
    friend class global_instance_maker<RedstoneDust>;

private:
    RedstoneDust()
        : ItemImage(L"builtin.redstone_dust", TextureAtlas::RedstoneDustItem.td(), nullptr)
    {
    }

public:
    static const RedstoneDust *pointer()
    {
        return global_instance_maker<RedstoneDust>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual Item onUse(Item item,
                       World &world,
                       WorldLockManager &lock_manager,
                       Player &player) const override;
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class Emerald final : public ItemImage
{
    friend class global_instance_maker<Emerald>;

private:
    Emerald() : ItemImage(L"builtin.emerald", TextureAtlas::Emerald.td(), nullptr)
    {
    }

public:
    static const Emerald *pointer()
    {
        return global_instance_maker<Emerald>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class LapisLazuli final : public ItemImage
{
    friend class global_instance_maker<LapisLazuli>;

private:
    LapisLazuli() : ItemImage(L"builtin.lapis_lazuli", TextureAtlas::LapisLazuli.td(), nullptr)
    {
    }

public:
    static const LapisLazuli *pointer()
    {
        return global_instance_maker<LapisLazuli>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
    virtual std::shared_ptr<void> readItemData(stream::Reader &reader) const override
    {
        return nullptr;
    }
    virtual void writeItemData(stream::Writer &writer, std::shared_ptr<void> data) const override
    {
    }
};

class Flint final : public ItemImage
{
    friend class global_instance_maker<Flint>;

private:
    Flint() : ItemImage(L"builtin.flint", TextureAtlas::Flint.td(), nullptr)
    {
    }

public:
    static const Flint *pointer()
    {
        return global_instance_maker<Flint>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
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

#endif // ITEM_MINERALS_H_INCLUDED
