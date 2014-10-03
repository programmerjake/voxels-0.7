#ifndef AIR_H_INCLUDED
#define AIR_H_INCLUDED

#include "block/block.h"
#include "util/global_instance_maker.h"

namespace Blocks
{
namespace builtin
{
class Air final : public BlockDescriptor
{
    friend class global_instance_maker<Air>;
public:
    static const Air *descriptor()
    {
        return global_instance_maker<Air>::getInstance();
    }
private:
    Air()
        : BlockDescriptor(L"builtin.air", BlockShape(nullptr), LightProperties(), true, false, false, false, false, false, false, Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), RenderLayer::Opaque)
    {
    }
};
}
}

#endif // AIR_H_INCLUDED
