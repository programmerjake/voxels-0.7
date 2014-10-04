#include "entity/entity.h"
#ifndef WORLD_H_INCLUDED
#define WORLD_H_INCLUDED

#include "physics/physics.h"
#include "util/position.h"
#include "render/mesh.h"
#include "render/render_layer.h"
#include "util/enum_traits.h"
#include <memory>
#include <atomic>
#include <array>
#include <mutex>
#include <cstdint>
#include <cwchar>
#include <string>
#include "render/renderer.h"
#include "util/cached_variable.h"
#include "platform/platform.h"
#include "util/unlock_guard.h"
#include "util/linked_map.h"
#include "ray_casting/ray_casting.h"
#include "util/flag.h"

using namespace std;

class WorldGenerator;

class World final
{
public:
    typedef PhysicsWorld::ChunkType ChunkType;
    typedef uint64_t SeedType;
private:
    shared_ptr<PhysicsWorld> physicsWorld;
    shared_ptr<const WorldGenerator> worldGenerator;
    SeedType worldGeneratorSeed;
public:
    SeedType getWorldGeneratorSeed() const
    {
        return worldGeneratorSeed;
    }
private:
    struct MeshesWithModifiedFlag
    {
        atomic_bool modified;
        enum_array<Mesh, RenderLayer> meshes;
        MeshesWithModifiedFlag(bool modified = true)
            : modified(modified)
        {
        }
        MeshesWithModifiedFlag(const MeshesWithModifiedFlag &rt)
            : modified((bool)rt.modified), meshes(rt.meshes)
        {
        }
    };
    Lighting::LightValueType skyLighting = Lighting::maxLight;
    struct RenderCacheChunk
    {
        const PositionI basePosition;
        Lighting::LightValueType skyLighting;
        MeshesWithModifiedFlag overallMeshes;
        static constexpr int GroupingSizeLog2 = 3;
        static constexpr int32_t GroupingSize = 1 << GroupingSizeLog2, GroupingFloorMask = -GroupingSize, GroupingModMask = GroupingSize - 1;
        array<array<array<MeshesWithModifiedFlag, ChunkType::chunkSizeZ / GroupingSize>, ChunkType::chunkSizeY / GroupingSize>, ChunkType::chunkSizeX / GroupingSize> groupMeshes;
        RenderCacheChunk(PositionI basePosition, Lighting::LightValueType skyLighting)
            : basePosition(basePosition), skyLighting(skyLighting)
        {
        }
    private:
        VectorI getGroupIndexes(VectorI relativePosition)
        {
            return VectorI(relativePosition.x >> GroupingSizeLog2, relativePosition.y >> GroupingSizeLog2, relativePosition.z >> GroupingSizeLog2);
        }
    public:
        void invalidatePosition(VectorI relativePosition)
        {
            VectorI groupIndexes = getGroupIndexes(relativePosition);
            overallMeshes.modified = true;
            groupMeshes.at(groupIndexes.x).at(groupIndexes.y).at(groupIndexes.z).modified = true;
        }
        void invalidateRange(VectorI minRelativePosition, VectorI maxRelativePosition)
        {
            overallMeshes.modified = true;
            minRelativePosition.x = max(minRelativePosition.x, 0);
            minRelativePosition.y = max(minRelativePosition.y, 0);
            minRelativePosition.z = max(minRelativePosition.z, 0);
            maxRelativePosition.x = min(maxRelativePosition.x, ChunkType::chunkSizeX - 1);
            maxRelativePosition.y = min(maxRelativePosition.y, ChunkType::chunkSizeY - 1);
            maxRelativePosition.z = min(maxRelativePosition.z, ChunkType::chunkSizeZ - 1);
            VectorI minGroupIndexes = getGroupIndexes(minRelativePosition);
            VectorI maxGroupIndexes = getGroupIndexes(maxRelativePosition);
            for(VectorI groupIndexes = minGroupIndexes; groupIndexes.x <= maxGroupIndexes.x; groupIndexes.x++)
            {
                for(groupIndexes.y = minGroupIndexes.y; groupIndexes.y <= maxGroupIndexes.y; groupIndexes.y++)
                {
                    for(groupIndexes.z = minGroupIndexes.z; groupIndexes.z <= maxGroupIndexes.z; groupIndexes.z++)
                    {
                        groupMeshes.at(groupIndexes.x).at(groupIndexes.y).at(groupIndexes.z).modified = true;
                    }
                }
            }
        }
    private:
        const enum_array<Mesh, RenderLayer> &updateGroupMeshes(BlockIterator blockIterator, VectorI relativeGroupBasePosition, Lighting::LightValueType currentSkyLighting)
        {
            VectorI groupIndexes = getGroupIndexes(relativeGroupBasePosition);
            MeshesWithModifiedFlag &meshes = groupMeshes.at(groupIndexes.x).at(groupIndexes.y).at(groupIndexes.z);
            if(!meshes.modified.exchange(false) && currentSkyLighting == skyLighting)
                return meshes.meshes;
            for(RenderLayer rl : enum_traits<RenderLayer>())
                meshes.meshes[rl].clear();
            BlockIterator xBlockIterator = blockIterator;
            xBlockIterator.moveTo(relativeGroupBasePosition + basePosition);
            for(int32_t rx = 0; rx < GroupingSize; rx++, xBlockIterator.moveTowardPX())
            {
                BlockIterator xyBlockIterator = xBlockIterator;
                for(int32_t ry = 0; ry < GroupingSize; ry++, xyBlockIterator.moveTowardPY())
                {
                    BlockIterator xyzBlockIterator = xyBlockIterator;
                    for(int32_t rz = 0; rz < GroupingSize; rz++, xyzBlockIterator.moveTowardPZ())
                    {
                        const Block &block = *xyzBlockIterator;
                        if(!block)
                            continue;
                        for(RenderLayer rl : enum_traits<RenderLayer>())
                        {
                            block.descriptor->render(block, meshes.meshes[rl], xyzBlockIterator, rl);
                        }
                    }
                }
            }
            return meshes.meshes;
        }
    public:
        bool updateMeshes(BlockIterator blockIterator, Lighting::LightValueType currentSkyLighting)
        {
            if(!overallMeshes.modified.exchange(false) && currentSkyLighting == skyLighting)
                return false;
            for(RenderLayer rl : enum_traits<RenderLayer>())
                overallMeshes.meshes[rl].clear();
            blockIterator.moveTo(basePosition);
            for(int32_t gx = 0; gx < ChunkType::chunkSizeX; gx += GroupingSize)
            {
                for(int32_t gy = 0; gy < ChunkType::chunkSizeY; gy += GroupingSize)
                {
                    for(int32_t gz = 0; gz < ChunkType::chunkSizeZ; gz += GroupingSize)
                    {
                        const enum_array<Mesh, RenderLayer> &meshes = updateGroupMeshes(blockIterator, VectorI(gx, gy, gz), currentSkyLighting);
                        for(RenderLayer rl : enum_traits<RenderLayer>())
                        {
                            overallMeshes.meshes[rl].append(meshes[rl]);
                        }
                    }
                }
            }
            skyLighting = currentSkyLighting;
            return true;
        }
    };
    unordered_map<PositionI, RenderCacheChunk> renderCacheChunks;
    RenderCacheChunk &getOrMakeRenderCacheChunk(PositionI chunkBasePosition)
    {
        auto iter = renderCacheChunks.find(chunkBasePosition);
        if(iter == renderCacheChunks.end())
            iter = std::get<0>(renderCacheChunks.insert(pair<PositionI, RenderCacheChunk>(chunkBasePosition, RenderCacheChunk(chunkBasePosition))));
        return std::get<1>(*iter);
    }
    const enum_array<Mesh, RenderLayer> &makeChunkMeshes(PositionI chunkBasePosition)
    {
        RenderCacheChunk &c = getOrMakeRenderCacheChunk(chunkBasePosition);
        c.updateMeshes(physicsWorld->getBlockIterator(chunkBasePosition), skyLighting);
        return c.overallMeshes.meshes;
    }
    CachedVariable<enum_array<shared_ptr<Mesh>, RenderLayer>> meshes;
    atomic_bool cachedMeshesInvalid;
    CachedVariable<enum_array<shared_ptr<CachedMesh>, RenderLayer>> cachedMeshes;
    mutex generateChunkMutex;
    list<shared_ptr<World>> generatedChunksList;
    unordered_set<PositionI> generatedChunksSet;
    unordered_set<PositionI> needGenerateChunksSet;
    list<PositionI> needGenerateChunksList;
    linked_map<PositionI, list<Entity>> entities;
    size_t entityCount = 0;
    enum class BlockUpdateType
    {
        Lighting,
        DEFINE_ENUM_LIMITS(Lighting, Lighting)
    };
    typedef PositionI BlockUpdate;
    struct BlockUpdateChunk
    {
        const PositionI chunkPosition;
        typedef atomic_bool BaseType;
        array<array<array<enum_array<BaseType, BlockUpdateType>, ChunkType::chunkSizeZ>, ChunkType::chunkSizeY>, ChunkType::chunkSizeX> blocks;
        explicit BlockUpdateChunk(PositionI chunkPosition)
            : chunkPosition(chunkPosition)
        {
            for(auto &i : blocks)
            {
                for(auto &j : i)
                {
                    for(auto &k : j)
                    {
                        for(BaseType &v : k)
                        {
                            v = false;
                        }
                    }
                }
            }
        }
        BlockUpdateChunk(const BlockUpdateChunk &rt)
            : BlockUpdateChunk(rt.chunkPosition)
        {
        }
        BlockUpdateChunk(BlockUpdateChunk &&rt)
            : BlockUpdateChunk(rt.chunkPosition)
        {
        }
        enum_array<BaseType, BlockUpdateType> &at(VectorI pos)
        {
            return blocks.at(pos.x).at(pos.y).at(pos.z);
        }
    };
    mutex blockUpdateMutex;
    unordered_map<PositionI, BlockUpdateChunk> blockUpdateChunks;
    BlockUpdateChunk &getOrMakeBlockUpdateChunk(PositionI chunkBasePosition)
    {
        lock_guard<mutex> lockIt(blockUpdateMutex);
        auto iter = blockUpdateChunks.find(chunkBasePosition);
        if(iter == blockUpdateChunks.end())
            iter = std::get<0>(blockUpdateChunks.insert(make_pair(chunkBasePosition, BlockUpdateChunk(chunkBasePosition))));
        return std::get<1>(*iter);
    }
    enum_array<mutex, BlockUpdateType> blockUpdateMutexes;
    enum_array<flag, BlockUpdateType> blockUpdateFlags;
    enum_array<list<BlockUpdate>, BlockUpdateType> blockUpdates;
    static thread_local list<BlockUpdate> freeBlockUpdates;
    bool hasBlockUpdate(BlockUpdateType t, BlockUpdate bu)
    {
        BlockUpdateChunk &c = getOrMakeBlockUpdateChunk(ChunkType::getChunkBasePosition(bu));
        return c.at(ChunkType::getChunkRelativePosition(bu)).at(t);
    }
    void addBlockUpdate(BlockUpdateType t, BlockUpdate bu)
    {
        BlockUpdateChunk &c = getOrMakeBlockUpdateChunk(ChunkType::getChunkBasePosition(bu));
        if(c.at(ChunkType::getChunkRelativePosition(bu)).at(t).exchange(true))
            return;
        lock_guard<mutex> lockIt(blockUpdateMutexes[t]);
        if(freeBlockUpdates.empty())
            blockUpdates[t].push_back(bu);
        else
        {
            blockUpdates[t].splice(blockUpdates[t].end(), freeBlockUpdates, freeBlockUpdates.begin());
            blockUpdates[t].back() = bu;
        }
        blockUpdateFlags[t] = true;
    }
    void addAllBlockUpdates(PositionI pos)
    {
        BlockUpdateChunk &c = getOrMakeBlockUpdateChunk(ChunkType::getChunkBasePosition(pos));
        for(BlockUpdateType t : enum_traits<BlockUpdateType>())
        {
            if(c.at(ChunkType::getChunkRelativePosition(pos)).at(t).exchange(true))
                continue;
            lock_guard<mutex> lockIt(blockUpdateMutexes[t]);
            BlockUpdate bu = pos;
            if(freeBlockUpdates.empty())
                blockUpdates[t].push_back(bu);
            else
            {
                blockUpdates[t].splice(blockUpdates[t].end(), freeBlockUpdates, freeBlockUpdates.begin());
                blockUpdates[t].back() = bu;
            }
            blockUpdateFlags[t] = true;
        }
    }
    void addAllBlockUpdatesInChunkRange(VectorI rmin, VectorI rmax, PositionI chunkBasePosition)
    {
        BlockUpdateChunk &c = getOrMakeBlockUpdateChunk(chunkBasePosition);
        for(BlockUpdateType t : enum_traits<BlockUpdateType>())
        {
            lock_guard<mutex> lockIt(blockUpdateMutexes[t]);
            rmin.x = max(rmin.x, 0);
            rmin.y = max(rmin.y, 0);
            rmin.z = max(rmin.z, 0);
            rmax.x = min(rmax.x, ChunkType::chunkSizeX - 1);
            rmax.y = min(rmax.y, ChunkType::chunkSizeY - 1);
            rmax.z = min(rmax.z, ChunkType::chunkSizeZ - 1);
            for(VectorI rpos = rmin; rpos.x <= rmax.x; rpos.x++)
            {
                for(rpos.y = rmin.y; rpos.y <= rmax.y; rpos.y++)
                {
                    for(rpos.z = rmin.z; rpos.z <= rmax.z; rpos.z++)
                    {
                        if(c.at(rpos).at(t).exchange(true))
                            continue;
                        BlockUpdate bu = rpos + chunkBasePosition;
                        if(freeBlockUpdates.empty())
                            blockUpdates[t].push_back(bu);
                        else
                        {
                            blockUpdates[t].splice(blockUpdates[t].end(), freeBlockUpdates, freeBlockUpdates.begin());
                            blockUpdates[t].back() = bu;
                        }
                    }
                }
            }
            blockUpdateFlags[t] = true;
        }
    }
    bool anyBlockUpdate(BlockUpdateType t)
    {
        return blockUpdateFlags[t];
    }
    bool removeBlockUpdate(BlockUpdateType t, BlockUpdate &retval)
    {
        lock_guard<mutex> lockIt(blockUpdateMutexes[t]);
        if(blockUpdates[t].empty())
        {
            return false;
        }
        retval = blockUpdates[t].front();
        freeBlockUpdates.splice(freeBlockUpdates.begin(), blockUpdates[t], blockUpdates[t].begin());
        if(freeBlockUpdates.size() >= 1000000)
            freeBlockUpdates.pop_front();
        BlockUpdateChunk &c = getOrMakeBlockUpdateChunk(ChunkType::getChunkBasePosition(retval));
        c.at(ChunkType::getChunkRelativePosition(retval)).at(t) = false;
        if(blockUpdates[t].empty())
        {
            blockUpdateFlags[t] = false;
        }
        return true;
    }
public:
    World(SeedType seed, shared_ptr<const WorldGenerator> worldGenerator)
        : physicsWorld(make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed), cachedMeshesInvalid(true)
    {
    }
    explicit World(SeedType seed);
    explicit World(wstring seed);
    World();
    void scheduleGenerateChunk(PositionI cpos)
    {
        lock_guard<mutex> lockIt(generateChunkMutex);
        if(generatedChunksSet.count(cpos) != 0)
            return;
        if(std::get<1>(needGenerateChunksSet.insert(cpos)))
            needGenerateChunksList.push_back(cpos);
    }
    const enum_array<shared_ptr<Mesh>, RenderLayer> &getMeshes()
    {
        return meshes.read();
    }
    const enum_array<shared_ptr<CachedMesh>, RenderLayer> &getCachedMeshes()
    {
        return cachedMeshes.read();
    }
    void updateMeshes(PositionI viewPosition, int32_t viewDistance)
    {
        assert(viewDistance >= 0);
        PositionI minPosition = ChunkType::getChunkBasePosition(viewPosition - VectorI(viewDistance));
        PositionI maxPosition = ChunkType::getChunkBasePosition(viewPosition + VectorI(viewDistance));
        PositionI pos = minPosition;
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            meshes.writeRef()[rl] = make_shared<Mesh>();
        }
        for(pos.x = minPosition.x; pos.x <= maxPosition.x; pos.x += ChunkType::chunkSizeX)
        {
            for(pos.y = minPosition.y; pos.y <= maxPosition.y; pos.y += ChunkType::chunkSizeY)
            {
                for(pos.z = minPosition.z; pos.z <= maxPosition.z; pos.z += ChunkType::chunkSizeZ)
                {
                    scheduleGenerateChunk(pos);
                    const enum_array<Mesh, RenderLayer> &chunkMeshes = makeChunkMeshes(pos);
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        meshes.writeRef()[rl]->append(chunkMeshes[rl]);
                    }
                }
            }
        }
        meshes.finishWrite();
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            meshes.writeRef()[rl] = nullptr;
        }
        cachedMeshesInvalid = true;
    }
    void updateCachedMeshes()
    {
        if(!cachedMeshesInvalid.exchange(false))
            return;
        enum_array<shared_ptr<Mesh>, RenderLayer> meshes = getMeshes();
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            if(meshes[rl] == nullptr)
                meshes[rl] = make_shared<Mesh>();
            cachedMeshes.writeRef()[rl] = makeCachedMesh(*meshes[rl]);
        }
        cachedMeshes.finishWrite();
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            cachedMeshes.writeRef()[rl] = nullptr;
        }
    }
    BlockIterator getBlockIterator(PositionI pos)
    {
        return physicsWorld->getBlockIterator(pos);
    }
    const Block getBlock(PositionI pos)
    {
        return physicsWorld->getBlock(pos);
    }
    void invalidate(PositionI pos)
    {
        physicsWorld->getOrAddChunk(ChunkType::getChunkBasePosition(pos)).onChange();
        getOrMakeRenderCacheChunk(ChunkType::getChunkBasePosition(pos)).invalidatePosition((VectorI)ChunkType::getChunkRelativePosition(pos));
        addAllBlockUpdates(pos);
    }
    void invalidateRange(PositionI minPosition, VectorI maxPosition)
    {
        PositionI minCPos = ChunkType::getChunkBasePosition(minPosition);
        VectorI maxCPos = ChunkType::getChunkBasePosition(maxPosition);
        for(PositionI cpos = minCPos; cpos.x <= maxCPos.x; cpos.x += ChunkType::chunkSizeX)
        {
            for(cpos.y = minCPos.y; cpos.y <= maxCPos.y; cpos.y += ChunkType::chunkSizeY)
            {
                for(cpos.z = minCPos.z; cpos.z <= maxCPos.z; cpos.z += ChunkType::chunkSizeZ)
                {
                    physicsWorld->getOrAddChunk(cpos).onChange();
                    getOrMakeRenderCacheChunk(cpos).invalidateRange(minPosition - cpos, maxPosition - cpos);
                    addAllBlockUpdatesInChunkRange(minPosition - cpos, maxPosition - cpos, cpos);
                }
            }
        }
    }
    void invalidateRange(VectorI minPosition, PositionI maxPosition)
    {
        invalidateRange(PositionI(minPosition, maxPosition.d), (VectorI)maxPosition);
    }
    void invalidateRange(PositionI minPosition, PositionI maxPosition)
    {
        assert(minPosition.d == maxPosition.d);
        invalidateRange(minPosition, (VectorI)maxPosition);
    }
    void setBlock(BlockIterator bi, Block block)
    {
        PositionI pos = bi.position();
        VectorI relativePosition = bi.currentRelativePosition;
        ChunkType &chunk = *bi.chunk;
        Lighting oldLighting = chunk.blocks[relativePosition.x][relativePosition.y][relativePosition.z].lighting;
        block.createNewLighting(oldLighting);
        chunk.blocks[relativePosition.x][relativePosition.y][relativePosition.z] = block;
        invalidateRange(pos - VectorI(1), pos + VectorI(1));
    }
    void setBlock(PositionI pos, Block block)
    {
        PositionI basePosition = ChunkType::getChunkBasePosition(pos);
        VectorI relativePosition = ChunkType::getChunkRelativePosition(pos);
        ChunkType &chunk = physicsWorld->getOrAddChunk(basePosition);
        Lighting oldLighting = chunk.blocks[relativePosition.x][relativePosition.y][relativePosition.z].lighting;
        block.createNewLighting(oldLighting);
        chunk.blocks[relativePosition.x][relativePosition.y][relativePosition.z] = block;
        invalidateRange(pos - VectorI(1), pos + VectorI(1));
    }
