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
#ifndef RANDOM_WORLD_GENERATOR_H_INCLUDED
#define RANDOM_WORLD_GENERATOR_H_INCLUDED

#include "world/world_generator.h"
#include "util/checked_array.h"
#include <memory>
#include <unordered_map>
#include <random>
#include <cmath>
#include <limits>
#include <atomic>
#include <exception>
#include "util/blocks_generate_array.h"

namespace programmerjake
{
namespace voxels
{
class RandomSource final
{
public:
    typedef std::size_t RandomClass;
    static constexpr RandomClass fbmRandomStart = 0;
    static constexpr RandomClass fbmRandomSize = 5;
    static constexpr RandomClass biomeTemperatureStart = fbmRandomStart + fbmRandomSize;
    static constexpr RandomClass biomeTemperatureSize = 5;
    static constexpr RandomClass biomeHumidityStart = biomeTemperatureStart + biomeTemperatureSize;
    static constexpr RandomClass biomeHumiditySize = 5;
    static constexpr RandomClass oreGeneratePositionStart = biomeHumidityStart + biomeHumiditySize;
    static constexpr RandomClass oreGeneratePositionSize = 1;
    static constexpr RandomClass bedrockGeneratePositionStart = oreGeneratePositionStart + oreGeneratePositionSize;
    static constexpr RandomClass bedrockGeneratePositionSize = 1;
    static RandomClass allocateRandomClasses(std::size_t count)
    {
        static std::atomic_size_t nextRandomClass(bedrockGeneratePositionStart + bedrockGeneratePositionSize);
        return nextRandomClass.fetch_add(count, std::memory_order_relaxed);
    }
private:
    struct RandomChunkKey
    {
        PositionI basePosition;
        RandomClass randomClass;
        RandomChunkKey(PositionI basePosition, RandomClass randomClass)
            : basePosition(basePosition), randomClass(randomClass)
        {
        }
        RandomChunkKey() = default;
        bool operator ==(const RandomChunkKey &rt) const
        {
            return basePosition == rt.basePosition && randomClass == rt.randomClass;
        }
        bool operator !=(const RandomChunkKey &rt) const
        {
            return basePosition != rt.basePosition || randomClass != rt.randomClass;
        }
    };
    struct RandomChunkKeyHasher
    {
        std::size_t operator ()(const RandomChunkKey &v) const
        {
            return (std::uint32_t)std::hash<PositionI>()(v.basePosition) + (std::uint32_t)65537 * (std::uint32_t)v.randomClass;
        }
    };
    struct RandomCacheChunk
    {
        checked_array<checked_array<checked_array<std::uint32_t, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> values;
        const RandomChunkKey key;
        typedef std::minstd_rand random_generator;
        static random_generator makeRandomGenerator(World::SeedType seed, RandomChunkKey key)
        {
            seed += (std::uint32_t)RandomChunkKeyHasher()(key);
            return random_generator(seed);
        }
        RandomCacheChunk(World::SeedType seed, RandomChunkKey key)
            : key(key)
        {
            random_generator rg = makeRandomGenerator(seed, key);
            std::uniform_int_distribution<std::int32_t> distribution;
            for(auto &i : values)
            {
                for(auto &j : i)
                {
                    for(auto &v : j)
                    {
                        v = distribution(rg);
                    }
                }
            }
        }
        std::int32_t getValueI(std::int32_t rx, std::int32_t ry, std::int32_t rz) const
        {
            return values[rx][ry][rz];
        }
        std::int32_t getValueI(VectorI relativePosition) const
        {
            return getValueI(relativePosition.x, relativePosition.y, relativePosition.z);
        }
        float getValueCanonicalF(std::int32_t rx, std::int32_t ry, std::int32_t rz) const
        {
            return (float)getValueI(rx, ry, rz) * (-1.0f / (float)std::numeric_limits<std::int32_t>::min());
        }
        float getValueBalancedF(std::int32_t rx, std::int32_t ry, std::int32_t rz) const
        {
            std::uint32_t v = getValueI(rx, ry, rz);
            v <<= 1; // shift max bit into sign bit
            return (float)(std::int32_t)v * (-1.0f / (float)std::numeric_limits<std::int32_t>::min());
        }
        float getValueCanonicalF(VectorI relativePosition) const
        {
            return getValueCanonicalF(relativePosition.x, relativePosition.y, relativePosition.z);
        }
        float getValueBalancedF(VectorI relativePosition) const
        {
            return getValueBalancedF(relativePosition.x, relativePosition.y, relativePosition.z);
        }
    };
    const World::SeedType seed;
    std::unordered_map<RandomChunkKey, RandomCacheChunk, RandomChunkKeyHasher> randomCache;
public:
    RandomSource(World::SeedType seed)
        : seed(seed)
    {
    }
    const RandomCacheChunk &getChunk(PositionI chunkBasePosition, RandomClass randomClass)
    {
        RandomChunkKey key(chunkBasePosition, randomClass);
        auto iter = randomCache.find(key);
        if(iter == randomCache.end())
        {
            iter = std::get<0>(randomCache.insert(std::make_pair(key, RandomCacheChunk(seed, key))));
        }
        return std::get<1>(*iter);
    }
    std::int32_t getValueI(PositionI position, RandomClass randomClass)
    {
        return getChunk(BlockChunk::getChunkBasePosition(position), randomClass).getValueI(BlockChunk::getChunkRelativePosition((VectorI)position));
    }
    float getValueBalancedF(PositionI position, RandomClass randomClass)
    {
        return getChunk(BlockChunk::getChunkBasePosition(position), randomClass).getValueBalancedF(BlockChunk::getChunkRelativePosition((VectorI)position));
    }
    float getValueCanonicalF(PositionI position, RandomClass randomClass)
    {
        return getChunk(BlockChunk::getChunkBasePosition(position), randomClass).getValueCanonicalF(BlockChunk::getChunkRelativePosition((VectorI)position));
    }
    float getValueBalancedF(PositionF position, RandomClass randomClass)
    {
        PositionI floorPos = (PositionI)position;
        VectorF t = position - floorPos;
        VectorF nt = VectorF(1) - t;
        float vnxnynz = getValueBalancedF(floorPos + VectorI(0, 0, 0), randomClass);
        float vpxnynz = getValueBalancedF(floorPos + VectorI(1, 0, 0), randomClass);
        float vnxpynz = getValueBalancedF(floorPos + VectorI(0, 1, 0), randomClass);
        float vpxpynz = getValueBalancedF(floorPos + VectorI(1, 1, 0), randomClass);
        float vnxnypz = getValueBalancedF(floorPos + VectorI(0, 0, 1), randomClass);
        float vpxnypz = getValueBalancedF(floorPos + VectorI(1, 0, 1), randomClass);
        float vnxpypz = getValueBalancedF(floorPos + VectorI(0, 1, 1), randomClass);
        float vpxpypz = getValueBalancedF(floorPos + VectorI(1, 1, 1), randomClass);
        float vnxny = vnxnynz * nt.z + vnxnypz * t.z;
        float vpxny = vpxnynz * nt.z + vpxnypz * t.z;
        float vnxpy = vnxpynz * nt.z + vnxpypz * t.z;
        float vpxpy = vpxpynz * nt.z + vpxpypz * t.z;
        float vnx = vnxny * nt.y + vnxpy * t.y;
        float vpx = vpxny * nt.y + vpxpy * t.y;
        return vnx * nt.x + vpx * t.x;
    }
    float getValueCanonicalF(PositionF position, RandomClass randomClass)
    {
        PositionI floorPos = (PositionI)position;
        VectorF t = position - floorPos;
        VectorF nt = VectorF(1) - t;
        float vnxnynz = getValueCanonicalF(floorPos + VectorI(0, 0, 0), randomClass);
        float vpxnynz = getValueCanonicalF(floorPos + VectorI(1, 0, 0), randomClass);
        float vnxpynz = getValueCanonicalF(floorPos + VectorI(0, 1, 0), randomClass);
        float vpxpynz = getValueCanonicalF(floorPos + VectorI(1, 1, 0), randomClass);
        float vnxnypz = getValueCanonicalF(floorPos + VectorI(0, 0, 1), randomClass);
        float vpxnypz = getValueCanonicalF(floorPos + VectorI(1, 0, 1), randomClass);
        float vnxpypz = getValueCanonicalF(floorPos + VectorI(0, 1, 1), randomClass);
        float vpxpypz = getValueCanonicalF(floorPos + VectorI(1, 1, 1), randomClass);
        float vnxny = vnxnynz * nt.z + vnxnypz * t.z;
        float vpxny = vpxnynz * nt.z + vpxnypz * t.z;
        float vnxpy = vnxpynz * nt.z + vnxpypz * t.z;
        float vpxpy = vpxpynz * nt.z + vpxpypz * t.z;
        float vnx = vnxny * nt.y + vnxpy * t.y;
        float vpx = vpxny * nt.y + vpxpy * t.y;
        return vnx * nt.x + vpx * t.x;
    }
    float getFBMValue(PositionF position, RandomClass randomClassStart = fbmRandomStart, std::size_t randomClassCount = fbmRandomSize, float amplitudeFactor = 0.4f)
    {
        float retval = 0;
        float factor = 1;
        float scale = 1;
        for(RandomClass randomClass = randomClassStart; randomClass < randomClassStart + randomClassCount; randomClass++)
        {
            retval += factor * getValueBalancedF(position * scale, randomClass);
            factor *= amplitudeFactor;
            scale *= 2;
        }
        return retval;
    }
    BiomeProperties getBiomeProperties(PositionI pos)
    {
        pos.y = 0;
        PositionF fpos = pos;
        fpos *= 0.01f;
        float temperature = limit<float>(getFBMValue(fpos, RandomSource::biomeTemperatureStart, RandomSource::biomeTemperatureSize) * 0.4f + 0.5f, 0, 1);
        float humidity = limit<float>(getFBMValue(fpos, RandomSource::biomeHumidityStart, RandomSource::biomeHumiditySize) * 0.4f + 0.5f, 0, 1);
        return BiomeProperties(BiomeDescriptors.getBiomeWeights(temperature, humidity, pos, *this));
    }
    bool positionHasBedrock(PositionI pos)
    {
        const int bedrockMaxHeight = 3;
        if(pos.y >= bedrockMaxHeight)
            return false;
        if(pos.y <= 0)
            return true;
        float probability = 1.0f - (float)pos.y / bedrockMaxHeight;
        if(getValueCanonicalF(pos, RandomSource::bedrockGeneratePositionStart) < probability)
            return true;
        return false;
    }
};

class RandomWorldGenerator : public WorldGenerator
{
protected:
    virtual void generate(PositionI chunkBasePosition, BlocksGenerateArray &blocks, World &world, WorldLockManager &lock_manager, RandomSource &randomSource, const std::atomic_bool *abortFlag) const = 0;
    struct GenerationAbortedException final : public std::exception
    {
        virtual const char *what() const noexcept override final
        {
            return "generation aborted";
        }
    };
    static void checkForAbort(const std::atomic_bool *abortFlag)
    {
        if(abortFlag != nullptr && abortFlag->load(std::memory_order_relaxed))
            throw GenerationAbortedException();
    }
public:
    virtual void generateChunk(PositionI chunkBasePosition, World &world, WorldLockManager &lock_manager, const std::atomic_bool *abortFlag) const override final
    {
        try
        {
            std::unique_ptr<BlocksGenerateArray> blocks(new BlocksGenerateArray);
            RandomSource randomSource(world.getWorldGeneratorSeed());
            generate(chunkBasePosition, *blocks, world, lock_manager, randomSource, abortFlag);
            world.setBlockRange(chunkBasePosition, chunkBasePosition + VectorI(BlockChunk::chunkSizeX - 1, BlockChunk::chunkSizeY - 1, BlockChunk::chunkSizeZ - 1), lock_manager, *blocks, VectorI(0));
        }
        catch(GenerationAbortedException &)
        {
            return;
        }
    }
};
}
}

#endif // RANDOM_WORLD_GENERATOR_H_INCLUDED
