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
#include "ui/player_dialog.h"
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
void renderSunOrMoon(Renderer &renderer, Matrix orientationTransform, VectorF position, TextureDescriptor td, float extent = 0.1f)
{
    VectorF upVector = extent * normalize(cross(position, VectorF(0, 0, 1)));
    VectorF rightVector = extent * normalize(cross(position, upVector));

    VectorF nxny = position - upVector - rightVector;
    VectorF nxpy = position + upVector - rightVector;
    VectorF pxny = position - upVector + rightVector;
    VectorF pxpy = position + upVector + rightVector;
    ColorF c = colorizeIdentity();
    renderer << transform(inverse(orientationTransform), Generate::quadrilateral(td,
                                                                 nxny, c,
                                                                 pxny, c,
                                                                 pxpy, c,
                                                                 nxpy, c));
}
}
void GameUi::clear(Renderer &renderer)
{
    std::shared_ptr<Player> player = playerW.lock();
    PositionF playerPosition = player->getPosition();
    background = world->getSkyColor(playerPosition);
    Ui::clear(renderer);
    Matrix tform = player->getViewTransform();
    viewPoint->setPosition(playerPosition);
    renderer << RenderLayer::Opaque;
    if(hasSun(playerPosition.d))
        renderSunOrMoon(renderer, player->getWorldOrientationTransform(), world->getSunPosition(), TextureAtlas::Sun.td());
    if(hasMoon(playerPosition.d))
    {
        TextureDescriptor moonTD;
        switch(world->getVisibleMoonPhase())
        {
        case 0:
            moonTD = TextureAtlas::Moon0.td();
            break;
        case 1:
            moonTD = TextureAtlas::Moon1.td();
            break;
        case 2:
            moonTD = TextureAtlas::Moon2.td();
            break;
        case 3:
            moonTD = TextureAtlas::Moon3.td();
            break;
        case 4:
            moonTD = TextureAtlas::Moon4.td();
            break;
        case 5:
            moonTD = TextureAtlas::Moon5.td();
            break;
        case 6:
            moonTD = TextureAtlas::Moon6.td();
            break;
        default:
            moonTD = TextureAtlas::Moon7.td();
            break;
        }
        renderSunOrMoon(renderer, player->getWorldOrientationTransform(), world->getMoonPosition(), moonTD);
    }
    renderer << start_overlay;
    RayCasting::Collision collision = player->castRay(*world, lock_manager, RayCasting::BlockCollisionMaskDefault);
    if(collision.valid())
    {
        Matrix selectionBoxTransform;
        bool drawSelectionBox = false;
        bool drawDestructionBox = false;
        switch(collision.type)
        {
        case RayCasting::Collision::Type::Block:
        {
            BlockIterator bi = world->getBlockIterator(collision.blockPosition);
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
    renderer << start_overlay << reset_render_layer;
}
void GameUi::startInventoryDialog()
{
    setDialog(std::make_shared<PlayerInventory>(playerW.lock()));
}
void GameUi::setDialogWorldAndLockManager()
{
    std::shared_ptr<PlayerDialog> playerDialog = std::dynamic_pointer_cast<PlayerDialog>(dialog);
    if(playerDialog != nullptr)
        playerDialog->setWorldAndLockManager(*world, lock_manager);
}
}
}
}
