#include "world/world.h"
#include "world/world_generator.h"
#include <random>
#include "block/builtin/air.h"
#include "block/builtin/stone.h"
#include <cmath>

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
    constexpr MyWorldGenerator()
    {
    }
    virtual void generateChunk(PositionI chunkBasePosition, World &world) const override
    {
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
                    if(fpos.y < std::sin(fpos.x / 2) * std::sin(fpos.z / 2))
                        block = Block(Blocks::builtin::Stone::descriptor());
                    blocks[rx][ry][rz] = block;
                }
            }
        }
        world.setBlockRange<false>(chunkBasePosition, blocks);
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
