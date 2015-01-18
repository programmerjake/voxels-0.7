/*
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
#ifndef ENTITIES_BUILTIN_ITEMS_STONE_H_INCLUDED
#define ENTITIES_BUILTIN_ITEMS_STONE_H_INCLUDED

#include "entity/builtin/block_item.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"

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
class Stone final : public BlockItem
{
    friend class global_instance_maker<Stone>;
private:
    Stone()
        : BlockItem(L"builtin.items.stone", TextureAtlas::Stone.td())
    {
    }
public:
    static const Stone *descriptor()
    {
        return global_instance_maker<Stone>::getInstance();
    }
};
}
}
}
}
}

#endif // ENTITIES_BUILTIN_ITEMS_STONE_H_INCLUDED