private:
    void setBlockLighting(BlockIterator bi, Lighting lighting)
    {
        PositionI pos = bi.position();
        VectorI relativePosition = bi.currentRelativePosition;
        ChunkType &chunk = *bi.chunk;
        chunk.blocks[relativePosition.x][relativePosition.y][relativePosition.z].lighting = lighting;
        invalidateRange(pos - VectorI(1), pos + VectorI(1));
    }
    template <bool ignoreNulls, size_t xSize, size_t ySize, size_t zSize>
    void chunkSetBlockRange(VectorI relativeBasePos, ChunkType &chunk, const array<array<array<Block, zSize>, ySize>, xSize> &blocks)
    {
        VectorI minRPos = VectorI(max<int32_t>(0, relativeBasePos.x), max<int32_t>(0, relativeBasePos.y), max<int32_t>(0, relativeBasePos.z));
        VectorI maxRPos = VectorI(min<int32_t>(ChunkType::chunkSizeX - 1, relativeBasePos.x + xSize - 1), min<int32_t>(ChunkType::chunkSizeY - 1, relativeBasePos.y + ySize - 1), min<int32_t>(ChunkType::chunkSizeZ - 1, relativeBasePos.z + zSize - 1));
        for(VectorI rpos = minRPos; rpos.x <= maxRPos.x; rpos.x++)
        {
            for(rpos.y = minRPos.y; rpos.y <= maxRPos.y; rpos.y++)
            {
                for(rpos.z = minRPos.z; rpos.z <= maxRPos.z; rpos.z++)
                {
                    VectorI apos = rpos - relativeBasePos;
                    const Block &block = blocks.at(apos.x).at(apos.y).at(apos.z);
                    if(ignoreNulls)
                        if(!block)
                            continue;
                    chunk.blocks[rpos.x][rpos.y][rpos.z] = block;
                }
            }
        }
    }
