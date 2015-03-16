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
#include "ui/gameui.h"
#include "render/generate.h"
#include "texture/texture_atlas.h"
#include "world/view_point.h"
#include "ui/inventory.h"
#include <vector>

namespace programmerjake
{
namespace voxels
{
namespace ui
{
namespace
{
Mesh makeSelectionMesh()
{
    TextureDescriptor td = TextureAtlas::Selection.td();
    return (Mesh)transform(Matrix::translate(-0.5, -0.5, -0.5).concat(Matrix::scale(1.01)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(td, td, td, td, td, td));
}
Mesh getSelectionMesh()
{
    static thread_local Mesh mesh = makeSelectionMesh();
    return mesh;
}
std::vector<Mesh> makeDestructionMeshes()
{
    std::vector<Mesh> destructionMeshes;
    destructionMeshes.reserve((std::size_t)TextureAtlas::DeleteFrameCount());
    for(int i = 0; i < TextureAtlas::DeleteFrameCount(); i++)
    {
        TextureDescriptor td = TextureAtlas::Delete(i).td();
        destructionMeshes.push_back((Mesh)transform(Matrix::translate(-0.5, -0.5, -0.5).concat(Matrix::scale(1.01)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(td, td, td, td, td, td)));
    }
    return std::move(destructionMeshes);
}
Mesh getDestructionMesh(float progress)
{
    static thread_local std::vector<Mesh> meshes = makeDestructionMeshes();
    int index = ifloor(progress * (int)(meshes.size() - 1));
    index = limit<int>(index, 0, meshes.size() - 1);
    return meshes[index];
}
}
void GameUi::clear(Renderer &renderer)
{
    background = HSVF(0.6, 0.5, 1);
    Ui::clear(renderer);
    Matrix tform = player->getViewTransform();
    viewPoint->setPosition(player->getPosition());
    renderer << RenderLayer::Opaque;
    RayCasting::Collision collision = player->castRay(world, lock_manager, RayCasting::BlockCollisionMaskDefault);
    if(collision.valid())
    {
        Matrix selectionBoxTransform;
        bool drawSelectionBox = false;
        bool drawDestructionBox = false;
        switch(collision.type)
        {
        case RayCasting::Collision::Type::Block:
        {
            BlockIterator bi = world.getBlockIterator(collision.blockPosition);
            Block b = bi.get(lock_manager);
            if(b.good())
            {
                selectionBoxTransform = b.descriptor->getSelectionBoxTransform(b).concat(Matrix::translate(collision.blockPosition));
                drawSelectionBox = true;
                drawDestructionBox = true;
            }
            break;
        }
        case RayCasting::Collision::Type::Entity:
        {
            Entity *entity = collision.entity;
            if(entity != nullptr && entity->good())
            {
                selectionBoxTransform = entity->descriptor->getSelectionBoxTransform(*entity);
                drawSelectionBox = true;
            }
        }
        case RayCasting::Collision::Type::None:
            break;
        }
        if(drawSelectionBox)
        {
            renderer << transform(tform, transform(selectionBoxTransform, getSelectionMesh()));
        }
        if(drawDestructionBox)
        {
            float progress = blockDestructProgress.load(std::memory_order_relaxed);
            if(progress >= 0)
            {
                renderer << transform(tform, transform(selectionBoxTransform, getDestructionMesh(progress)));
            }
        }
    }
    viewPoint->render(renderer, tform, lock_manager);
    renderer << start_overlay << enable_depth_buffer;
}
void GameUi::startInventoryDialog()
{
    setDialog(std::make_shared<PlayerInventory>(player));
}
}
}
}
