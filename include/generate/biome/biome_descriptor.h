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
#ifndef BIOME_DESCRIPTOR_H_INCLUDED
#define BIOME_DESCRIPTOR_H_INCLUDED

#include "generate/biome/biome.h"
#include "util/block_chunk.h"
#include "generate/decorator_declaration.h"

namespace programmerjake
{
namespace voxels
{
class BiomeDescriptor
{
    friend class BiomeDescriptors_t;
private:
    BiomeIndex index;
public:
    BiomeIndex getIndex() const
    {
        return index;
    }
    const std::wstring name;
    const float temperature;
    const float humidity;
    const ColorF grassColor;
    const ColorF leavesColor;
    const ColorF waterColor;
    static ColorF makeBiomeGrassColor(float temperature, float humidity)
    {
        temperature = limit<float>(temperature, 0, 1);
        humidity = limit<float>(humidity, 0, 1) * temperature;
        return interpolate(temperature, RGBF(0.5f, 0.7f, 0.6f), interpolate(humidity, RGBF(0.75f, 0.7f, 0.33f), RGBF(0.25f, 1.0f, 0.2f)));
    }
    static ColorF makeBiomeLeavesColor(float temperature, float humidity)
    {
        temperature = limit<float>(temperature, 0, 1);
        humidity = limit<float>(humidity, 0, 1) * temperature;
        return interpolate(temperature, RGBF(0.4f, 0.63f, 0.5f), interpolate(humidity, RGBF(0.7f, 0.65f, 0.16f), RGBF(0.1f, 0.8f, 0.0f)));
    }
    static ColorF makeBiomeWaterColor(float temperature, float humidity)
    {
        temperature = limit<float>(temperature, 0, 1);
        humidity = limit<float>(humidity, 0, 1) * temperature;
        return interpolate(temperature, RGBF(0.0f, 0.0f, 1.0f), interpolate(humidity, RGBF(0.0f, 0.63f, 0.83f), RGBF(0.0f, 0.9f, 0.75f)));
    }
    typedef checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> BlocksArray;
    virtual float getBiomeCorrespondence(float temperature, float humidity, PositionI pos, RandomSource &randomSource) const = 0;
    virtual float getGroundHeight(PositionI columnBasePosition, RandomSource &randomSource) const = 0;
    virtual void makeGroundColumn(PositionI chunkBasePosition, PositionI columnBasePosition, BlocksArray &blocks, RandomSource &randomSource, int groundHeight) const = 0;
    virtual bool isGoodStartingPosition() const
    {
        return true;
    }
    virtual float getChunkDecoratorCount(DecoratorPointer decorator) const
    {
        return 0;
    }
protected:
    BiomeDescriptor(std::wstring name, float temperature, float humidity, ColorF grassColor, ColorF leavesColor, ColorF waterColor);
    BiomeDescriptor(std::wstring name, float temperature, float humidity)
        : BiomeDescriptor(name, temperature, humidity, makeBiomeGrassColor(temperature, humidity), makeBiomeLeavesColor(temperature, humidity), makeBiomeWaterColor(temperature, humidity))
    {
    }
};

inline BiomeDescriptor::BiomeDescriptor(std::wstring name, float temperature, float humidity, ColorF grassColor, ColorF leavesColor, ColorF waterColor)
    : name(name), temperature(temperature), humidity(humidity), grassColor(grassColor), leavesColor(leavesColor), waterColor(waterColor)
{
    BiomeDescriptors.addBiome(this);
}
}
}

#endif // BIOME_DESCRIPTOR_H_INCLUDED