public:
    template <bool ignoreNulls, size_t xSize, size_t ySize, size_t zSize>
    void setBlockRange(PositionI basePos, const array<array<array<Block, zSize>, ySize>, xSize> &blocks)
    {
        PositionI minCPos = ChunkType::getChunkBasePosition(basePos);
        VectorI maxCPos = ChunkType::getChunkBasePosition(basePos + VectorI(xSize - 1, ySize - 1, zSize - 1));
        for(PositionI cpos = minCPos; cpos.x <= maxCPos.x; cpos.x += ChunkType::chunkSizeX)
        {
            for(cpos.y = minCPos.y; cpos.y <= maxCPos.y; cpos.y += ChunkType::chunkSizeY)
            {
                for(cpos.z = minCPos.z; cpos.z <= maxCPos.z; cpos.z += ChunkType::chunkSizeZ)
                {
                    chunkSetBlockRange<ignoreNulls>(basePos - cpos, physicsWorld->getOrAddChunk(cpos), blocks);
                }
            }
        }
        invalidateRange(basePos - VectorI(1), basePos + VectorI(xSize, ySize, zSize));
    }
    void merge(World &other)
    {
        assert(&other != this);
        for(auto &element : other.physicsWorld->chunks)
        {
            const ChunkType &chunk = std::get<1>(element);
            setBlockRange<true>(chunk.basePosition, chunk.blocks);
        }
        for(auto e : other.generatedChunksSet)
        {
            generatedChunksSet.insert(e);
        }
        for(pair<const PositionI, list<Entity>> &v : other.entities)
        {
            list<Entity> &src = std::get<1>(v);
            for(Entity &e : src)
            {
                if(e.physicsObject != nullptr)
                {
                    e.physicsObject->transferToNewWorld(physicsWorld);
                }
            }
            list<Entity> &dest = entities[std::get<0>(v)];
            dest.splice(dest.end(), src);
        }
        entityCount += other.entityCount;
        other.entityCount = 0;
    }
    bool mergeGeneratedChunk()
    {
        shared_ptr<World> generatedChunk;
        {
            lock_guard<mutex> lockIt(generateChunkMutex);
            if(generatedChunksList.empty())
                return false;
            generatedChunk = generatedChunksList.front();
            generatedChunksList.pop_front();
        }
        merge(*generatedChunk);
        return true;
    }
    void mergeGeneratedChunks()
    {
        list<shared_ptr<World>> generatedChunks;
        {
            lock_guard<mutex> lockIt(generateChunkMutex);
            std::swap(generatedChunks, generatedChunksList);
        }
        for(shared_ptr<World> &generatedChunk : generatedChunks)
        {
            merge(*generatedChunk);
            generatedChunk = nullptr;
        }
    }
    bool generateChunk();
    bool needGenerateChunks()
    {
        lock_guard<mutex> lockIt(generateChunkMutex);
        if(needGenerateChunksList.empty())
            return false;
        return true;
    }
