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
#ifndef BLOCK_UPDATE_H_INCLUDED
#define BLOCK_UPDATE_H_INCLUDED

#include "util/position.h"
#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{
struct BlockUpdateKind : public enum_struct<BlockUpdateKind, std::uint8_t>
{
    constexpr explicit BlockUpdateKind(base_type value)
        : enum_struct(value)
    {
    }
    BlockUpdateKind() = default;
    static constexpr BlockUpdateKind Lighting() {return BlockUpdateKind(0);}
    static constexpr BlockUpdateKind General() {return BlockUpdateKind(1);}
    float defaultPeriod() const
    {
        switch((base_type)*this)
        {
        case (base_type)Lighting():
            return 0;
        case (base_type)General():
            return 0.05;
        }
        assert(false);
        return 0;
    }
    DEFINE_ENUM_STRUCT_LIMITS(Lighting, General)
};
}
}

#endif // BLOCK_UPDATE_H_INCLUDED
