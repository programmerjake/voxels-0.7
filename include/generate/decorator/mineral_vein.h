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
#include <random>
#include <algorithm>
#include <queue>

namespace programmerjake
{
namespace voxels
{
namespace Decorators
{
class MineralVeinDecorator : public Decorator
{
protected:
    virtual Block getOreBlock() const = 0;
    const std::size_t minBlockCount;
    const std::size_t maxBlockCount;
    const std::int32_t minGenerateHeight;
    const std::int32_t maxGenerateHeight;
    virtual std::size_t getGeneratePositionAndCount(PositionI &result, PositionI chunkBasePosition, PositionI columnBasePosition, PositionI surfacePosition, RandomSource &randomSource, std::size_t generateNumber) const
    {
        std::minstd_rand rg(randomSource.getValueI(columnBasePosition, RandomSource::oreGeneratePositionStart));
        rg.discard(generateNumber);
        auto minGenerateHeight = this->minGenerateHeight;
        auto maxGenerateHeight = this->maxGenerateHeight;
        maxGenerateHeight = std::min<decltype(maxGenerateHeight)>(maxGenerateHeight, surfacePosition.y);
        if(maxGenerateHeight < minGenerateHeight)
            return 0;
        result = columnBasePosition + VectorI(0, std::uniform_int_distribution<decltype(maxGenerateHeight)>(minGenerateHeight, maxGenerateHeight)(rg), 0);
        return std::uniform_int_distribution<std::size_t>(minBlockCount, maxBlockCount)(rg);
    }
    MineralVeinDecorator(std::wstring name, std::size_t minBlockCount, std::size_t maxBlockCount, std::int32_t minGenerateHeight, std::int32_t maxGenerateHeight)
        : Decorator(name, 0), minBlockCount(minBlockCount), maxBlockCount(maxBlockCount), minGenerateHeight(minGenerateHeight), maxGenerateHeight(maxGenerateHeight)
    {
    }
public:
    /** @brief generate this decorator in a chunk
     *
     * @param simulate if the generation should be simulated (viz., don't change any blocks or add entities, just return true if generation would succeed)
     * @param chunkBasePosition the base position of the chunk to generate in
     * @param columnBasePosition the base position of the column to generate in
     * @param surfacePosition the surface position of the column to generate in
     * @param lock_manager the WorldLockManager
     * @param world the World
     * @param blocks the blocks for this chunk
     * @param randomSource the RandomSource
     * @param generateNumber the number of times that a decorator was generated or tried to generate (use for picking a different position each time)
     * @return true if this decorator was generated or would be generated (if called again with simulate = false)
     *
     */
    virtual bool generateInChunk(bool simulate, PositionI chunkBasePosition, PositionI columnBasePosition, PositionI surfacePosition,
                                 WorldLockManager &lock_manager, World &world,
                                 checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks,
                                 RandomSource &randomSource, std::size_t generateNumber) const override
    {
        PositionI generatePosition;
        std::size_t generateCount = getGeneratePositionAndCount(generatePosition, chunkBasePosition, columnBasePosition, surfacePosition, randomSource, generateNumber);
        if(generateCount == 0)
            return false;
        assert(generateCount < 10000);
        assert(chunkBasePosition == BlockChunk::getChunkBasePosition(chunkBasePosition));
        if(simulate)
            return true;
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
        Block newBlock = getOreBlock();
        for(; !generatePositions.empty(); generatePositions.pop())
        {
            PositionI pos = generatePositions.top().rpos + generatePosition;
            if(chunkBasePosition != BlockChunk::getChunkBasePosition(pos))
                continue;
            VectorI rpos = BlockChunk::getChunkRelativePosition(pos);
            Block &b = blocks[rpos.x][rpos.y][rpos.z];
            if(b.descriptor == Blocks::builtin::Stone::descriptor())
                b = newBlock;
        }
        return true;
    }
};
}
}
}

#endif // DECORATOR_MINERAL_VEIN_H_INCLUDED
