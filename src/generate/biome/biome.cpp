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
#include "generate/biome/biome.h"
#include <algorithm>
#include <cmath>

namespace programmerjake
{
namespace voxels
{
linked_map<std::wstring, BiomeDescriptorPointer> *BiomeDescriptors_t::pBiomeNameMap = nullptr;
std::vector<BiomeDescriptorPointer> *BiomeDescriptors_t::pBiomeVector = nullptr;

BiomeWeights BiomeDescriptors_t::getBiomeWeights(float temperature, float humidity, PositionI pos, RandomSource &randomSource) const
{
    BiomeWeights retval;
    for(BiomeWeights::value_type &v : retval)
    {
        std::get<1>(v) = std::max<float>(0, std::get<0>(v)->getBiomeCorrespondence(temperature, humidity, pos, randomSource));
    }
    retval.normalize();
    for(BiomeWeights::value_type &v : retval)
    {
        std::get<1>(v) = std::pow(std::get<1>(v), 20);
    }
    retval.normalize();
    return std::move(retval);
}
}
}
