/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "util/block_chunk.h"
#include "util/wrapped_entity.h"
#include "util/logging.h"
#include "platform/platform.h"
#include <cassert>
#include "util/tls.h"

namespace programmerjake
{
namespace voxels
{
BlockChunkChunkVariables::~BlockChunkChunkVariables()
{
    TLS &tls = TLS::getSlow();
    while(blockUpdateListHead != nullptr)
    {
        BlockUpdate *deleteMe = blockUpdateListHead;
        blockUpdateListHead = blockUpdateListHead->chunk_next;
        BlockUpdate::free(deleteMe, tls);
    }
}

BlockChunk::BlockChunk(PositionI basePosition, IndirectBlockChunk *indirectBlockChunk)
    : BasicBlockChunk(basePosition), objectCounter(), indirectBlockChunk(indirectBlockChunk)
{
    assert(indirectBlockChunk);
#ifdef USE_SEMAPHORE_FOR_BLOCK_CHUNK
    for(int x = 0; x < subchunkCountX; x++)
    {
        for(int y = 0; y < subchunkCountY; y++)
        {
            for(int z = 0; z < subchunkCountZ; z++)
            {
                BlockChunkSubchunk &subchunk = subchunks[x][y][z];
                subchunk.lockImp.chunkSemaphore = indirectBlockChunk->chunkVariables.semaphore;
            }
        }
    }
#endif
}

BlockChunk::~BlockChunk()
{
    for(int x = 0; x < subchunkCountX; x++)
    {
        for(int y = 0; y < subchunkCountY; y++)
        {
            for(int z = 0; z < subchunkCountZ; z++)
            {
                BlockChunkSubchunk &subchunk = subchunks[x][y][z];
                subchunk.entityList.clear();
            }
        }
    }
}

void WrappedEntity::verify() const
{
    assert(currentSubchunk != nullptr && currentChunk != nullptr);
    assert(subchunkListMembers.is_linked());
    assert(chunkListMembers.is_linked());
    assert(currentSubchunk >= &currentChunk->subchunks[0][0][0]
           && currentSubchunk
                  <= &currentChunk->subchunks[BlockChunk::subchunkCountX
                                              - 1][BlockChunk::subchunkCountY
                                                   - 1][BlockChunk::subchunkCountZ - 1]);
}
}
}
