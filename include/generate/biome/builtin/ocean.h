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
#ifndef BIOME_OCEAN_H_INCLUDED
#define BIOME_OCEAN_H_INCLUDED

#include "generate/biome/biome_descriptor.h"
#include "util/global_instance_maker.h"
#include "generate/random_world_generator.h"
#include "world/world.h"
#include "block/builtin/air.h"
#include "block/builtin/grass.h"
#include "block/builtin/stone.h"
#include "block/builtin/dirt.h"
#include "block/builtin/water.h"
#include "generate/decorator/tree.h"
#include "block/builtin/bedrock.h"

namespace programmerjake
{
namespace voxels
{
namespace Biomes
{
namespace builtin
{
class Ocean final : public BiomeDescriptor
{
    friend class global_instance_maker<Ocean>;

public:
    static const Ocean *pointer()
    {
        return global_instance_maker<Ocean>::getInstance();
    }
    static BiomeDescriptorPointer descriptor()
    {
        return pointer();
    }

private:
    Ocean() : BiomeDescriptor(L"builtin.ocean", 0.5f, 1)
    {
    }

public:
    virtual float getBiomeCorrespondence(float temperature,
                                         float humidity,
                                         PositionI pos,
                                         RandomSource &randomSource) const override
    {
        pos.y = 0;
        float v =
            limit<float>(randomSource.getFBMValue((PositionF)pos * 0.01f) * 0.4f + 0.5f, 0, 1);
        return v;
    }
    virtual float getGroundHeight(PositionI columnBasePosition,
                                  RandomSource &randomSource) const override
    {
        PositionF pos = (PositionF)columnBasePosition;
        pos.y = 0;
        return randomSource.getFBMValue(pos * 0.05f) - 5 + World::SeaLevel;
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
            else if(position.y < groundHeight
                    || (position.y == groundHeight && position.y < World::SeaLevel))
                block = Block(Blocks::builtin::Dirt::descriptor());
            else if(position.y == groundHeight)
                block = Block(Blocks::builtin::Grass::descriptor());
            else if(position.y <= World::SeaLevel)
            {
                block = Block(Blocks::builtin::Water::descriptor());
                block.lighting = Lighting::makeSkyLighting();
                LightProperties waterLightProperties =
                    Blocks::builtin::Water::descriptor()->lightProperties;
                for(auto i = position.y; i <= World::SeaLevel; i++)
                {
                    block.lighting =
                        waterLightProperties.calculateTransmittedLighting(block.lighting);
                    if(block.lighting == Lighting(0, 0, 0))
                        break;
                }
            }
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
    virtual bool isGoodStartingPosition() const override
    {
        return false;
    }
    virtual float getChunkDecoratorCount(DecoratorDescriptorPointer descriptor) const override
    {
        if(dynamic_cast<const Decorators::builtin::TreeDecorator *>(descriptor) != nullptr)
            return 0;
        return BiomeDescriptor::getChunkDecoratorCount(descriptor);
    }
};
}
}
}
}

#endif // BIOME_OCEAN_H_INCLUDED
