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
#ifndef BUILTIN_WOOD_H_INCLUDED
#define BUILTIN_WOOD_H_INCLUDED

#include "util/wood_descriptor.h"
#include "texture/texture_atlas.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Woods
{
namespace builtin
{
class Oak : public SimpleWood
{
    friend class global_instance_maker<Oak>;
public:
    static const Oak *pointer()
    {
        return global_instance_maker<Oak>::getInstance();
    }
    static WoodDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Oak()
        : SimpleWood(L"builtin.oak", TextureAtlas::WoodEnd.td(), TextureAtlas::OakWood.td(), TextureAtlas::OakPlank.td(), TextureAtlas::OakSapling.td(), TextureAtlas::OakLeaves.td(), std::vector<TreeDescriptorPointer>())
    {
        #warning add tree descriptors
    }
};
}
}
}
}

#endif // BUILTIN_WOOD_H_INCLUDED
