/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef PARTICLE_H_INCLUDED
#define PARTICLE_H_INCLUDED

#include "entity/entity.h"
#include "physics/physics.h"
#include <vector>
#include <cmath>
#include "render/generate.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
class Particle : public EntityDescriptor
{
public:
    const bool collideWithBlocks;
    const VectorF gravity;
    const float extent;

protected:
    struct ParticleData
    {
        friend class Particle;

    private:
        double time = 0.0f;

    public:
        double getTime() const
        {
            return time;
        }
        const float existDuration;
        ParticleData(float existDuration) : existDuration(existDuration)
        {
        }
        virtual ~ParticleData() = default;
        void write(stream::Writer &writer) const
        {
            stream::write<float32_t>(writer, existDuration);
            stream::write<float64_t>(writer, time);
        }
        static ParticleData read(stream::Reader &reader)
        {
            float existDuration = stream::read_limited<float32_t>(reader, 0, 1e6);
            double time = stream::read_limited<float64_t>(reader, 0, existDuration + 1);
            ParticleData retval(existDuration);
            retval.time = time;
            return retval;
        }
    };
    std::vector<TextureDescriptor> frames;
    float framesPerSecond;
    bool loop;
    virtual float getExistDuration(World &world) const
    {
        return frames.size() / framesPerSecond;
    }
    virtual ColorF colorizeColor(const ParticleData &data) const
    {
        return colorizeIdentity();
    }
    ParticleData &getParticleData(std::shared_ptr<void> data) const
    {
        assert(data != nullptr);
        return *static_cast<ParticleData *>(data.get());
    }

public:
    Particle(std::wstring name,
             std::vector<TextureDescriptor> frames,
             float framesPerSecond,
             bool loop = false,
             bool collideWithBlocks = true,
             VectorF gravity = VectorF(0),
             float extent = 1.0f / 12.0f)
        : EntityDescriptor(name),
          collideWithBlocks(collideWithBlocks),
          gravity(gravity),
          extent(extent),
          frames(std::move(frames)),
          framesPerSecond(framesPerSecond),
          loop(loop)
    {
    }
    virtual std::shared_ptr<PhysicsObject> makePhysicsObject(
        Entity &entity,
        PositionF position,
        VectorF velocity,
        std::shared_ptr<PhysicsWorld> physicsWorld) const override
    {
        if(!collideWithBlocks)
        {
            return PhysicsObject::makeEmpty(position, velocity, physicsWorld, gravity);
        }
        return PhysicsObject::makeCylinder(
            position,
            velocity,
            gravity != VectorF(0),
            false,
            extent,
            extent,
            PhysicsProperties(PhysicsProperties::blockCollisionMask, 0, 0, 1, gravity),
            physicsWorld);
    }
    virtual void makeData(Entity &entity,
                          World &world,
                          WorldLockManager &lock_manager) const override
    {
        if(!entity.data)
            entity.data = std::shared_ptr<void>(new ParticleData(getExistDuration(world)));
    }
    virtual Transform getSelectionBoxTransform(const Entity &entity) const override
    {
        return Transform::translate(-0.5f, -0.5f, -0.5f)
            .concat(Transform::scale(extent))
            .concat(Transform::translate(entity.physicsObject->getPosition()));
    }
    virtual void moveStep(Entity &entity,
                          World &world,
                          WorldLockManager &lock_manager,
                          double deltaTime) const override
    {
        ParticleData &data = getParticleData(entity.data);
        data.time += deltaTime;
        if(data.time >= data.existDuration)
        {
            entity.destroy();
        }
    }
    virtual void render(Entity &entity,
                        Mesh &dest,
                        RenderLayer rl,
                        const Transform &cameraToWorldMatrix) const override
    {
        if(rl != RenderLayer::Opaque)
            return;
        double time = getParticleData(entity.data).getTime();
        std::size_t frameIndex = (std::size_t)(int)std::floor(time * framesPerSecond);
        if(loop)
            frameIndex %= frames.size();
        else if(frameIndex >= frames.size())
            return;
        TextureDescriptor frame = frames[frameIndex];

        VectorF upVector = cameraToWorldMatrix.normalMatrix.applyNoTranslate(VectorF(0, extent, 0));
        VectorF rightVector =
            cameraToWorldMatrix.normalMatrix.applyNoTranslate(VectorF(extent, 0, 0));
        VectorF center = entity.physicsObject->getPosition();

        VectorF nxny = center - upVector - rightVector;
        VectorF nxpy = center + upVector - rightVector;
        VectorF pxny = center - upVector + rightVector;
        VectorF pxpy = center + upVector + rightVector;
        ColorF c = colorizeColor(static_cast<float>(time));
        dest.append(Generate::quadrilateral(frame, nxny, c, pxny, c, pxpy, c, nxpy, c));
    }
};
}
}
}
}

#endif // PARTICLE_H_INCLUDED
