#ifndef STONE_BLOCK_H_INCLUDED
#define STONE_BLOCK_H_INCLUDED

#include "block/builtin/full_block.h"

namespace Blocks
{
namespace builtin
{
class StoneBlock : public FullBlock
{
protected:
    StoneBlock(wstring name, TextureDescriptor td)
        : FullBlock(name, true, td)
    {
    }
};
}
}

#endif // STONE_BLOCK_H_INCLUDED
