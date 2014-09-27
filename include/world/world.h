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
#include "render/renderer.h"
#include "util/cached_variable.h"

using namespace std;

class World final
{
private:
    shared_ptr<PhysicsWorld> physicsWorld;
    struct MeshesWithModifiedFlag
    {
        atomic_bool modified;
        enum_array<Mesh, RenderLayer> meshes;
        MeshWithModifiedFlag(bool modified = true)
            : modified(modified)
        {
        }
    };
    struct RenderCacheChunk
    {
        const PositionI basePosition;
        MeshesWithModifiedFlag overallMeshes;
        static constexpr int GroupingSizeLog2 = 3;
        static constexpr int32_t GroupingSize = 1 << GroupingSizeLog2, GroupingFloorMask = -GroupingSize, GroupingModMask = GroupingSize - 1;
        array<array<array<MeshesWithModifiedFlag, PhysicsWorld::ChunkType::chunkSizeZ / GroupingSize>, PhysicsWorld::ChunkType::chunkSizeY / GroupingSize>, PhysicsWorld::ChunkType::chunkSizeX / GroupingSize> groupMeshes;
        RenderCacheChunk(PositionI basePosition)
            : basePosition(basePosition)
        {
        }
        void invalidatePosition(VectorI relativePosition)
        {
            relativePosition.x >>= GroupingSizeLog2;
            relativePosition.y >>= GroupingSizeLog2;
            relativePosition.z >>= GroupingSizeLog2;
            overallMeshes.modified = true;
            groupMeshes.at(relativePosition.x).at(relativePosition.y).at(relativePosition.z).modified = true;
        }
        const enum_array<Mesh, RenderLayer> &updateGroupMeshes(BlockIterator blockIterator, VectorI relativeGroupBasePosition)
        {
            VectorI groupIndexes = VectorI(relativeGroupBasePosition.x >> GroupingSizeLog2, relativeGroupBasePosition.y >> GroupingSizeLog2, relativeGroupBasePosition.z >> GroupingSizeLog2);
            MeshesWithModifiedFlag &meshes = groupMeshes.at(groupIndexes.x).at(groupIndexes.y).at(groupIndexes.z);
            if(!meshes.modified.exchange(false))
                return meshes.meshes;
            for(RenderLayer rl : enum_traits<RenderLayer>)
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
                        for(RenderLayer rl : enum_traits<RenderLayer>)
                        {
                            block.descriptor->render(block, meshes.meshes[rl], xyzBlockIterator, rl);
                        }
                    }
                }
            }
            return meshes.meshes;
        }
        bool updateMeshes(BlockIterator blockIterator)
        {
            if(!overallMeshes.modified.exchange(false))
                return false;
            for(RenderLayer rl : enum_traits<RenderLayer>)
                overallMeshes.meshes[rl].clear();
            blockIterator.moveTo(basePosition);
            for(int32_t gx = 0; gx < PhysicsWorld::ChunkType::chunkSizeX; gx += GroupingSize)
            {
                for(int32_t gy = 0; gy < PhysicsWorld::ChunkType::chunkSizeY; gy += GroupingSize)
                {
                    for(int32_t gz = 0; gz < PhysicsWorld::ChunkType::chunkSizeZ; gz += GroupingSize)
                    {
                        const enum_array<Mesh, RenderLayer> &meshes = updateGroupMeshes(blockIterator, VectorI(gx, gy, gz));
                        for(RenderLayer rl : enum_traits<RenderLayer>)
                        {
                            overallMeshes.meshes[rl].append(meshes[rl]);
                        }
                    }
                }
            }
            return true;
        }
    };
    unordered_map<PositionI, RenderCacheChunk> renderCacheChunks;
    RenderCacheChunk &getOrMakeRenderCacheChunk(PositionI chunkBasePosition)
    {
        auto iter = renderCacheChunks.find(chunkBasePosition);
        if(iter == renderCacheChunks.end())
            iter = std::get<0>(renderCacheChunks.insert(pair<PositionI, RenderCacheChunk>(chunkBasePosition, RenderCacheChunk(chunkBasePosition)));
        return std::get<1>(*iter);
    }
    const enum_array<Mesh, RenderLayer> &makeChunkMeshes(PositionI chunkBasePosition)
    {
        RenderCacheChunk &c = getOrMakeRenderCacheChunk(chunkBasePosition);
        c.updateMeshes(physicsWorld->getBlockIterator(chunkBasePosition));
        return c.overallMeshes.meshes;
    }
    CachedVariable<enum_array<Mesh, RenderLayer>> cachedMeshes;
public:
    const enum_array<Mesh, RenderLayer> &getCachedMeshes()
    {
        return cachedMeshes.read();
    }
    void updateCachedMeshes(PositionI viewPosition, int32_t viewDistance)
    {
        assert(viewDistance >= 0);
        PositionI minPosition = PhysicsWorld::ChunkType::getChunkBasePosition(viewPosition - VectorI(viewDistance));
        PositionI maxPosition = PhysicsWorld::ChunkType::getChunkBasePosition(viewPosition + VectorI(viewDistance));
        PositionI pos = minPosition;
        for(RenderLayer rl : enum_traits<RenderLayer>)
        {
            cachedMeshes.writeRef()[rl].clear();
        }
        for(pos.x = minPosition.x; pos.x <= maxPosition.x; pos.x += PhysicsWorld::ChunkType::chunkSizeX)
        {
            for(pos.y = minPosition.y; pos.y <= maxPosition.y; pos.y += PhysicsWorld::ChunkType::chunkSizeY)
            {
                for(pos.z = minPosition.z; pos.z <= maxPosition.z; pos.z += PhysicsWorld::ChunkType::chunkSizeZ)
                {
                    const enum_array<Mesh, RenderLayer> &chunkMeshes = makeChunkMeshes(pos);
                    for(RenderLayer rl : enum_traits<RenderLayer>)
                    {
                        cachedMeshes.writeRef()[rl].append(chunkMeshes[rl]);
                    }
                }
            }
        }
        cachedMeshes.finishWrite();
    }
};

#endif // WORLD_H_INCLUDED
