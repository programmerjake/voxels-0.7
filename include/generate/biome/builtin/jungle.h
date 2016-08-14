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
#ifndef BIOME_JUNGLE_H_INCLUDED
#define BIOME_JUNGLE_H_INCLUDED

#include "generate/biome/biome_descriptor.h"
#include "util/global_instance_maker.h"
#include "generate/random_world_generator.h"
#include "world/world.h"
#include "block/builtin/air.h"
#include "block/builtin/grass.h"
#include "block/builtin/stone.h"
#include "block/builtin/dirt.h"
#include "block/builtin/bedrock.h"

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
    Jungle() : BiomeDescriptor(L"builtin.jungle", 1, 1)
    {
    }

public:
    virtual float getBiomeCorrespondence(float temperature,
                                         float humidity,
                                         PositionI pos,
                                         RandomSource &randomSource) const override
    {
        return temperature * humidity;
    }
    virtual float getGroundHeight(PositionI columnBasePosition,
                                  RandomSource &randomSource) const override
    {
        PositionF pos = (PositionF)columnBasePosition;
        pos.y = 0;
        return randomSource.getFBMValue(pos * 0.05f) + 3 + World::SeaLevel;
    }
    virtual void makeGroundColumn(PositionI chunkBasePosition,
                                  PositionI columnBasePosition,
                                  BlocksGenerateArray &blocks,
                                  RandomSource &randomSource,
                                  int groundHeight) const override
    {
        for(std::int32_t dy = 0; dy < BlockChunk::chunkSizeY; dy++)
        {
            PositionI position = columnBasePosition + VectorI(0, dy, 0);
            VectorI relativePosition = position - chunkBasePosition;
            Block block;
            if(position.y < groundHeight - 5)
                block = Block(Blocks::builtin::Stone::descriptor());
            else if(position.y < groundHeight)
                block = Block(Blocks::builtin::Dirt::descriptor());
            else if(position.y == groundHeight)
                block = Block(Blocks::builtin::Grass::descriptor());
            else
            {
                block = Block(Blocks::builtin::Air::descriptor());
                block.lighting = Lighting::makeSkyLighting();
            }
            if(randomSource.positionHasBedrock(position))
                block = Block(Blocks::builtin::Bedrock::descriptor());
            blocks[relativePosition.x][relativePosition.y][relativePosition.z] = block;
        }
    }
};
}
}
}
}

#endif // BIOME_JUNGLE_H_INCLUDED