private:
    bool putEntityInProperChunk(list<Entity> &srcList, list<Entity>::iterator iter)
    {
        if(!*iter)
            return false;
        if(!iter->physicsObject)
            return false;
        if(iter->physicsObject->isDestroyed())
            return false;
        PositionF pos = iter->physicsObject->getPosition();
        PositionI chunkPos = ChunkType::getChunkBasePosition((PositionI)pos);
        list<Entity> &l = entities[chunkPos];
        if(&l != &srcList)
            l.splice(l.end(), srcList, iter);
        return true;
    }
public:
    void updateLighting();
    const flag &getNeedLightingUpdateFlag() const
    {
        return blockUpdateFlags[BlockUpdateType::Lighting];
    }
    void move(double deltaTime)
    {
        vector<pair<list<Entity> *, list<Entity>::iterator>> entitiesList;
        entitiesList.reserve(entityCount);
        for(pair<const PositionI, list<Entity>> &v : entities)
        {
            list<Entity> &l = std::get<1>(v);
            for(auto i = l.begin(); i != l.end();)
            {
                if(*i && i->physicsObject && !i->physicsObject->isDestroyed())
                {
                    entitiesList.push_back(make_pair(&l, i++));
                }
                else
                {
                    i = l.erase(i);
                    entityCount--;
                }
            }
        }
        for(auto i : entitiesList)
        {
            assert(*std::get<1>(i));
            std::get<1>(i)->descriptor->moveStep(*std::get<1>(i), *this, deltaTime);
        }
        physicsWorld->stepTime(deltaTime);
        for(auto i : entitiesList)
        {
            if(!*std::get<1>(i))
                continue;
            putEntityInProperChunk(*std::get<0>(i), std::get<1>(i));
        }
        updateLighting();
    }
    void addEntity(EntityDescriptorPointer descriptor, PositionF position, VectorF velocity = VectorF(0), shared_ptr<void> data = nullptr)
    {
        assert(descriptor != nullptr);
        Entity e = Entity(descriptor, descriptor->physicsObjectConstructor->make(position, velocity, physicsWorld), data);
        list<Entity> l;
        l.push_back(e);
        if(putEntityInProperChunk(l, l.begin()))
        {
            entityCount++;
        }
        else
        {
            assert(false);
        }
    }
    void generateEntityMeshes(enum_array<Mesh, RenderLayer> &entityMeshes, PositionI viewPosition, int32_t viewDistance)
    {
        assert(viewDistance >= 0);
        PositionI minPosition = ChunkType::getChunkBasePosition(viewPosition - VectorI(viewDistance));
        PositionI maxPosition = ChunkType::getChunkBasePosition(viewPosition + VectorI(viewDistance));
        PositionI pos = minPosition;
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            entityMeshes[rl].clear();
        }
        for(pos.x = minPosition.x; pos.x <= maxPosition.x; pos.x += ChunkType::chunkSizeX)
        {
            for(pos.y = minPosition.y; pos.y <= maxPosition.y; pos.y += ChunkType::chunkSizeY)
            {
                for(pos.z = minPosition.z; pos.z <= maxPosition.z; pos.z += ChunkType::chunkSizeZ)
                {
                    for(RenderLayer rl : enum_traits<RenderLayer>())
                    {
                        const list<Entity> &l = entities[pos];
                        for(const Entity & e : l)
                        {
                            if(!e)
                                continue;
                            e.descriptor->render(e, entityMeshes[rl], rl);
                        }
                    }
                }
            }
        }
    }
    inline double getCurrentTime() const
    {
        return physicsWorld->getCurrentTime();
    }
};

#include "world/world_generator.h"

#endif // WORLD_H_INCLUDED
