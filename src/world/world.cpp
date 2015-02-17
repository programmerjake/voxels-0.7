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
#include "world/view_point.h"
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
#include "util/logging.h"

using namespace std;

namespace programmerjake
{
namespace voxels
{
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
        checked_array<checked_array<checked_array<checked_array<float, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX>, totalRandomSize> blocks;
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
            assert(rx >= 0 && rx < BlockChunk::chunkSizeX);
            assert(ry >= 0 && ry < BlockChunk::chunkSizeY);
            assert(rz >= 0 && rz < BlockChunk::chunkSizeZ);
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
            return getChunk(BlockChunk::getChunkBasePosition(position)).getValue(BlockChunk::getChunkRelativePosition((VectorI)position), randomClass);
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
    virtual void generateChunk(PositionI chunkBasePosition, World &world, WorldLockManager &lock_manager) const override
    {
#if 0
        LocalData &localData = getLocalData(world);
#else
        LocalData localData(world.getWorldGeneratorSeed());
#endif
        typedef checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> BlockArrayType;
        std::unique_ptr<BlockArrayType> blocks(new BlockArrayType);
        for(auto i = BlockChunkRelativePositionIterator::begin(); i != BlockChunkRelativePositionIterator::end(); i++)
        {
            PositionI pos = chunkBasePosition + *i;
            PositionF fpos = pos + VectorF(0.5f);
            Block block;
            if(fpos.y - World::AverageGroundHeight < 3 * localData.getFBMValue(fpos * 0.1f))
                block = Block(Blocks::builtin::Stone::descriptor());
            else
            {
                block = Block(Blocks::builtin::Air::descriptor());
                block.lighting = Lighting::makeSkyLighting();
            }
            blocks->at(i->x)[i->y][i->z] = block;
        }
        world.setBlockRange(chunkBasePosition, chunkBasePosition + VectorI(BlockChunk::chunkSizeX - 1, BlockChunk::chunkSizeY - 1, BlockChunk::chunkSizeZ - 1), lock_manager, *blocks, VectorI(0));
#if 0
        constexpr float newEntityHeight = 3.5f;
        PositionF epos = VectorF(0.5f, newEntityHeight, 0.5f) + chunkBasePosition * VectorF(0, 0, 0);
        if(BlockChunk::getChunkBasePosition((PositionI)epos) == chunkBasePosition)
        {
            world.addEntity(Entities::builtin::items::Stone::descriptor(), epos);
            world.addEntity(Entities::builtin::items::Stone::descriptor(), epos + VectorF(0, 1, 0));
        }
#endif
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
    const int initSize = 5;
    for(int dx = -initSize; dx <= initSize; dx++)
    {
        for(int dz = -initSize; dz <= initSize; dz++)
        {
            getBlockIterator(PositionI(dx * BlockChunk::chunkSizeX, 0, dz * BlockChunk::chunkSizeZ, Dimension::Overworld)); // create chunk
        }
    }
    lightingThread = thread([this]()
    {
        lightingThreadFn();
    });
#if 1 || defined(NDEBUG)
    const int threadCount = 5;
#else
    const int threadCount = 1;
#endif
    for(int i = 0; i < threadCount; i++)
    {
        chunkGeneratingThreads.emplace_back([this]()
        {
            chunkGeneratingThreadFn();
        });
    }
}

World::~World()
{
    assert(viewPoints.empty());
    destructing = true;
    if(lightingThread.joinable())
        lightingThread.join();
    for(auto &t : chunkGeneratingThreads)
        if(t.joinable())
            t.join();
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
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void World::chunkGeneratingThreadFn()
{
    WorldLockManager lock_manager;
    BlockChunkMap *chunks = &physicsWorld->chunks;
    while(!destructing)
    {
        bool didAnything = false;
        BlockChunk *bestChunk = nullptr;
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
            if(std::isnan(currentChunkPriority))
                continue;
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
            debugLog << L"generating " << chunk->basePosition << L" " << chunkPriority << std::endl;
            didAnything = true;

            {
                WorldLockManager new_lock_manager;
                World newWorld(worldGeneratorSeed, nullptr, internal_construct_flag());
                worldGenerator->generateChunk(chunk->basePosition, newWorld, new_lock_manager);
#if 0
                new_lock_manager.clear();
                newWorld.lightingThread = thread([&]()
                {
                    newWorld.lightingThreadFn();
                });
                for(int i = 0; i < 100 && !newWorld.isLightingStable() && !destructing; i++)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                newWorld.destructing = true;
#endif

                typedef checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> BlockArrayType;
                std::unique_ptr<BlockArrayType> blocks(new BlockArrayType);
                BlockIterator cbi = newWorld.getBlockIterator(chunk->basePosition);
                for(auto i = BlockChunkRelativePositionIterator::begin(); i != BlockChunkRelativePositionIterator::end(); i++)
                {
                    BlockIterator bi = cbi;
                    bi.moveBy(*i);
                    blocks->at(i->x)[i->y][i->z] = bi.get(new_lock_manager);
                }
                new_lock_manager.clear();
                setBlockRange(chunk->basePosition, chunk->basePosition + VectorI(BlockChunk::chunkSizeX - 1, BlockChunk::chunkSizeY - 1, BlockChunk::chunkSizeZ - 1), lock_manager, *blocks, VectorI(0));
            }

            chunk->chunkVariables.generated = true;
        }

        if(!didAnything)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

namespace
{
float boxDistanceSquared(VectorF minCorner, VectorF maxCorner, VectorF pos)
{
    VectorF closestPoint(limit(pos.x, minCorner.x, maxCorner.x),
                         limit(pos.y, minCorner.y, maxCorner.y),
                         limit(pos.z, minCorner.z, maxCorner.z));
    return absSquared(closestPoint - pos);
}
}

float World::getChunkGeneratePriority(BlockIterator bi, WorldLockManager &lock_manager) // low values mean high priority, NAN means don't generate
{
    if(bi.position().y != 0)
        return NAN;
    std::unique_lock<std::mutex> lockIt(viewPointsLock);
    float retval;
    bool retvalSet = false;
    PositionF chunkMinCorner = bi.position();
    PositionF chunkMaxCorner = chunkMinCorner + VectorF(BlockChunk::chunkSizeX, BlockChunk::chunkSizeY, BlockChunk::chunkSizeZ);
    PositionF chunkMinCornerXZ = chunkMinCorner;
    PositionF chunkMaxCornerXZ = chunkMaxCorner;
    chunkMinCornerXZ.y = 0;
    chunkMaxCornerXZ.y = 0;
    for(ViewPoint *viewPoint : viewPoints)
    {
        PositionF pos;
        std::int32_t viewDistance;
        viewPoint->getPositionAndViewDistance(pos, viewDistance);
        PositionF posXZ = pos;
        posXZ.y = 0;
        float distSquared = boxDistanceSquared(chunkMinCornerXZ, chunkMaxCornerXZ, posXZ);
        if(pos.d != bi.position().d || distSquared > 1.2f * (float)viewDistance * viewDistance)
            continue;
        if(!retvalSet || retval > distSquared)
        {
            retval = distSquared;
            retvalSet = true;
        }
    }
    if(!retvalSet)
        return NAN;
    return retval;
}
}
}
