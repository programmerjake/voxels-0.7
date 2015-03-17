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
#ifndef ITEM_ORES_H_INCLUDED
#define ITEM_ORES_H_INCLUDED

#include "block/builtin/ores.h"
#include "texture/texture_atlas.h"
#include "item/builtin/block.h"

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
};


}
}
}
}

#endif // ITEM_ORES_H_INCLUDED
