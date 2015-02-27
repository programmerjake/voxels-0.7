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
#ifndef BIOME_JUNGLE_H_INCLUDED
#define BIOME_JUNGLE_H_INCLUDED

#include "generate/biome/biome.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Biomes
{
namespace builtin
{
class Jungle final : public BiomeDescriptor
{
    friend class global_instance_maker<Jungle>;
public:
    static const Jungle *pointer()
    {
        return global_instance_maker<Jungle>::getInstance();
    }
    static BiomeDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Jungle()
        : BiomeDescriptor(L"builtin.jungle", 1, 1)
    {
    }
public:
    virtual float getBiomeCorrespondence(float temperature, float humidity, PositionI pos, RandomSource &randomSource) const override
    {
        return temperature * humidity;
    }
};
}
}
}
}

#endif // BIOME_JUNGLE_H_INCLUDED
