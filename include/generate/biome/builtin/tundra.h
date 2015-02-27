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
#ifndef TUNDRA_H_INCLUDED
#define TUNDRA_H_INCLUDED

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
class Tundra final : public BiomeDescriptor
{
    friend class global_instance_maker<Tundra>;
public:
    static const Tundra *pointer()
    {
        return global_instance_maker<Tundra>::getInstance();
    }
    static BiomeDescriptorPointer descriptor()
    {
        return pointer();
    }
private:
    Tundra()
        : BiomeDescriptor(L"builtin.tundra", 0, 0)
    {
    }
public:
    virtual float getBiomeCorrespondence(float temperature, float humidity, PositionI pos, RandomSource &randomSource) const override
    {
        return (1 - temperature) * (1 - humidity);
    }
};
}
}
}
}

#endif // TUNDRA_H_INCLUDED
