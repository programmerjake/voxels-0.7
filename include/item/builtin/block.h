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
#ifndef ITEM_BLOCK_H_INCLUDED
#define ITEM_BLOCK_H_INCLUDED

#include "item/item.h"
#include "render/generate.h"
#include <cmath>
#include "util/math_constants.h"
#include <algorithm>
#include "block/block.h"
#include "entity/builtin/item.h"
#include "player/player.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
class ItemBlock : public ItemDescriptor
{
    ItemBlock(const ItemBlock &) = delete;
    ItemBlock &operator =(const ItemBlock &) = delete;
    Mesh mesh;
    BlockDescriptorPointer block;
    static Matrix getPreorientSelectionBoxTransform()
    {
        return Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.25f)).concat(Matrix::translate(0, -0.125f, 0));
    }
    static enum_array<Mesh, RenderLayer> makeMeshes(Mesh mesh)
    {
        enum_array<Mesh, RenderLayer> retval;
        retval[RenderLayer::Opaque].append(transform(getPreorientSelectionBoxTransform(), std::move(mesh)));
        return std::move(retval);
    }
protected:
    ItemBlock(std::wstring name, Mesh boxMesh, BlockDescriptorPointer block)
        : ItemDescriptor(name, makeMeshes(boxMesh), getPreorientSelectionBoxTransform()),
        mesh(),
        block(block)
    {
        assert(block != nullptr);
        Matrix tform = Matrix::translate(-0.5f, -0.5f, -0.5f);
        tform = tform.concat(Matrix::rotateY(-M_PI / 4));
        tform = tform.concat(Matrix::rotateX(M_PI / 6));
        tform = tform.concat(Matrix::scale(0.8f / M_SQRT3, 0.8f / M_SQRT3, 0.1f / M_SQRT3));
        tform = tform.concat(Matrix::translate(0.5f, 0.5f, 0.1f));
        mesh = transform(tform, std::move(boxMesh));
    }
    ItemBlock(std::wstring name, TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz, BlockDescriptorPointer block)
        : ItemBlock(name, Generate::unitBox(nx, px, ny, py, nz, pz), block)
    {
    }
    ItemBlock(std::wstring name, TextureDescriptor td, BlockDescriptorPointer block)
        : ItemBlock(name, td, td, td, td, td, td, block)
    {
    }
    virtual Item getAfterPlaceItem() const
    {
        return Item();
    }
    BlockDescriptorPointer getBlock() const
    {
        return block;
    }
public:
    static void renderMesh(Mesh &dest, float minX, float maxX, float minY, float maxY, const Mesh &mesh)
    {
        Matrix tform = Matrix::scale(std::min<float>(maxX - minX, maxY - minY));
        tform = tform.concat(Matrix::translate(minX, minY, -1));
        tform = tform.concat(Matrix::scale(maxRenderZ));
        dest.append(transform(tform, mesh));
    }
    virtual void render(Item item, Mesh &dest, float minX, float maxX, float minY, float maxY) const override
    {
        renderMesh(dest, minX, maxX, minY, maxY, mesh);
    }
    virtual bool dataEqual(std::shared_ptr<void> data1, std::shared_ptr<void> data2) const override
    {
        return true;
    }
    virtual Item onUse(Item item, World &world, WorldLockManager &lock_manager, Player &player) const override
    {
        RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
        if(c.valid())
        {
            if(player.placeBlock(c, world, lock_manager, Block(block)))
                return getAfterPlaceItem();
        }
        return item;
    }
    virtual Item onDispenseOrDrop(Item item, World &world, WorldLockManager &lock_manager, PositionI dispensePosition, VectorF dispenseDirection, bool useSpecialAction) const override
    {
        ItemDescriptor::addToWorld(world, lock_manager, ItemStack(item), dispensePosition + VectorF(0.5), dispenseDirection * Entities::builtin::EntityItem::dropSpeed);
        return Item();
    }
};
}
}
}
}

#endif // ITEM_BLOCK_H_INCLUDED
