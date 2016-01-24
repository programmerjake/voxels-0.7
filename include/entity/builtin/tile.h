/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
    TileEntity(const TileEntity &) = delete;
    TileEntity &operator =(const TileEntity &) = delete;
private:
    struct TileData final
    {
        TileData(const TileData &) = delete;
        TileData &operator =(const TileData &) = delete;
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
    TileData &getTileData(std::shared_ptr<void> dataIn) const
    {
        TileData *data = static_cast<TileData *>(dataIn.get());
        assert(data != nullptr);
        assert(data->expectedBlock.descriptor == block);
        return *data;
    }
    const BlockDescriptorPointer block;
public:
    typedef void (BlockDescriptor::*MoveHandlerType)(Block b, BlockIterator bi, World &world, WorldLockManager &lock_manager, double deltaTime) const;
    typedef std::atomic_bool *(BlockDescriptor::*AttachHandlerType)(Block b, BlockIterator bi, World &world, WorldLockManager &lock_manager) const;
private:
    const MoveHandlerType moveHandler;
    const AttachHandlerType attachHandler;
public:
    explicit TileEntity(BlockDescriptorPointer block, MoveHandlerType moveHandler, AttachHandlerType attachHandler)
        : EntityDescriptor(L"builtin.tile_entity(block=" + block->name + L")"),
        block(block),
        moveHandler(moveHandler),
        attachHandler(attachHandler)
    {
    }
    virtual std::shared_ptr<PhysicsObject> makePhysicsObject(Entity &entity, PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> physicsWorld) const override
    {
        return PhysicsObject::makeEmpty(position, velocity, physicsWorld);
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
        BlockIterator bi = world.getBlockIterator(data.expectedPosition, lock_manager.tls);
        Block b = bi.get(lock_manager);
        b.lighting = data.expectedBlock.lighting; // ignore block lighting in comparison
        auto oldData = b.data;
        if(data.expectedBlock.data == nullptr)
        {
            b.data = nullptr;
        }
        if(b != data.expectedBlock || oldData == nullptr)
        {
            if(data.hasEntity)
                *data.hasEntity = false;
            entity.destroy();
            return;
        }
        else if(data.expectedBlock.data == nullptr)
        {
            data.expectedBlock.data = oldData;
            data.hasEntity = (block->*attachHandler)(data.expectedBlock, bi, world, lock_manager);
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
    virtual void write(PositionF positionf, VectorF velocity, std::shared_ptr<void> dataIn, stream::Writer &writer) const override
    {
        TileData &data = getTileData(dataIn);
        Block b = data.expectedBlock;
        b.data = nullptr;
        PositionI p = data.expectedPosition;
        stream::write<Block>(writer, b);
        stream::write<PositionI>(writer, p);
    }
    virtual std::shared_ptr<void> read(PositionF positionf, VectorF velocity, stream::Reader &reader) const override
    {
        Block b = stream::read<Block>(reader);
        b.data = nullptr;
        PositionI p = stream::read<PositionI>(reader);
        return std::shared_ptr<void>(new TileData(b, p, nullptr));
    }
};
}
}
}
}

#endif // TILE_H_INCLUDED
