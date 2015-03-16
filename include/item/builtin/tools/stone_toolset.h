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
#ifndef STONE_TOOLSET_H_INCLUDED
#define STONE_TOOLSET_H_INCLUDED

#include "item/builtin/tools/simple_toolset.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"
#include "item/builtin/cobblestone.h"

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
}
}
}
}
}

#endif // STONE_TOOLSET_H_INCLUDED
