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
    /** @brief reschedule a block update
     *
     * @param bi a <code>BlockIterator</code> to the block to reschedule the block update for
     * @param lock_manager this thread's <code>WorldLockManager</code>
     * @param kind the kind of block update
     * @param updateTimeFromNow how far in the future to schedule the block update or (for values < 0) a flag to delete the block update
     * @return the time left for the previous block update or -1 if there is no previous block update
     *
     */
    float rescheduleBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind, float updateTimeFromNow)
    {
        BlockChunkBlock &b = bi.getBlock(lock_manager);
        BlockUpdate **ppnode = &b.updateListHead;
        BlockUpdate *pnode = b.updateListHead;
        std::unique_lock<std::mutex> lockIt;
        while(pnode != nullptr)
        {
            if(pnode->kind == kind)
            {
                if(updateTimeFromNow < 0)
                {
                    lockIt = std::unique_lock<std::mutex>(bi.chunk->chunkVariables.blockUpdateListLock);
                    #error finish
                }
            }
        }
    }
    float deleteBlockUpdate(BlockIterator bi, WorldLockManager &lock_manager, BlockUpdateKind kind) // returns the time left for the previous block update or -1
    {
        return rescheduleBlockUpdate(bi, lock_manager, kind, -1);
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
            rescheduleBlockUpdate(bi, lock_manager, kind, kind.defaultPeriod());
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
