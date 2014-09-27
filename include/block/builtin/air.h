#ifndef AIR_H_INCLUDED
#define AIR_H_INCLUDED

#include "block/block.h"
#include "util/util.h"

namespace Blocks
{
namespace builtin
{
class Air final : public BlockDescriptor
{
public:
    static const Air *descriptor;
private:
    Air()
        : BlockDescriptor(L"builtin.air", BlockShape(nullptr), true, false, false, false, false, false, false, Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), RenderLayer::Opaque)
    {
    }
    static initializer theInitializer;
    static void init()
    {
        descriptor = new Air();
    }
    static void deinit()
    {
        delete const_cast<Air *>(descriptor);
        descriptor = nullptr;
    }
};
}
}

#endif // AIR_H_INCLUDED
