#ifndef BLOCK_ITEM_H_INCLUDED
#define BLOCK_ITEM_H_INCLUDED

#include "entity/builtin/item.h"
#include "render/generate.h"

namespace Entities
{
namespace builtin
{
class BlockItem : public Item
{
private:
    static enum_array<Mesh, RenderLayer> makeMeshes(TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz, RenderLayer rl)
    {
        enum_array<Mesh, RenderLayer> retval;
        retval[rl].append(transform(Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.25f)).concat(Matrix::translate(0, -0.125f, 0)), Generate::unitBox(nx, px, ny, py, nz, pz)));
        return std::move(retval);
    }
protected:
    BlockItem(wstring name, TextureDescriptor td, RenderLayer rl = RenderLayer::Opaque)
        : Item(name, makeMeshes(td, td, td, td, td, td, rl))
    {
    }
    BlockItem(wstring name, TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz, RenderLayer rl = RenderLayer::Opaque)
        : Item(name, makeMeshes(nx, px, ny, py, nz, pz, rl))
    {
    }
};
}
}

#endif // BLOCK_ITEM_H_INCLUDED
