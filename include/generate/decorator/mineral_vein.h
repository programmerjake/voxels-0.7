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
#ifndef DECORATOR_MINERAL_VEIN_H_INCLUDED
#define DECORATOR_MINERAL_VEIN_H_INCLUDED

#include "generate/decorator.h"
#include "block/block.h"
#include "generate/random_world_generator.h"
#include "block/builtin/stone.h"
#include "generate/decorator/pregenerated_instance.h"
#include <random>
#include <algorithm>
#include <queue>

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
    class MineralVeinDecoratorInstance : public PregeneratedDecoratorInstance
    {
    public:
        MineralVeinDecoratorInstance(PositionI position, DecoratorDescriptorPointer descriptor, VectorI baseOffset, VectorI theSize)
            : PregeneratedDecoratorInstance(position, descriptor, baseOffset, theSize)
        {
        }
        virtual bool canReplace(Block oldBlock, Block newBlock) const override
        {
            if(oldBlock.descriptor != Blocks::builtin::Stone::descriptor())
                return false;
            return newBlock.good();
        }
    };
    virtual Block getOreBlock() const = 0;
    const std::size_t minBlockCount;
    const std::size_t maxBlockCount;
    const std::int32_t minGenerateHeight;
    const std::int32_t maxGenerateHeight;
    virtual std::size_t getGeneratePositionAndCount(PositionI &result, PositionI chunkBasePosition, PositionI columnBasePosition, PositionI surfacePosition, RandomSource &randomSource, std::uint32_t generateNumber) const
    {
        std::minstd_rand rg(randomSource.getValueI(columnBasePosition, RandomSource::oreGeneratePositionStart) + generateNumber);
        auto minGenerateHeight = this->minGenerateHeight;
        auto maxGenerateHeight = this->maxGenerateHeight;
        maxGenerateHeight = std::min<decltype(maxGenerateHeight)>(maxGenerateHeight, surfacePosition.y);
        if(maxGenerateHeight < minGenerateHeight)
            return 0;
        result = columnBasePosition + VectorI(0, std::uniform_int_distribution<decltype(maxGenerateHeight)>(minGenerateHeight, maxGenerateHeight)(rg), 0);
        return std::uniform_int_distribution<std::size_t>(minBlockCount, maxBlockCount)(rg);
    }
    MineralVeinDecorator(std::wstring name, std::size_t minBlockCount, std::size_t maxBlockCount, std::int32_t minGenerateHeight, std::int32_t maxGenerateHeight, float defaultPreChunkGenerateCount)
        : DecoratorDescriptor(name, 0), minBlockCount(minBlockCount), maxBlockCount(maxBlockCount), minGenerateHeight(minGenerateHeight), maxGenerateHeight(maxGenerateHeight), defaultPreChunkGenerateCount(defaultPreChunkGenerateCount)
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
     * @param generateNumber a number that is different for each decorator in a chunk (use for picking a different position each time)
     * @return the new DecoratorInstance or nullptr
     *
     */
    virtual std::shared_ptr<const DecoratorInstance> createInstance(PositionI chunkBasePosition, PositionI columnBasePosition, PositionI surfacePosition,
                                 WorldLockManager &lock_manager, BlockIterator chunkBaseIterator,
                                 const checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks,
                                 RandomSource &randomSource, std::uint32_t generateNumber) const override
    {
        PositionI generatePosition;
        std::size_t generateCount = getGeneratePositionAndCount(generatePosition, chunkBasePosition, columnBasePosition, surfacePosition, randomSource, generateNumber);
        if(generateCount == 0)
            return nullptr;
        assert(generateCount < 10000);
        assert(chunkBasePosition == BlockChunk::getChunkBasePosition(chunkBasePosition));
        int generateCubeSize = (int)std::cbrt(generateCount);
        struct GeneratePosition
        {
            VectorI rpos;
            float distSquared;
            bool operator ==(const GeneratePosition &rt) const
            {
                return distSquared == rt.distSquared;
            }
            bool operator !=(const GeneratePosition &rt) const
            {
                return distSquared != rt.distSquared;
            }
            bool operator <(const GeneratePosition &rt) const
            {
                return distSquared < rt.distSquared;
            }
            bool operator <=(const GeneratePosition &rt) const
            {
                return distSquared <= rt.distSquared;
            }
            bool operator >=(const GeneratePosition &rt) const
            {
                return distSquared >= rt.distSquared;
            }
            bool operator >(const GeneratePosition &rt) const
            {
                return distSquared > rt.distSquared;
            }
            GeneratePosition()
            {
            }
            GeneratePosition(VectorI rpos)
                : rpos(rpos), distSquared(absSquared((VectorF)rpos))
            {
            }
        };
        std::vector<GeneratePosition> generatePositionsVector;
        generatePositionsVector.reserve(generateCount);
        std::priority_queue<GeneratePosition> generatePositions(std::less<GeneratePosition>(), std::move(generatePositionsVector));
        for(VectorI rpos = VectorI(-generateCubeSize); rpos.x <= generateCubeSize; rpos.x++)
        {
            for(rpos.y = -generateCubeSize; rpos.y <= generateCubeSize; rpos.y++)
            {
                for(rpos.z = -generateCubeSize; rpos.z <= generateCubeSize; rpos.z++)
                {
                    if(generatePositions.size() >= generateCount)
                        generatePositions.pop();
                    generatePositions.push(GeneratePosition(rpos));
                }
            }
        }
        std::shared_ptr<MineralVeinDecoratorInstance> retval = std::make_shared<MineralVeinDecoratorInstance>(generatePosition, this, VectorI(-generateCubeSize), VectorI(2 * generateCubeSize + 1));
        Block newBlock = getOreBlock();
        for(; !generatePositions.empty(); generatePositions.pop())
        {
            PositionI pos = generatePositions.top().rpos + generatePosition;
            if(chunkBasePosition != BlockChunk::getChunkBasePosition(pos))
                continue;
            retval->setBlock(generatePositions.top().rpos, newBlock);
        }
        return retval;
    }
};
}
}
}
}

#endif // DECORATOR_MINERAL_VEIN_H_INCLUDED
