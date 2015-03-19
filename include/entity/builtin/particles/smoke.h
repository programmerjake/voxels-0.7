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
#ifndef PARTICLES_SMOKE_H_INCLUDED
#define PARTICLES_SMOKE_H_INCLUDED

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

class Smoke final : public Particle
{
    friend class global_instance_maker<Smoke>;
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
    Smoke()
        : Particle(L"builtin.particles.smoke", makeFrames(), 4.0f, 0, false, true, VectorF(0, 0.5f, 0))
    {
        existDuration = frames.size() / framesPerSecond;
    }
protected:
    virtual ColorF colorizeColor(double time) const
    {
        return GrayscaleF(0.6f - 0.6f / (1.0f + time));
    }
public:
    static const Smoke *pointer()
    {
        return global_instance_maker<Smoke>::getInstance();
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
};

}
}
}
}
}

#endif // PARTICLES_SMOKE_H_INCLUDED
