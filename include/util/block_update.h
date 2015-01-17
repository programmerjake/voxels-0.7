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
