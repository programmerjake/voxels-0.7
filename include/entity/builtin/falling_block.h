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
#ifndef FALLING_BLOCK_ENTITY_H_INCLUDED
#define FALLING_BLOCK_ENTITY_H_INCLUDED

#include "entity/entity.h"
#include "block/block.h"
#include "item/item.h"
#include <atomic>
#include "render/generate.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
class FallingBlock : public EntityDescriptor
{
protected:
    Mesh blockMesh;
    FallingBlock(std::wstring name, Mesh blockMesh)
        : EntityDescriptor(name), blockMesh(blockMesh)
    {
    }
    FallingBlock(std::wstring name, TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz)
        : EntityDescriptor(name), blockMesh(Generate::unitBox(nx, px, ny, py, nz, pz))
    {
    }
    FallingBlock(std::wstring name, TextureDescriptor td)
        : EntityDescriptor(name), blockMesh(Generate::unitBox(td, td, td, td, td, td))
    {
    }
    struct FallingBlockData final
    {
        double timeLeft = 4.0f;
        std::atomic_bool collided;
        FallingBlockData()
            : collided(false)
        {
        }
    };
    double &getTimeLeft(std::shared_ptr<void> data) const
    {
        assert(data != nullptr);
        return static_cast<FallingBlockData *>(data.get())->timeLeft;
    }
    std::atomic_bool &getCollided(std::shared_ptr<void> data) const
    {
        assert(data != nullptr);
        return static_cast<FallingBlockData *>(data.get())->collided;
    }
    struct MyCollisionHandler : public PhysicsCollisionHandler
    {
        std::shared_ptr<std::atomic_bool> collidedPtr;
        virtual void onCollide(std::shared_ptr<PhysicsObject> collidingObject, std::shared_ptr<PhysicsObject> otherObject) override
        {
        }
        virtual void onCollideWithBlock(std::shared_ptr<PhysicsObject> collidingObject, BlockIterator otherObject, WorldLockManager &lock_manager) override
        {
            collidedPtr->store(true, std::memory_order_relaxed);
        }
        MyCollisionHandler(std::shared_ptr<std::atomic_bool> collidedPtr)
            : collidedPtr(collidedPtr)
        {
        }
        MyCollisionHandler(std::shared_ptr<FallingBlockData> data)
            : collidedPtr(data, &data->collided)
        {
        }
    };
    virtual Block getPlacedBlock() const = 0;
    virtual Item getDroppedItem() const = 0;
    virtual void dropItem(PositionF position, World &world, WorldLockManager &lock_manager) const
    {
        Item item = getDroppedItem();
        if(item.good())
            ItemDescriptor::addToWorld(world, lock_manager, ItemStack(item), position);
    }
    virtual void placeBlock(PositionF position, World &world, WorldLockManager &lock_manager) const
    {
        BlockIterator bi = world.getBlockIterator((PositionI)position);
        Block b = bi.get(lock_manager);
        Block newBlock = getPlacedBlock();
        if(!b.good() || !newBlock.good() || !b.descriptor->isReplaceableByFallingBlock())
        {
            dropItem(position, world, lock_manager);
            return;
        }
        b.descriptor->onReplace(world, b, bi, lock_manager);
        world.setBlock(bi, lock_manager, newBlock);
    }
public:
    virtual void moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const override
    {
        if(getCollided(entity.data).load(std::memory_order_relaxed))
        {
            placeBlock(entity.physicsObject->getPosition(), world, lock_manager);
            entity.destroy();
            return;
        }
        double &timeLeft = getTimeLeft(entity.data);
        timeLeft -= deltaTime;
        if(timeLeft <= 0)
        {
            dropItem(entity.physicsObject->getPosition(), world, lock_manager);
            entity.destroy();
            return;
        }
    }
    virtual void render(Entity &entity, Mesh &dest, RenderLayer rl, Matrix cameraToWorldMatrix) const override
    {
        if(rl != RenderLayer::Opaque)
            return;
        dest.append(transform(Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.98f)).concat(Matrix::translate(entity.physicsObject->getPosition())), blockMesh));
    }
    virtual Matrix getSelectionBoxTransform(const Entity &entity) const override
    {
        return Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.98f)).concat(Matrix::translate(entity.physicsObject->getPosition()));
    }
protected:
    virtual void makeData(Entity &entity) const
    {
        if(entity.data == nullptr)
            entity.data = std::shared_ptr<void>(new FallingBlockData);
    }
public:
    virtual void makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const override
    {
        makeData(entity);
    }
    virtual std::shared_ptr<PhysicsObject> makePhysicsObject(Entity &entity, World &world, PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> physicsWorld) const override
    {
        makeData(entity);
        std::shared_ptr<PhysicsCollisionHandler> collisionHandler = std::make_shared<MyCollisionHandler>(std::static_pointer_cast<FallingBlockData>(entity.data));
        return PhysicsObject::makeBox(position, velocity, true, false, VectorF(0.49f), PhysicsProperties(PhysicsProperties::blockCollisionMask, 0, 0, 1), physicsWorld, collisionHandler);
    }
};
}
}
}
}

#endif // FALLING_BLOCK_ENTITY_H_INCLUDED
