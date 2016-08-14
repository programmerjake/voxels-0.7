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
#ifndef PREGENERATED_INSTANCE_H_INCLUDED
#define PREGENERATED_INSTANCE_H_INCLUDED

#include "generate/decorator.h"
#include "block/block.h"
#include <vector>
#include <cassert>
#include <algorithm>

namespace programmerjake
{
namespace voxels
{
class PregeneratedDecoratorInstance : public DecoratorInstance
{
private:
    std::vector<PackedBlock> blocks;
    const VectorI baseOffset, theSize;

public:
    PregeneratedDecoratorInstance(PositionI position,
                                  DecoratorDescriptorPointer descriptor,
                                  VectorI baseOffset,
                                  VectorI theSize)
        : DecoratorInstance(position, descriptor),
          blocks(),
          baseOffset(baseOffset),
          theSize(theSize)
    {
        blocks.resize(theSize.x * theSize.y * theSize.z);
    }
    void setBlock(VectorI rpos, Block b)
    {
        rpos -= baseOffset;
        assert(rpos.x < theSize.x && rpos.y < theSize.y && rpos.z < theSize.z);
        blocks[rpos.x * theSize.z * theSize.y + rpos.y * theSize.z + rpos.z] = (PackedBlock)b;
    }
    Block getBlock(VectorI rpos) const
    {
        rpos -= baseOffset;
        if(rpos.x < 0 || rpos.x >= theSize.x || rpos.y < 0 || rpos.y >= theSize.y || rpos.z < 0
           || rpos.z >= theSize.z)
            return Block();
        return (Block)blocks[rpos.x * theSize.z * theSize.y + rpos.y * theSize.z + rpos.z];
    }
    VectorI maxRelativePosition() const
    {
        return baseOffset + theSize - VectorI(1);
    }
    VectorI minRelativePosition() const
    {
        return baseOffset;
    }
    VectorI size() const
    {
        return theSize;
    }
    virtual bool canReplace(Block oldBlock, Block newBlock) const
    {
        return newBlock.good();
    }
    virtual void generateInChunk(PositionI chunkBasePosition,
                                 WorldLockManager &lock_manager,
                                 World &world,
                                 BlocksGenerateArray &blocks) const override
    {
        assert(chunkBasePosition.d == position.d);
        VectorI minPos = minRelativePosition() + position;
        VectorI maxPos = maxRelativePosition() + position;
        minPos.x = std::max(minPos.x, chunkBasePosition.x);
        minPos.y = std::max(minPos.y, chunkBasePosition.y);
        minPos.z = std::max(minPos.z, chunkBasePosition.z);
        maxPos.x = std::min(maxPos.x, chunkBasePosition.x + BlockChunk::chunkSizeX - 1);
        maxPos.y = std::min(maxPos.y, chunkBasePosition.y + BlockChunk::chunkSizeY - 1);
        maxPos.z = std::min(maxPos.z, chunkBasePosition.z + BlockChunk::chunkSizeZ - 1);
        for(VectorI pos = minPos; pos.x <= maxPos.x; pos.x++)
        {
            for(pos.y = minPos.y; pos.y <= maxPos.y; pos.y++)
            {
                for(pos.z = minPos.z; pos.z <= maxPos.z; pos.z++)
                {
                    VectorI crpos = pos - chunkBasePosition;
                    VectorI rpos = pos - position;
                    Block &b = blocks[crpos.x][crpos.y][crpos.z];
                    Block newB = getBlock(rpos);
                    if(canReplace(b, newB))
                        b = newB;
                }
            }
        }
    }
};
}
}

#endif // PREGENERATED_INSTANCE_H_INCLUDED
