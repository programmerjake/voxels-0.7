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
#ifndef TILE_H_INCLUDED
#define TILE_H_INCLUDED

#include "entity/entity.h"
#include "physics/physics.h"
#include <atomic>

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
class TileEntity final : public EntityDescriptor
{
private:
    struct TileData final
    {
        Block expectedBlock;
        PositionI expectedPosition;
        std::atomic_bool *hasEntity;
        TileData(Block expectedBlock, PositionI expectedPosition, std::atomic_bool *hasEntity)
            : expectedBlock(expectedBlock), expectedPosition(expectedPosition), hasEntity(hasEntity)
        {
        }
    };
    TileData &getTileData(const Entity &entity) const
    {
        TileData *data = static_cast<TileData *>(entity.data.get());
        assert(data != nullptr);
        assert(data->expectedBlock.descriptor == block);
        return *data;
    }
    const BlockDescriptorPointer block;
public:
    typedef void (BlockDescriptor::*MoveHandlerType)(Block b, BlockIterator bi, World &world, WorldLockManager &lock_manager, double deltaTime) const;
private:
    const MoveHandlerType moveHandler;
public:
    explicit TileEntity(BlockDescriptorPointer block, MoveHandlerType moveHandler)
        : EntityDescriptor(L"builtin.tile_entity(block=" + block->name + L")", PhysicsObjectConstructor::empty()), block(block), moveHandler(moveHandler)
    {
    }
    virtual void makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const override
    {
        getTileData(entity);
    }
    virtual Matrix getSelectionBoxTransform(const Entity &entity) const override
    {
        return Matrix::identity();
    }
    virtual void moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const override
    {
        TileData &data = getTileData(entity);
        BlockIterator bi = world.getBlockIterator(data.expectedPosition);
        Block b = bi.get(lock_manager);
        b.lighting = data.expectedBlock.lighting; // ignore block lighting in comparison
        if(b != data.expectedBlock)
        {
            if(data.hasEntity)
                *data.hasEntity = false;
            entity.destroy();
            return;
        }
        (block->*moveHandler)(data.expectedBlock, bi, world, lock_manager, deltaTime);
    }
    virtual void render(Entity &entity, Mesh &dest, RenderLayer rl, Matrix cameraToWorldMatrix) const override
    {
    }
    void addToWorld(World &world, WorldLockManager &lock_manager, PositionI position, Block block, std::atomic_bool *hasEntity = nullptr) const
    {
        world.addEntity(this, (PositionF)position, VectorF(0), lock_manager, std::shared_ptr<void>(new TileData(block, position, hasEntity)));
    }
};
}
}
}
}

#endif // TILE_H_INCLUDED
