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
#ifndef DECORATOR_MINERAL_VEIN_H_INCLUDED
#define DECORATOR_MINERAL_VEIN_H_INCLUDED

#include "generate/decorator.h"
#include "block/block.h"
#include "generate/random_world_generator.h"
#include "block/builtin/stone.h"
#include "generate/decorator/pregenerated_instance.h"
#include <random>
#include <algorithm>
#include "util/math_constants.h"
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
namespace Decorators
{
namespace builtin
{
class MineralVeinDecorator : public DecoratorDescriptor
{
protected:
    class MineralVeinDecoratorInstance : public DecoratorInstance
    {
    private:
        const std::uint32_t seed;
        const Block oreBlock;
        const std::size_t maxBlockCount;

    public:
        MineralVeinDecoratorInstance(PositionI position,
                                     DecoratorDescriptorPointer descriptor,
                                     std::uint32_t seed,
                                     Block oreBlock,
                                     std::size_t maxBlockCount)
            : DecoratorInstance(position, descriptor),
              seed(seed),
              oreBlock(oreBlock),
              maxBlockCount(maxBlockCount)
        {
        }
        virtual void generateInChunk(PositionI chunkBasePosition,
                                     WorldLockManager &lock_manager,
                                     World &world,
                                     BlocksGenerateArray &blocks) const override
        {
#ifndef __ANDROID__
            using std::cbrt; // work around android cbrt c++11 issue
#endif
            if(maxBlockCount == 0)
                return;
            VectorI chunkLimitPosition = chunkBasePosition + VectorI(BlockChunk::chunkSizeX - 1,
                                                                     BlockChunk::chunkSizeY - 1,
                                                                     BlockChunk::chunkSizeZ - 1);
            std::minstd_rand rg(seed);
            rg.discard(3);
            std::size_t blockCount = std::uniform_int_distribution<std::size_t>(
                (maxBlockCount + 1) / 2, maxBlockCount)(rg);
            float sizeF = cbrt((float)blockCount * (float)(0.75f / M_PI));
            int size = iceil(sizeF * 2);
            VectorI minP = (position - VectorI(size));
            VectorI maxP = (position + VectorI(size));
            if(minP.x < chunkBasePosition.x)
                minP.x = chunkBasePosition.x;
            if(minP.y < chunkBasePosition.y)
                minP.y = chunkBasePosition.y;
            if(minP.z < chunkBasePosition.z)
                minP.z = chunkBasePosition.z;
            if(maxP.x > chunkLimitPosition.x)
                maxP.x = chunkLimitPosition.x;
            if(maxP.y > chunkLimitPosition.y)
                maxP.y = chunkLimitPosition.y;
            if(maxP.z > chunkLimitPosition.z)
                maxP.z = chunkLimitPosition.z;
            std::size_t count = 0;
            for(VectorI p = minP; p.x <= maxP.x; p.x++)
            {
                for(p.y = minP.y; p.y <= maxP.y; p.y++)
                {
                    for(p.z = minP.z; p.z <= maxP.z; p.z++)
                    {
                        float curSize = sizeF + (std::generate_canonical<float, 1000>(rg) - 0.5f);
                        if(absSquared(p - position) < curSize * curSize)
                        {
                            if(++count >= blockCount)
                                return;
                            VectorI rp = p - chunkBasePosition;
                            Block &b = blocks[rp.x][rp.y][rp.z];
                            if(b.descriptor == Blocks::builtin::Stone::descriptor())
                            {
                                b = oreBlock;
                            }
                        }
                    }
                }
            }
        }
    };
    virtual Block getOreBlock() const = 0;
    const std::size_t maxBlockCount;
    const std::int32_t minGenerateHeight;
    const std::int32_t maxGenerateHeight;
    virtual bool getGeneratePosition(PositionI &result,
                                     PositionI chunkBasePosition,
                                     PositionI columnBasePosition,
                                     PositionI surfacePosition,
                                     RandomSource &randomSource,
                                     std::uint32_t generateNumber) const
    {
        std::minstd_rand rg(
            randomSource.getValueI(columnBasePosition, RandomSource::oreGeneratePositionStart)
            + generateNumber);
        auto minGenerateHeight = this->minGenerateHeight;
        auto maxGenerateHeight = this->maxGenerateHeight;
        maxGenerateHeight =
            std::min<decltype(maxGenerateHeight)>(maxGenerateHeight, surfacePosition.y);
        if(maxGenerateHeight < minGenerateHeight)
            return false;
        result =
            columnBasePosition + VectorI(0,
                                         std::uniform_int_distribution<decltype(maxGenerateHeight)>(
                                             minGenerateHeight, maxGenerateHeight)(rg),
                                         0);
        return true;
    }
    MineralVeinDecorator(std::wstring name,
                         std::size_t maxBlockCount,
                         std::int32_t minGenerateHeight,
                         std::int32_t maxGenerateHeight,
                         float defaultPreChunkGenerateCount,
                         float priority)
        : DecoratorDescriptor(name, 0, priority),
          maxBlockCount(maxBlockCount),
          minGenerateHeight(minGenerateHeight),
          maxGenerateHeight(maxGenerateHeight),
          defaultPreChunkGenerateCount(defaultPreChunkGenerateCount)
    {
    }

public:
    const float defaultPreChunkGenerateCount;
    /** @brief create a DecoratorInstance for this decorator in a chunk
     *
     * @param chunkBasePosition the base position of the chunk to generate in
     * @param columnBasePosition the base position of the column to generate in
     * @param surfacePosition the surface position of the column to generate in
     * @param lock_manager the WorldLockManager
     * @param chunkBaseIterator a BlockIterator to chunkBasePosition
     * @param blocks the blocks for this chunk
     * @param randomSource the RandomSource
     * @param generateNumber a number that is different for each decorator in a chunk (use for
     *picking a different position each time)
     * @return the new DecoratorInstance or nullptr
     *
     */
    virtual std::shared_ptr<const DecoratorInstance> createInstance(
        PositionI chunkBasePosition,
        PositionI columnBasePosition,
        PositionI surfacePosition,
        WorldLockManager &lock_manager,
        BlockIterator chunkBaseIterator,
        const BlocksGenerateArray &blocks,
        RandomSource &randomSource,
        std::uint32_t generateNumber) const override
    {
        PositionI generatePosition;
        if(!getGeneratePosition(generatePosition,
                                chunkBasePosition,
                                columnBasePosition,
                                surfacePosition,
                                randomSource,
                                generateNumber))
            return nullptr;
        assert(chunkBasePosition == BlockChunk::getChunkBasePosition(chunkBasePosition));
        return std::make_shared<MineralVeinDecoratorInstance>(
            generatePosition, this, generateNumber, getOreBlock(), maxBlockCount);
    }
};
}
}
}
}

#endif // DECORATOR_MINERAL_VEIN_H_INCLUDED
