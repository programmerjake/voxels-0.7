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
#include "util/block_chunk.h"
#ifndef WRAPPED_ENTITY_H_INCLUDED
#define WRAPPED_ENTITY_H_INCLUDED

#include "entity/entity.h"
#include "util/block_iterator.h" // for WorldLockManager
#include <mutex>

namespace programmerjake
{
namespace voxels
{
struct WrappedEntity final
{
    Entity entity;
    WrappedEntity *subchunk_next;
    WrappedEntity *subchunk_prev;
    WrappedEntity *chunk_next;
    WrappedEntity *chunk_prev;
    BlockChunkSubchunk *currentSubchunk = nullptr;
    BlockChunk *currentChunk = nullptr;
    void insertInLists(BlockChunk *chunk, BlockChunkSubchunk &subchunk, WorldLockManager &lock_manager)
    {
        subchunk_prev = nullptr;
        chunk_prev = nullptr;
        currentSubchunk = &subchunk;
        currentChunk = chunk;
        lock_manager.block_lock.set(subchunk.lock);
        subchunk_next = subchunk.entityListHead;
        if(subchunk.entityListHead != nullptr)
            subchunk.entityListHead->subchunk_prev = this;
        else
            subchunk.entityListTail = this;
        subchunk.entityListHead = this;
        std::unique_lock<std::recursive_mutex> lockIt(chunk->chunkVariables.entityListLock);
        chunk_next = chunk->chunkVariables.entityListHead;
        if(chunk->chunkVariables.entityListHead != nullptr)
            chunk->chunkVariables.entityListHead->chunk_prev = this;
        else
            chunk->chunkVariables.entityListTail = this;
        chunk->chunkVariables.entityListHead = this;
    }
    void removeFromLists(WorldLockManager &lock_manager)
    {
        lock_manager.block_lock.set(currentSubchunk->lock);
        if(subchunk_next == nullptr)
            currentSubchunk->entityListTail = subchunk_prev;
        else
            subchunk_next->subchunk_prev = subchunk_prev;
        if(subchunk_prev == nullptr)
            currentSubchunk->entityListHead = subchunk_next;
        else
            subchunk_prev->subchunk_next = subchunk_next;
        subchunk_prev = nullptr;
        subchunk_next = nullptr;
        currentSubchunk = nullptr;
        std::unique_lock<std::recursive_mutex> lockIt(currentChunk->chunkVariables.entityListLock);
        if(chunk_next == nullptr)
            currentChunk->chunkVariables.entityListTail = chunk_prev;
        else
            chunk_next->chunk_prev = chunk_prev;
        if(chunk_prev == nullptr)
            currentChunk->chunkVariables.entityListHead = chunk_next;
        else
            chunk_prev->subchunk_next = chunk_next;
        chunk_prev = nullptr;
        chunk_next = nullptr;
        currentChunk = nullptr;
    }
    void moveToNewLists(BlockChunk *chunk, BlockChunkSubchunk &subchunk, WorldLockManager &lock_manager)
    {
        if(currentChunk == chunk && currentSubchunk == &subchunk)
            return;
        removeFromLists(lock_manager);
        insertInLists(chunk, subchunk, lock_manager);
    }
};
}
}

#endif // WRAPPED_ENTITY_H_INCLUDED
