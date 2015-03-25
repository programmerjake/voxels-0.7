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
#ifndef TREE_DECORATOR_H_INCLUDED
#define TREE_DECORATOR_H_INCLUDED

#include "generate/decorator.h"
#include "util/wood_descriptor.h"
#include "generate/biome/biome_descriptor.h"
#include "block/builtin/dirt_block.h"
#include "block/builtin/dirt.h"
#include "block/builtin/air.h"
#include <cassert>

namespace programmerjake
{
namespace voxels
{
namespace Decorators
{
namespace builtin
{
class TreeDecorator : public DecoratorDescriptor
{
public:
    const TreeDescriptorPointer treeDescriptor;
    TreeDecorator(TreeDescriptorPointer treeDescriptor)
        : DecoratorDescriptor(L"builtin.tree(treeDescriptor=" + treeDescriptor->name + L")", 1, 1000), treeDescriptor(treeDescriptor)
    {
    }
protected:
    class Instance : public DecoratorInstance
    {
        Tree tree;
    public:
        Instance(PositionI position, DecoratorDescriptorPointer descriptor, Tree tree)
            : DecoratorInstance(position, descriptor), tree(std::move(tree))
        {
        }
        virtual void generateInChunk(PositionI chunkBasePosition, WorldLockManager &lock_manager, World &world,
                                     checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks) const override
        {
            assert(chunkBasePosition.d == position.d);
            VectorI minPos = tree.getArrayMin() + position;
            VectorI maxPos = tree.getArrayMax() + position;
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
                        Block newB = tree.getBlock(rpos);
                        newB = tree.descriptor->selectBlock(b, newB, false);
                        if(newB.good())
                            b = newB;
                    }
                }
            }
            PositionI setToDirtPosition = position - VectorI(0, 1, 0);
            BlockDescriptorPointer dirtDescriptor = Blocks::builtin::Dirt::descriptor();
            if(BlockChunk::getChunkBasePosition(setToDirtPosition) == chunkBasePosition)
            {
                VectorI crpos = setToDirtPosition - chunkBasePosition;
                Block &b = blocks[crpos.x][crpos.y][crpos.z];
                if(b.descriptor != dirtDescriptor && dynamic_cast<const Blocks::builtin::DirtBlock *>(b.descriptor) != nullptr)
                    b = Block(dirtDescriptor);
            }
        }
    };
public:
    /** @brief create a DecoratorInstance for this decorator in a chunk
     *
     * @param chunkBasePosition the base position of the chunk to generate in
     * @param columnBasePosition the base position of the column to generate in
     * @param surfacePosition the surface position of the column to generate in
     * @param lock_manager the WorldLockManager
     * @param chunkBaseIterator a BlockIterator to chunkBasePosition
     * @param blocks the blocks for this chunk
     * @param randomSource the RandomSource
     * @param generateNumber a number that is different for each decorator in a chunk (use for picking a different position each time)
     * @return the new DecoratorInstance or nullptr
     *
     */
    virtual std::shared_ptr<const DecoratorInstance> createInstance(PositionI chunkBasePosition, PositionI columnBasePosition, PositionI surfacePosition,
                                 WorldLockManager &lock_manager, BlockIterator chunkBaseIterator,
                                 const checked_array<checked_array<checked_array<Block, BlockChunk::chunkSizeZ>, BlockChunk::chunkSizeY>, BlockChunk::chunkSizeX> &blocks,
                                 RandomSource &randomSource, std::uint32_t generateNumber) const override
    {
        VectorI surfaceRPos = surfacePosition - chunkBasePosition;
        Block groundBlock = blocks[surfaceRPos.x][surfaceRPos.y][surfaceRPos.z];
        Block aboveGroundBlock = blocks[surfaceRPos.x][surfaceRPos.y + 1][surfaceRPos.z];
        if(dynamic_cast<const Blocks::builtin::DirtBlock *>(groundBlock.descriptor) == nullptr)
            return nullptr;
        if(aboveGroundBlock.descriptor != Blocks::builtin::Air::descriptor())
            return nullptr;
        return std::make_shared<Instance>(surfacePosition + VectorI(0, 1, 0), this, treeDescriptor->generateTree(generateNumber));
    }
    virtual float getChunkDecoratorCount(BiomeDescriptorPointer biome) const
    {
        return treeDescriptor->getChunkDecoratorCount(biome);
    }
};
}
}
}
}

#endif // TREE_DECORATOR_H_INCLUDED
