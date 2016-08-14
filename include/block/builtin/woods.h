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
#ifndef BLOCK_BUILTIN_WOOD_H_INCLUDED
#define BLOCK_BUILTIN_WOOD_H_INCLUDED

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
    static std::vector<TreeDescriptorPointer> makeTreeDescriptors();

private:
    Oak()
        : SimpleWood(L"builtin.oak",
                     TextureAtlas::WoodEnd.td(),
                     TextureAtlas::OakWood.td(),
                     TextureAtlas::OakPlank.td(),
                     TextureAtlas::OakSapling.td(),
                     TextureAtlas::OakLeaves.td(),
                     TextureAtlas::OakLeavesBlocked.td(),
                     makeTreeDescriptors())
    {
    }
};
class Birch : public SimpleWood
{
    friend class global_instance_maker<Birch>;

public:
    static const Birch *pointer()
    {
        return global_instance_maker<Birch>::getInstance();
    }
    static WoodDescriptorPointer descriptor()
    {
        return pointer();
    }
    static std::vector<TreeDescriptorPointer> makeTreeDescriptors();

private:
    Birch()
        : SimpleWood(L"builtin.birch",
                     TextureAtlas::WoodEnd.td(),
                     TextureAtlas::BirchWood.td(),
                     TextureAtlas::BirchPlank.td(),
                     TextureAtlas::BirchSapling.td(),
                     TextureAtlas::BirchLeaves.td(),
                     TextureAtlas::BirchLeavesBlocked.td(),
                     makeTreeDescriptors())
    {
    }
};
class Spruce : public SimpleWood
{
    friend class global_instance_maker<Spruce>;

public:
    static const Spruce *pointer()
    {
        return global_instance_maker<Spruce>::getInstance();
    }
    static WoodDescriptorPointer descriptor()
    {
        return pointer();
    }
    static std::vector<TreeDescriptorPointer> makeTreeDescriptors();

private:
    Spruce()
        : SimpleWood(L"builtin.spruce",
                     TextureAtlas::WoodEnd.td(),
                     TextureAtlas::SpruceWood.td(),
                     TextureAtlas::SprucePlank.td(),
                     TextureAtlas::SpruceSapling.td(),
                     TextureAtlas::SpruceLeaves.td(),
                     TextureAtlas::SpruceLeavesBlocked.td(),
                     makeTreeDescriptors())
    {
    }
};
class Jungle : public SimpleWood
{
    friend class global_instance_maker<Jungle>;

public:
    static const Jungle *pointer()
    {
        return global_instance_maker<Jungle>::getInstance();
    }
    static WoodDescriptorPointer descriptor()
    {
        return pointer();
    }
    static std::vector<TreeDescriptorPointer> makeTreeDescriptors();

private:
    Jungle()
        : SimpleWood(L"builtin.jungle",
                     TextureAtlas::WoodEnd.td(),
                     TextureAtlas::JungleWood.td(),
                     TextureAtlas::JunglePlank.td(),
                     TextureAtlas::JungleSapling.td(),
                     TextureAtlas::JungleLeaves.td(),
                     TextureAtlas::JungleLeavesBlocked.td(),
                     makeTreeDescriptors())
    {
    }
};
}
}
}
}

#endif // BLOCK_BUILTIN_WOOD_H_INCLUDED
