#ifndef STONE_BLOCK_H_INCLUDED
#define STONE_BLOCK_H_INCLUDED

#include "block/builtin/full_block.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class StoneBlock : public FullBlock
{
protected:
    StoneBlock(std::wstring name, TextureDescriptor td)
        : FullBlock(name, LightProperties(), true, td)
    {
    }
};
}
}
}
}

#endif // STONE_BLOCK_H_INCLUDED
