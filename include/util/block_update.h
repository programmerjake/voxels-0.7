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
#ifndef BLOCK_UPDATE_H_INCLUDED
#define BLOCK_UPDATE_H_INCLUDED

#include "util/position.h"
#include "util/enum_traits.h"
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
enum class BlockUpdateKind : std::uint8_t
{
    Lighting,
    General,
    UpdateNotify,
    Water,
    Redstone,
    RedstoneDust,
    DEFINE_ENUM_LIMITS(Lighting, RedstoneDust)
};

inline float BlockUpdateKindDefaultPeriod(BlockUpdateKind buKind)
{
    switch(buKind)
    {
    case BlockUpdateKind::Lighting:
        return 0;
    case BlockUpdateKind::General:
        return 0.05f;
    case BlockUpdateKind::UpdateNotify:
        return 0;
    case BlockUpdateKind::Water:
        return 0.25f;
    case BlockUpdateKind::Redstone:
        return 0.1f;
    case BlockUpdateKind::RedstoneDust:
        return 0.1f;
    }
    UNREACHABLE();
    return 0;
}
}
}

#endif // BLOCK_UPDATE_H_INCLUDED
