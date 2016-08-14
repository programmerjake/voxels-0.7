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
#ifndef BLOCK_FACE_H_INCLUDED
#define BLOCK_FACE_H_INCLUDED

#include "util/enum_traits.h"
#include "util/vector.h"
#include "util/util.h"
#include <cassert>
#include <cmath>

namespace programmerjake
{
namespace voxels
{
enum class BlockFace : std::uint8_t
{
    NX,
    PX,
    NY,
    PY,
    NZ,
    PZ,
    DEFINE_ENUM_LIMITS(NX, PZ)
};

enum class BlockFaceOrNone : std::uint8_t
{
    None,
    NX,
    PX,
    NY,
    PY,
    NZ,
    PZ,
    DEFINE_ENUM_LIMITS(None, PZ)
};

inline BlockFaceOrNone toBlockFaceOrNone(BlockFace f)
{
    switch(f)
    {
    case BlockFace::NX:
        return BlockFaceOrNone::NX;
    case BlockFace::PX:
        return BlockFaceOrNone::PX;
    case BlockFace::NY:
        return BlockFaceOrNone::NY;
    case BlockFace::PY:
        return BlockFaceOrNone::PY;
    case BlockFace::NZ:
        return BlockFaceOrNone::NZ;
    case BlockFace::PZ:
        return BlockFaceOrNone::PZ;
    }
    UNREACHABLE();
    return BlockFaceOrNone::None;
}

inline BlockFace toBlockFace(BlockFaceOrNone f)
{
    switch(f)
    {
    case BlockFaceOrNone::None:
        UNREACHABLE();
        return BlockFace::NX;
    case BlockFaceOrNone::NX:
        return BlockFace::NX;
    case BlockFaceOrNone::PX:
        return BlockFace::PX;
    case BlockFaceOrNone::NY:
        return BlockFace::NY;
    case BlockFaceOrNone::PY:
        return BlockFace::PY;
    case BlockFaceOrNone::NZ:
        return BlockFace::NZ;
    case BlockFaceOrNone::PZ:
        return BlockFace::PZ;
    }
    UNREACHABLE();
    return BlockFace::NX;
}

constexpr int getBlockFaceOutDirectionX(BlockFace f)
{
    return (f == BlockFace::NX) ? -1 : ((f == BlockFace::PX) ? 1 : 0);
}

constexpr int getBlockFaceOutDirectionY(BlockFace f)
{
    return (f == BlockFace::NY) ? -1 : ((f == BlockFace::PY) ? 1 : 0);
}

constexpr int getBlockFaceOutDirectionZ(BlockFace f)
{
    return (f == BlockFace::NZ) ? -1 : ((f == BlockFace::PZ) ? 1 : 0);
}

constexpr VectorI getBlockFaceOutDirection(BlockFace f)
{
    return VectorI(
        getBlockFaceOutDirectionX(f), getBlockFaceOutDirectionY(f), getBlockFaceOutDirectionZ(f));
}

constexpr int getBlockFaceInDirectionX(BlockFace f)
{
    return (f == BlockFace::NX) ? 1 : ((f == BlockFace::PX) ? -1 : 0);
}

constexpr int getBlockFaceInDirectionY(BlockFace f)
{
    return (f == BlockFace::NY) ? 1 : ((f == BlockFace::PY) ? -1 : 0);
}

constexpr int getBlockFaceInDirectionZ(BlockFace f)
{
    return (f == BlockFace::NZ) ? 1 : ((f == BlockFace::PZ) ? -1 : 0);
}

constexpr VectorI getBlockFaceInDirection(BlockFace f)
{
    return VectorI(
        getBlockFaceInDirectionX(f), getBlockFaceInDirectionY(f), getBlockFaceInDirectionZ(f));
}

inline BlockFace getOppositeBlockFace(BlockFace f)
{
    switch(f)
    {
    case BlockFace::NX:
        return BlockFace::PX;
    case BlockFace::NY:
        return BlockFace::PY;
    case BlockFace::NZ:
        return BlockFace::PZ;
    case BlockFace::PX:
        return BlockFace::NX;
    case BlockFace::PY:
        return BlockFace::NY;
    case BlockFace::PZ:
        return BlockFace::NZ;
    }
    UNREACHABLE();
    return BlockFace::NX;
}

inline BlockFace getBlockFaceFromOutVector(VectorF v)
{
    VectorF absV = VectorF(std::fabs(v.x), std::fabs(v.y), std::fabs(v.z));
    if(absV.x > absV.y && absV.x > absV.z)
    {
        if(v.x < 0)
            return BlockFace::NX;
        return BlockFace::PX;
    }
    if(absV.y > absV.z)
    {
        if(v.y < 0)
            return BlockFace::NY;
        return BlockFace::PY;
    }
    if(v.z < 0)
        return BlockFace::NZ;
    return BlockFace::PZ;
}

inline BlockFace getBlockFaceFromInVector(VectorF v)
{
    return getBlockFaceFromOutVector(-v);
}
}
}

#endif // BLOCK_FACE_H_INCLUDED
