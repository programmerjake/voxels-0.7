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
#ifndef BIOME_DESERT_H_INCLUDED
#define BIOME_DESERT_H_INCLUDED

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
class Desert final : public BiomeDescriptor
{
    friend class global_instance_maker<Desert>;
public:
    static const Desert *pointer()
    {
        return global_instance_maker<Desert>::getInstance();
    }
    static BiomeDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Desert()
        : BiomeDescriptor(L"builtin.desert", 1, 0)
    {
    }
public:
    virtual float getBiomeCorrespondence(float temperature, float humidity, PositionI pos, RandomSource &randomSource) const override
    {
        return temperature * (1 - humidity);
    }
};
}
}
}
}

#endif // BIOME_DESERT_H_INCLUDED
