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
#ifndef MINERAL_TOOLSETS_H_INCLUDED
#define MINERAL_TOOLSETS_H_INCLUDED

#include "item/builtin/tools/simple_toolset.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"
#include "item/builtin/cobblestone.h"
#include "item/builtin/minerals.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
namespace tools
{

class StoneToolset final : public SimpleToolset
{
    friend class global_instance_maker<StoneToolset>;
private:
    StoneToolset()
        : SimpleToolset(L"builtin.stone", ToolLevel_Stone, 1.0f / 4.0f, 132, Item(Items::builtin::Cobblestone::descriptor()), TextureAtlas::StonePickaxe.td(), TextureAtlas::StoneAxe.td(), TextureAtlas::StoneShovel.td(), TextureAtlas::StoneHoe.td())
    {
    }
public:
    static const StoneToolset *pointer()
    {
        return global_instance_maker<StoneToolset>::getInstance();
    }
    static const SimpleToolset *descriptor()
    {
        return pointer();
    }
};

class IronToolset final : public SimpleToolset
{
    friend class global_instance_maker<IronToolset>;
private:
    IronToolset()
        : SimpleToolset(L"builtin.iron", ToolLevel_Iron, 1.0f / 6.0f, 251, Item(Items::builtin::IronIngot::descriptor()), TextureAtlas::IronPickaxe.td(), TextureAtlas::IronAxe.td(), TextureAtlas::IronShovel.td(), TextureAtlas::IronHoe.td())
    {
    }
public:
    static const IronToolset *pointer()
    {
        return global_instance_maker<IronToolset>::getInstance();
    }
    static const SimpleToolset *descriptor()
    {
        return pointer();
    }
};

class GoldToolset final : public SimpleToolset
{
    friend class global_instance_maker<GoldToolset>;
private:
    GoldToolset()
        : SimpleToolset(L"builtin.gold", ToolLevel_Wood, 1.0f / 12.0f, 33, Item(Items::builtin::GoldIngot::descriptor()), TextureAtlas::GoldPickaxe.td(), TextureAtlas::GoldAxe.td(), TextureAtlas::GoldShovel.td(), TextureAtlas::GoldHoe.td())
    {
    }
public:
    static const GoldToolset *pointer()
    {
        return global_instance_maker<GoldToolset>::getInstance();
    }
    static const SimpleToolset *descriptor()
    {
        return pointer();
    }
};

class DiamondToolset final : public SimpleToolset
{
    friend class global_instance_maker<DiamondToolset>;
private:
    DiamondToolset()
        : SimpleToolset(L"builtin.diamond", ToolLevel_Diamond, 1.0f / 8.0f, 1562, Item(Items::builtin::Diamond::descriptor()), TextureAtlas::DiamondPickaxe.td(), TextureAtlas::DiamondAxe.td(), TextureAtlas::DiamondShovel.td(), TextureAtlas::DiamondHoe.td())
    {
    }
public:
    static const DiamondToolset *pointer()
    {
        return global_instance_maker<DiamondToolset>::getInstance();
    }
    static const SimpleToolset *descriptor()
    {
        return pointer();
    }
};

}
}
}
}
}

#endif // MINERAL_TOOLSETS_H_INCLUDED
