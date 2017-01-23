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
    SynchronousUpdateFinalize,
    DEFINE_ENUM_LIMITS(Lighting, SynchronousUpdateFinalize)
};

enum class BlockUpdatePhase
{
    Asynchronous,
    Calculate,
    Update,

    InitialPhase = Calculate,
    DEFINE_ENUM_LIMITS(Asynchronous, Update)
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
    case BlockUpdateKind::SynchronousUpdateFinalize:
        return -1;
    }
    UNREACHABLE();
    return -1;
}

inline BlockUpdatePhase BlockUpdateKindPhase(BlockUpdateKind buKind)
{
    switch(buKind)
    {
    case BlockUpdateKind::Lighting:
        return BlockUpdatePhase::Asynchronous;
    case BlockUpdateKind::General:
        return BlockUpdatePhase::Update;
    case BlockUpdateKind::UpdateNotify:
        return BlockUpdatePhase::Calculate;
    case BlockUpdateKind::Water:
        return BlockUpdatePhase::Update;
    case BlockUpdateKind::Redstone:
        return BlockUpdatePhase::Calculate;
    case BlockUpdateKind::SynchronousUpdateFinalize:
        return BlockUpdatePhase::Update;
    }
    UNREACHABLE();
    return BlockUpdatePhase::Asynchronous;
}

inline BlockUpdatePhase BlockUpdatePhaseNext(BlockUpdatePhase phase)
{
    switch(phase)
    {
    case BlockUpdatePhase::Asynchronous:
        UNREACHABLE();
        return BlockUpdatePhase::InitialPhase;
    case BlockUpdatePhase::Calculate:
        return BlockUpdatePhase::Update;
    case BlockUpdatePhase::Update:
        return BlockUpdatePhase::Calculate;
    }
    UNREACHABLE();
    return BlockUpdatePhase::InitialPhase;
}
}
}

#endif // BLOCK_UPDATE_H_INCLUDED
