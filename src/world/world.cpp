#include "world/world.h"
#include "world/world_generator.h"
#include <random>
#include "block/builtin/air.h"
#include "block/builtin/stone.h"
#include "entity/builtin/items/stone.h"
#include <cmath>
#include <random>
#include <limits>
#include <thread>
#include <unordered_map>
#include <mutex>

using namespace std;

bool World::generateChunk()
{
    lock_guard<mutex> lockIt(generateChunkMutex);
    if(needGenerateChunksList.empty())
        return false;
    PositionI cpos = needGenerateChunksList.front();
    needGenerateChunksList.pop_front();
    needGenerateChunksSet.erase(cpos);
    generatedChunksSet.insert(cpos);
    shared_ptr<World> pworld = make_shared<World>(worldGeneratorSeed, nullptr);
    {
        unlock_guard<mutex> unlockIt(generateChunkMutex);
        assert(worldGenerator != nullptr);
        World &world = *pworld;
        worldGenerator->generateChunk(cpos, world);
        world.generatedChunksSet.insert(cpos);
    }
    generatedChunksList.push_back(pworld);
    return true;
}

namespace
{
class MyWorldGenerator final : public WorldGenerator
{
public:
    MyWorldGenerator()
    {
    }
private:
    typedef size_t RandomClass;
    static constexpr RandomClass fbmRandomStart = 0;
    static constexpr RandomClass fbmRandomSize = 5;
    static constexpr RandomClass totalRandomSize = fbmRandomSize + fbmRandomStart;
    struct RandomCacheChunk
    {
        array<array<array<array<float, World::ChunkType::chunkSizeZ>, World::ChunkType::chunkSizeY>, World::ChunkType::chunkSizeX>, totalRandomSize> blocks;
        PositionI basePosition;
        typedef minstd_rand random_generator;
        static random_generator makeRandomGenerator(World::SeedType seed, PositionI basePosition)
        {
            seed += std::hash<PositionI>()(basePosition);
            return random_generator(seed);
        }
        RandomCacheChunk()
        {
        }
        RandomCacheChunk(World::SeedType seed, PositionI basePosition)
            : basePosition(basePosition)
        {
            random_generator rg = makeRandomGenerator(seed, basePosition);
            for(auto &i : blocks)
            {
                for(auto &j : i)
                {
                    for(auto &k : j)
                    {
                        for(auto &v : k)
                        {
                            v = generate_canonical<float, std::numeric_limits<float>::digits>(rg);
                        }
                    }
                }
            }
        }
        float getValue(int32_t rx, int32_t ry, int32_t rz, RandomClass randomClass) const
        {
            assert(rx >= 0 && rx < World::ChunkType::chunkSizeX);
            assert(ry >= 0 && ry < World::ChunkType::chunkSizeY);
            assert(rz >= 0 && rz < World::ChunkType::chunkSizeZ);
            assert(randomClass >= 0 && randomClass < totalRandomSize);
            return blocks[randomClass][rx][ry][rz];
        }
        float getValue(VectorI relativePosition, RandomClass randomClass) const
        {
            return getValue(relativePosition.x, relativePosition.y, relativePosition.z, randomClass);
        }
    };
    struct LocalData
    {
        const World::SeedType seed;
        unordered_map<PositionI, RandomCacheChunk> randomCache;
        LocalData(World::SeedType seed)
            : seed(seed)
        {
        }
        const RandomCacheChunk &getChunk(PositionI chunkBasePosition)
        {
            auto iter = randomCache.find(chunkBasePosition);
            if(iter == randomCache.end())
            {
                iter = std::get<0>(randomCache.insert(make_pair(chunkBasePosition, RandomCacheChunk(seed, chunkBasePosition))));
            }
            return std::get<1>(*iter);
        }
        float getValue(PositionI position, RandomClass randomClass)
        {
            return getChunk(World::ChunkType::getChunkBasePosition(position)).getValue(World::ChunkType::getChunkRelativePosition((VectorI)position), randomClass);
        }
        float getValue(PositionF position, RandomClass randomClass)
        {
            PositionI floorPos = (PositionI)position;
            VectorF t = position - floorPos;
            VectorF nt = VectorF(1) - t;
            float vnxnynz = getValue(floorPos + VectorI(0, 0, 0), randomClass);
            float vpxnynz = getValue(floorPos + VectorI(1, 0, 0), randomClass);
            float vnxpynz = getValue(floorPos + VectorI(0, 1, 0), randomClass);
            float vpxpynz = getValue(floorPos + VectorI(1, 1, 0), randomClass);
            float vnxnypz = getValue(floorPos + VectorI(0, 0, 1), randomClass);
            float vpxnypz = getValue(floorPos + VectorI(1, 0, 1), randomClass);
            float vnxpypz = getValue(floorPos + VectorI(0, 1, 1), randomClass);
            float vpxpypz = getValue(floorPos + VectorI(1, 1, 1), randomClass);
            float vnxny = vnxnynz * nt.z + vnxnypz * t.z;
            float vpxny = vpxnynz * nt.z + vpxnypz * t.z;
            float vnxpy = vnxpynz * nt.z + vnxpypz * t.z;
            float vpxpy = vpxpynz * nt.z + vpxpypz * t.z;
            float vnx = vnxny * nt.y + vnxpy * t.y;
            float vpx = vpxny * nt.y + vpxpy * t.y;
            return vnx * nt.x + vpx * t.x;
        }
        float getBalancedValue(PositionI position, RandomClass randomClass)
        {
            return getValue(position, randomClass) * 2 - 1;
        }
        float getBalancedValue(PositionF position, RandomClass randomClass)
        {
            PositionI floorPos = (PositionI)position;
            VectorF t = position - floorPos;
            VectorF nt = VectorF(1) - t;
            float vnxnynz = getBalancedValue(floorPos + VectorI(0, 0, 0), randomClass);
            float vpxnynz = getBalancedValue(floorPos + VectorI(1, 0, 0), randomClass);
            float vnxpynz = getBalancedValue(floorPos + VectorI(0, 1, 0), randomClass);
            float vpxpynz = getBalancedValue(floorPos + VectorI(1, 1, 0), randomClass);
            float vnxnypz = getBalancedValue(floorPos + VectorI(0, 0, 1), randomClass);
            float vpxnypz = getBalancedValue(floorPos + VectorI(1, 0, 1), randomClass);
            float vnxpypz = getBalancedValue(floorPos + VectorI(0, 1, 1), randomClass);
            float vpxpypz = getBalancedValue(floorPos + VectorI(1, 1, 1), randomClass);
            float vnxny = vnxnynz * nt.z + vnxnypz * t.z;
            float vpxny = vpxnynz * nt.z + vpxnypz * t.z;
            float vnxpy = vnxpynz * nt.z + vnxpypz * t.z;
            float vpxpy = vpxpynz * nt.z + vpxpypz * t.z;
            float vnx = vnxny * nt.y + vnxpy * t.y;
            float vpx = vpxny * nt.y + vpxpy * t.y;
            return vnx * nt.x + vpx * t.x;
        }
        float getFBMValue(PositionF position, float amplitudeFactor = 0.4f)
        {
            float retval = 0;
            float factor = 1;
            float scale = 1;
            for(RandomClass randomClass = fbmRandomStart; randomClass < fbmRandomStart + fbmRandomSize; randomClass++)
            {
                retval += factor * getBalancedValue(position * scale, randomClass);
                factor *= amplitudeFactor;
            }
            return retval;
        }
    };
    mutable mutex localDataLock;
    mutable unordered_map<thread::id, unordered_map<World::SeedType, LocalData>> localDataMapMap;
    unordered_map<World::SeedType, LocalData> &getThreadLocalData() const
    {
        lock_guard<mutex> lockIt(localDataLock);
        return localDataMapMap[this_thread::get_id()];
    }
    LocalData &getLocalData(const World &world) const
    {
        unordered_map<World::SeedType, LocalData> &localDataMap = getThreadLocalData();
        auto key = world.getWorldGeneratorSeed();
        auto iter = localDataMap.find(key);
        if(iter == localDataMap.end())
            iter = std::get<0>(localDataMap.insert(make_pair(key, LocalData(world.getWorldGeneratorSeed()))));
        return std::get<1>(*iter);
    }
public:
    virtual void generateChunk(PositionI chunkBasePosition, World &world) const override
    {
        LocalData &localData = getLocalData(world);
        decltype(World::ChunkType::blocks) blocks;
        for(int32_t rx = 0; rx < World::ChunkType::chunkSizeX; rx++)
        {
            for(int32_t ry = 0; ry < World::ChunkType::chunkSizeY; ry++)
            {
                for(int32_t rz = 0; rz < World::ChunkType::chunkSizeZ; rz++)
                {
                    PositionI pos = chunkBasePosition + VectorI(rx, ry, rz);
                    PositionF fpos = pos + VectorF(0.5f);
                    Block block = Block(Blocks::builtin::Air::descriptor());
                    if(fpos.y < 3 * localData.getFBMValue(fpos * 0.1f))
                        block = Block(Blocks::builtin::Stone::descriptor());
                    blocks[rx][ry][rz] = block;
                }
            }
        }
        world.setBlockRange<false>(chunkBasePosition, blocks);
        constexpr float newEntityHeight = 3.5f;
        PositionF epos = VectorF(0.5f, newEntityHeight, 0.5f) + chunkBasePosition * VectorF(1, 0, 0);
        if(World::ChunkType::getChunkBasePosition((PositionI)epos) == chunkBasePosition)
        {
            world.addEntity(Entities::builtin::items::Stone::descriptor(), epos);
            world.addEntity(Entities::builtin::items::Stone::descriptor(), epos + VectorF(0, 1, 0));
        }
    }
};

shared_ptr<const WorldGenerator> makeWorldGenerator()
{
    return make_shared<MyWorldGenerator>();
}
}

World::World(SeedType seed)
    : World(seed, makeWorldGenerator())
{

}

namespace
{
World::SeedType makeSeed(wstring seed)
{
    World::SeedType retval = 0;
    for(wchar_t ch : seed)
    {
        retval *= 8191;
        retval += ch;
    }
    return retval;
}
}

World::World(wstring seed)
    : World(makeSeed(seed))
{

}

namespace
{
World::SeedType makeRandomSeed()
{
    random_device g;
    return uniform_int_distribution<World::SeedType>()(g);
}
}

World::World()
    : World(makeRandomSeed())
{

}
