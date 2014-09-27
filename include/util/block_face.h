#ifndef BLOCK_FACE_H_INCLUDED
#define BLOCK_FACE_H_INCLUDED

#include "util/enum_traits.h"
#include "util/vector.h"
#include <cassert>

using namespace std;

enum class BlockFace
{
    NX,
    PX,
    NY,
    PY,
    NZ,
    PZ,
    DEFINE_ENUM_LIMITS(NX, PZ)
};

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
    return VectorI(getBlockFaceOutDirectionX(f), getBlockFaceOutDirectionY(f), getBlockFaceOutDirectionZ(f));
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
    return VectorI(getBlockFaceInDirectionX(f), getBlockFaceInDirectionY(f), getBlockFaceInDirectionZ(f));
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
    assert(false);
    return BlockFace::NX;
}

#endif // BLOCK_FACE_H_INCLUDED
