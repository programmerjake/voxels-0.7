#ifndef STONE_H_INCLUDED
#define STONE_H_INCLUDED

#include "block/builtin/stone_block.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"

namespace Blocks
{
namespace builtin
{
class Stone final : public StoneBlock
{
    friend class global_instance_maker<Stone>;
public:
    static const Stone *descriptor()
    {
        return global_instance_maker<Stone>::getInstance();
    }
private:
    Stone()
        : StoneBlock(L"builtin.stone", TextureAtlas::Stone.td())
    {
    }
};
}
}

#endif // STONE_H_INCLUDED
