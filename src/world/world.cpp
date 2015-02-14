/*
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
#include <chrono>

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace
{
#if 0
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
        checked_array<checked_array<checked_array<checked_array<float, World::ChunkType::chunkSizeZ>, World::ChunkType::chunkSizeY>, World::ChunkType::chunkSizeX>, totalRandomSize> blocks;
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
                    Block block;
                    if(fpos.y < 3 * localData.getFBMValue(fpos * 0.1f))
                        block = Block(Blocks::builtin::Stone::descriptor());
                    else
                    {
                        block = Block(Blocks::builtin::Air::descriptor());
                        block.lighting = Lighting::makeSkyLighting();
                    }
                    blocks[rx][ry][rz] = block;
                }
            }
        }
        world.setBlockRange<false>(chunkBasePosition, blocks);
        constexpr float newEntityHeight = 3.5f;
        PositionF epos = VectorF(0.5f, newEntityHeight, 0.5f) + chunkBasePosition * VectorF(0, 0, 0);
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
#else
shared_ptr<const WorldGenerator> makeWorldGenerator()
{
    #warning finish
    return nullptr;
#endif
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

namespace
{
World::SeedType makeRandomSeed()
{
    random_device g;
    return uniform_int_distribution<World::SeedType>()(g);
}
}

World::World(SeedType seed, std::shared_ptr<const WorldGenerator> worldGenerator, internal_construct_flag)
    : physicsWorld(std::make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed), destructing(false), lightingStable(false)
{
}

World::World()
    : World(makeRandomSeed())
{

}

BlockUpdate *World::removeAllBlockUpdatesInChunk(BlockUpdateKind kind, BlockIterator bi, WorldLockManager &lock_manager)
{
    BlockChunk *chunk = bi.chunk;
    lock_manager.clear();
    BlockChunkFullLock lockChunk(*chunk);
    auto lockIt = std::unique_lock<decltype(chunk->chunkVariables.blockUpdateListLock)>(chunk->chunkVariables.blockUpdateListLock);
    BlockUpdate *retval = nullptr;
    for(BlockUpdate *node = chunk->chunkVariables.blockUpdateListHead; node != nullptr;)
    {
        if(node->kind == kind)
        {
            if(node->chunk_prev != nullptr)
                node->chunk_prev->chunk_next = node->chunk_next;
            else
                chunk->chunkVariables.blockUpdateListHead = node->chunk_next;
            if(node->chunk_next != nullptr)
                node->chunk_next->chunk_prev = node->chunk_prev;
            else
                chunk->chunkVariables.blockUpdateListTail = node->chunk_prev;
            BlockUpdate *next_node = node->chunk_next;
            node->chunk_prev = nullptr;
            node->chunk_next = retval;
            if(retval != nullptr)
                retval->chunk_prev = node;
            retval = node;
            node = next_node;
            VectorI relativePos = BlockChunk::getChunkRelativePosition(retval->position);
            for(BlockUpdate **pnode = &chunk->blocks[relativePos.x][relativePos.y][relativePos.z].updateListHead; *pnode != nullptr; pnode = &(*pnode)->block_next)
            {
                if(*pnode == retval)
                {
                    *pnode = retval->block_next;
                    break;
                }
            }
            retval->block_next = nullptr;
        }
        else
            node = node->chunk_next;
    }
    return retval;
}

World::World(SeedType seed, std::shared_ptr<const WorldGenerator> worldGenerator)
    : physicsWorld(std::make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed), destructing(false), lightingStable(false)
{
    lightingThread = thread([this]()
    {
        lightingThreadFn();
    });
    chunkGeneratingThread = thread([this]()
    {
        chunkGeneratingThreadFn();
    });
}

World::~World()
{
    destructing = true;
    if(lightingThread.joinable())
        lightingThread.join();
    if(chunkGeneratingThread.joinable())
        chunkGeneratingThread.join();
}

Lighting World::getBlockLighting(BlockIterator bi, WorldLockManager &lock_manager, bool isTopFace)
{
    Block b = bi.get(lock_manager);
    if(!b)
    {
        switch(bi.position().d)
        {
        case Dimension::Overworld:
            if(bi.position().y >= 64 && isTopFace)
                return Lighting::makeSkyLighting();
            return Lighting();
        }
        assert(false);
        return Lighting();
    }
    return b.lighting;
}

void World::lightingThreadFn()
{
    WorldLockManager lock_manager;
    BlockChunkMap *chunks = &physicsWorld->chunks;
    while(!destructing)
    {
        bool didAnything = false;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            if(destructing)
                break;
            BlockChunk *chunk = &(*chunkIter);
            if(chunkIter.is_locked())
                chunkIter.unlock();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            for(BlockUpdate *node = removeAllBlockUpdatesInChunk(BlockUpdateKind::Lighting, cbi, lock_manager); node != nullptr; node = removeAllBlockUpdatesInChunk(BlockUpdateKind::Lighting, cbi, lock_manager))
            {
                didAnything = true;
                while(node != nullptr)
                {
                    BlockIterator bi = cbi;
                    bi.moveTo(node->position);
                    Block b = bi.get(lock_manager);
                    if(b.good())
                    {
                        BlockIterator binx = bi;
                        binx.moveTowardNX();
                        BlockIterator bipx = bi;
                        bipx.moveTowardPX();
                        BlockIterator biny = bi;
                        biny.moveTowardNY();
                        BlockIterator bipy = bi;
                        bipy.moveTowardPY();
                        BlockIterator binz = bi;
                        binz.moveTowardNZ();
                        BlockIterator bipz = bi;
                        bipz.moveTowardPZ();
                        Lighting newLighting = b.descriptor->lightProperties.eval(getBlockLighting(binx, lock_manager, false),
                                                                                  getBlockLighting(bipx, lock_manager, false),
                                                                                  getBlockLighting(biny, lock_manager, false),
                                                                                  getBlockLighting(bipy, lock_manager, true),
                                                                                  getBlockLighting(binz, lock_manager, false),
                                                                                  getBlockLighting(bipz, lock_manager, false));
                        if(newLighting != b.lighting)
                        {
                            b.lighting = newLighting;
                            setBlock(bi, lock_manager, b);
                        }
                    }

                    BlockUpdate *deleteMe = node;
                    node = node->chunk_next;
                    if(node != nullptr)
                        node->chunk_prev = nullptr;
                    delete deleteMe;
                }
                if(destructing)
                    break;
            }
        }
        if(!didAnything)
        {
            lightingStable = true;
            this_thread::yield();
        }
    }
}

void World::chunkGeneratingThreadFn()
{
    WorldLockManager lock_manager;
    while(!destructing)
    {
        bool didAnything = false;
        BlockChunk *bestChunk;
        bool haveChunk = false;
        float chunkPriority = 0;
        for(auto chunkIter = chunks->begin(); chunkIter != chunks->end(); chunkIter++)
        {
            BlockChunk *chunk = &(*chunkIter);
            if(chunk->chunkVariables.generated)
                continue;
            if(chunk->chunkVariables.generateStarted)
                continue;
            if(chunkIter.is_locked())
                chunkIter.unlock();
            BlockIterator cbi(chunk, chunks, chunk->basePosition, VectorI(0));
            float currentChunkPriority = getChunkGeneratePriority(cbi, lock_manager);
            if(!haveChunk || chunkPriority > currentChunkPriority) // low values mean high priority
            {
                haveChunk = true;
                bestChunk = chunk;
                chunkPriority = currentChunkPriority;
            }
        }

        if(haveChunk)
        {
            BlockChunk *chunk = bestChunk;
            if(chunk->chunkVariables.generateStarted.exchange(true)) // someone else got this chunk
                continue;
            didAnything = true;

            #warning add actual generate code

            chunk->chunkVariables.generated = true;
        }

        if(!didAnything)
        {
            this_thread::yield();
        }
    }
}

float World::getChunkGeneratePriority(BlockIterator bi, WorldLockManager &lock_manager) // low values mean high priority
{
    return 0;
}
}
}
