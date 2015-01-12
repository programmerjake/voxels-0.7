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
#include "util/spin_lock.h"
#include "util/parallel_map.h"

namespace programmerjake
{
namespace voxels
{
class WorldGenerator;

class World final
{
    World(const World &) = delete;
    const World &operator =(const World &) = delete;
public:
    typedef std::uint64_t SeedType;
    static SeedType makeSeed(std::wstring seed)
    {
        SeedType retval = ((SeedType)0xF9CCC7E0 << 32) + 0x987EEEE5; // large prime number
        for(wchar_t ch : seed)
        {
            retval = 23 * retval + (SeedType)ch;
        }
        return retval;
    }
public:
    SeedType getWorldGeneratorSeed() const
    {
        return worldGeneratorSeed;
    }
public:
    World(SeedType seed, std::shared_ptr<const WorldGenerator> worldGenerator)
        : physicsWorld(std::make_shared<PhysicsWorld>()), worldGenerator(worldGenerator), worldGeneratorSeed(seed)
    {
    }
    World(std::wstring seed, std::shared_ptr<const WorldGenerator> worldGenerator)
        : World(makeSeed(seed), worldGenerator)
    {
    }
    explicit World(SeedType seed);
    explicit World(std::wstring seed)
        : World(makeSeed(seed))
    {
    }
    explicit World();
    ~World();
    inline double getCurrentTime() const
    {
        return physicsWorld->getCurrentTime();
    }
    float scheduleBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind, float timeLeft) // returns the time left for the previous block update or -1
    {
        BlockChunkBlock &b = bi.getBlock(lock_manager);
        BlockUpdate *
    }
    float deleteBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind) // returns the time left for the previous block update or -1
    {
        return scheduleBlockUpdate(bi, lock_manager, kind, -1);
    }
    void setBlock(BlockIterator bi, WorldLockManager &lock_manager, Block newBlock)
    {
        BlockChunkBlock &b = bi.getBlock(lock_manager);
        b.block = newBlock;
        b.lightingValid = false;
        bi.getSubchunk().cachedMesh = nullptr;
        bi.chunk->chunkVariables.cachedMesh = nullptr;
        for(BlockUpdateKind kind : enum_traits<BlockUpdateKind>())
        {
            scheduleBlockUpdate(bi, lock_manager, kind, 0);
            #warning fix
        }
    }
    BlockIterator getBlockIterator(PositionI position) const
    {
        return physicsWorld->getBlockIterator(position);
    }
private:
    std::shared_ptr<PhysicsWorld> physicsWorld;
    std::shared_ptr<const WorldGenerator> worldGenerator;
    SeedType worldGeneratorSeed;
};
}
}

#include "world/world_generator.h"

#endif // WORLD_H_INCLUDED
