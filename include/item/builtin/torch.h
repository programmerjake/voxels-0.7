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
#ifndef ITEM_TORCH_H_INCLUDED
#define ITEM_TORCH_H_INCLUDED

#include "item/builtin/image.h"
#include "block/builtin/torch.h"
#include "texture/texture_atlas.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{

class GenericTorch : public ItemImage
{
public:
    const float torchHeight, torchWidth;
    GenericTorch(std::wstring name, TextureDescriptor bottomTexture, TextureDescriptor sideTexture, TextureDescriptor topTexture, const Blocks::builtin::GenericTorch *block, float torchHeight = 10.0f / 16.0f, float torchWidth = 2.0f / 16.0f)
        : ItemImage(name, makeItemMesh(sideTexture), transform(Matrix::translate(0.5f, 0, 0), Blocks::builtin::GenericTorch::makeMesh(bottomTexture, sideTexture, topTexture, torchHeight, torchWidth)), block), torchHeight(torchHeight), torchWidth(torchWidth)
    {
    }
protected:
    const Blocks::builtin::GenericTorch *getBlock() const
    {
        return dynamic_cast<const Blocks::builtin::GenericTorch *>(ItemImage::getBlock());
    }
public:
    virtual Item onUse(Item item, World &world, WorldLockManager &lock_manager, Player &player) const override
    {
        const Blocks::builtin::GenericTorch *block = getBlock();
        if(block == nullptr)
            return item;
        RayCasting::Collision c = player.getPlacedBlockPosition(world, lock_manager);
        if(c.valid())
        {
            if(c.type == RayCasting::Collision::Type::Block && c.blockFace != BlockFaceOrNone::None)
            {
                BlockDescriptorPointer newDescriptor = block->getDescriptor(getOppositeBlockFace(toBlockFace(c.blockFace)));
                if(newDescriptor != nullptr)
                {
                    if(player.placeBlock(c, world, lock_manager, Block(newDescriptor)))
                        return getAfterPlaceItem();
                }
            }
        }
        return item;
    }
};

class Torch final : public GenericTorch
{
    friend class global_instance_maker<Torch>;
private:
    Torch()
        : GenericTorch(L"builtin.torch", TextureAtlas::TorchBottom.td(), TextureAtlas::TorchSide.td(), TextureAtlas::TorchTop.td(), Blocks::builtin::Torch::pointer())
    {
    }
public:
    static const Torch *pointer()
    {
        return global_instance_maker<Torch>::getInstance();
    }
    static ItemDescriptorPointer descriptor()
    {
        return pointer();
    }
};

}
}
}
}

#endif // ITEM_TORCH_H_INCLUDED
