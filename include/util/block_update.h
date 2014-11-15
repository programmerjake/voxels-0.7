#ifndef BLOCK_UPDATE_H_INCLUDED
#define BLOCK_UPDATE_H_INCLUDED

#include "util/position.h"
#include "util/enum_traits.h"

enum class BlockUpdateType
{
    Lighting,
    General,
    DEFINE_ENUM_LIMITS(Lighting, General)
};

struct BlockUpdate
{
    double updateTime;
    constexpr BlockUpdate(double updateTime = -1)
        : updateTime(updateTime)
    {
    }
    constexpr bool good() const
    {
        return updateTime >= 0;
    }
    constexpr bool empty() const
    {
        return !good();
    }
};

typedef enum_array<BlockUpdate, BlockUpdateType> BlockUpdates;

struct BlockUpdateDescriptor
{
    BlockUpdateType type;
    BlockUpdate *update;
};

#endif // BLOCK_UPDATE_H_INCLUDED
