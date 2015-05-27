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
#ifndef PARTICLES_REDSTONE_H_INCLUDED
#define PARTICLES_REDSTONE_H_INCLUDED

#include "entity/builtin/particle.h"
#include "texture/texture_atlas.h"
#include "util/global_instance_maker.h"
#include "world/world.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
namespace particles
{

class Redstone final : public Particle
{
    friend class global_instance_maker<Redstone>;
private:
    static std::vector<TextureDescriptor> makeFrames()
    {
        std::vector<TextureDescriptor> retval;
        retval.reserve(TextureAtlas::ParticleSmokeFrameCount());
        for(int i = TextureAtlas::ParticleSmokeFrameCount() - 1; i >= 0; i--)
        {
            retval.push_back(TextureAtlas::ParticleSmoke(i).td());
        }
        return std::move(retval);
    }
    Redstone()
        : Particle(L"builtin.particles.redstone", makeFrames(), 4.0f, false, true, VectorF(0, 0.2f, 0))
    {
    }
protected:
    virtual ColorF colorizeColor(const ParticleData &dataIn) const override
    {
        return RGBF(0.75f, 0, 0);
    }
    virtual float getExistDuration(World &world) const override
    {
        return 0.2f / std::uniform_real_distribution<float>(0.1f, 1.0f)(world.getRandomGenerator());
    }
public:
    static const Redstone *pointer()
    {
        return global_instance_maker<Redstone>::getInstance();
    }
    static EntityDescriptorPointer descriptor()
    {
        return pointer();
    }
    static Entity *addToWorld(World &world, WorldLockManager &lock_manager, PositionF position)
    {
        VectorF v = VectorF::random(world.getRandomGenerator()) * 0.25f;
        return world.addEntity(descriptor(), position, v, lock_manager);
    }
    virtual void write(PositionF position, VectorF velocity, std::shared_ptr<void> dataIn, stream::Writer &writer) const override
    {
        ParticleData data = getParticleData(dataIn);
        stream::write<ParticleData>(writer, data);
    }
    virtual std::shared_ptr<void> read(PositionF position, VectorF velocity, stream::Reader &reader) const override
    {
        ParticleData data = stream::read<ParticleData>(reader);
        return std::shared_ptr<void>(new ParticleData(data));
    }
};

}
}
}
}
}

#endif // PARTICLES_REDSTONE_H_INCLUDED
