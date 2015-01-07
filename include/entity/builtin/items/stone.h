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
