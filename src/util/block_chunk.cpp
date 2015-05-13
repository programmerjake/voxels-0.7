/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
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
#include "util/wrapped_entity.h"
#include "util/logging.h"
#include "platform/platform.h"

namespace programmerjake
{
namespace voxels
{
BlockChunkChunkVariables::~BlockChunkChunkVariables()
{
    while(blockUpdateListHead != nullptr)
    {
        BlockUpdate *deleteMe = blockUpdateListHead;
        blockUpdateListHead = blockUpdateListHead->chunk_next;
        BlockUpdate::free(deleteMe);
    }
}

BlockChunk::BlockChunk(PositionI basePosition)
    : BasicBlockChunk(basePosition),
    objectCounter()
{
    dumpStackTraceToDebugLog();
}

BlockChunk::~BlockChunk()
{
    for(auto i = chunkVariables.entityList.begin(); i != chunkVariables.entityList.end(); i = chunkVariables.entityList.erase(i))
    {
        WrappedEntity &we = *i;
        if(we.currentSubchunk != nullptr)
        {
            auto iter = we.currentSubchunk->entityList.to_iterator(&we);
            we.currentSubchunk->entityList.erase(iter);
        }
    }
}
}
}
