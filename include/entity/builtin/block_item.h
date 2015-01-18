/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef BLOCK_ITEM_H_INCLUDED
#define BLOCK_ITEM_H_INCLUDED

#include "entity/builtin/item.h"
#include "render/generate.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
class BlockItem : public Item
{
private:
    static Matrix getPreorientSelectionBoxTransform()
    {
        return Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.25f)).concat(Matrix::translate(0, -0.125f, 0));
    }
    static enum_array<Mesh, RenderLayer> makeMeshes(TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz, RenderLayer rl)
    {
        enum_array<Mesh, RenderLayer> retval;
        retval[rl].append(transform(getPreorientSelectionBoxTransform(), Generate::unitBox(nx, px, ny, py, nz, pz)));
        return std::move(retval);
    }
protected:
    BlockItem(std::wstring name, TextureDescriptor td, RenderLayer rl = RenderLayer::Opaque)
        : Item(name, makeMeshes(td, td, td, td, td, td, rl), getPreorientSelectionBoxTransform())
    {
    }
    BlockItem(std::wstring name, TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz, RenderLayer rl = RenderLayer::Opaque)
        : Item(name, makeMeshes(nx, px, ny, py, nz, pz, rl), getPreorientSelectionBoxTransform())
    {
    }
};
}
}
}
}

#endif // BLOCK_ITEM_H_INCLUDED
