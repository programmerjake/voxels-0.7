/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
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
#ifndef IMAGE_ITEM_H_INCLUDED
#define IMAGE_ITEM_H_INCLUDED

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
class ImageItem : public EntityItem
{
private:
    static Matrix getPreorientSelectionBoxTransform()
    {
        return Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.5f)).concat(Matrix::translate(0, 0, 0));
    }
    static enum_array<Mesh, RenderLayer> makeMeshes(TextureDescriptor td)
    {
        enum_array<Mesh, RenderLayer> retval;
        retval[RenderLayer::Opaque].append(transform(Matrix::translate(0, 0, 0.5f).concat(getPreorientSelectionBoxTransform()), Generate::item3DImage(td)));
        return std::move(retval);
    }
    static enum_array<Mesh, RenderLayer> makeMeshes(Mesh mesh, RenderLayer rl)
    {
        enum_array<Mesh, RenderLayer> retval;
        retval[rl].append(transform(Matrix::translate(0, 0, 0.5f).concat(getPreorientSelectionBoxTransform()), std::move(mesh)));
        return std::move(retval);
    }
protected:
    ImageItem(std::wstring name, Mesh mesh, RenderLayer rl = RenderLayer::Opaque)
        : EntityItem(name, makeMeshes(std::move(mesh), rl), getPreorientSelectionBoxTransform())
    {
    }
    ImageItem(std::wstring name, TextureDescriptor td)
        : EntityItem(name, makeMeshes(td), getPreorientSelectionBoxTransform())
    {
    }
    virtual void onGiveToPlayer(Player &player) const override = 0;
};
}
}
}
}

#endif // IMAGE_ITEM_H_INCLUDED
