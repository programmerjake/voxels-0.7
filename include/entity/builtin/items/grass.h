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
#ifndef ENTITIES_BUILTIN_ITEMS_GRASS_H_INCLUDED
#define ENTITIES_BUILTIN_ITEMS_GRASS_H_INCLUDED

#include "entity/builtin/block_item.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"
#include "render/generate.h"
#include "item/builtin/grass.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
namespace items
{
class Grass final : public BlockItem
{
    friend class Items::builtin::Grass;
    friend class global_instance_maker<Grass>;
private:
    static Mesh makeMesh()
    {
        Mesh retval = Generate::unitBox(TextureAtlas::DirtMask.td(), TextureAtlas::DirtMask.td(),
                                        TextureAtlas::Dirt.td(), TextureDescriptor(),
                                        TextureAtlas::DirtMask.td(), TextureAtlas::DirtMask.td());
        retval.append(colorize(BiomeDescriptor::makeBiomeGrassColor(0.5f, 0.5f), Generate::unitBox(TextureAtlas::GrassMask.td(), TextureAtlas::GrassMask.td(),
                                                                                                    TextureDescriptor(), TextureAtlas::GrassTop.td(),
                                                                                                    TextureAtlas::GrassMask.td(), TextureAtlas::GrassMask.td())));
        return std::move(retval);
    }
    Grass()
        : BlockItem(L"builtin.items.grass", makeMesh())
    {
    }
public:
    static const Grass *descriptor()
    {
        return global_instance_maker<Grass>::getInstance();
    }
protected:
    virtual void onGiveToPlayer(Player &player) const override;
};
}
}
}
}
}

#endif // ENTITIES_BUILTIN_ITEMS_GRASS_H_INCLUDED
