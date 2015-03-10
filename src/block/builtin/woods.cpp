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
#include "block/builtin/woods.h"
#include "block/builtin/dirt.h"
#include "block/builtin/stone.h"
#include "block/builtin/cobblestone.h"
#include "block/builtin/air.h"
#include "block/builtin/wood.h"
#include "generate/decorator/tree.h"
#include <random>

namespace programmerjake
{
namespace voxels
{
namespace Woods
{
namespace builtin
{
namespace
{
class SmallOakTree : public TreeDescriptor
{
public:
    DecoratorDescriptorPointer decorator;
    SmallOakTree()
        : TreeDescriptor(L"builtin.small_oak", 1, true)
    {
        decorator = new Decorators::builtin::TreeDecorator(this);
    }
    virtual Tree generateTree(std::uint32_t seed) const override
    {
        enum_array<Block, LogOrientation> logs;
        for(LogOrientation i : enum_traits<LogOrientation>())
        {
            logs[i] = Block(Oak::descriptor()->getLogBlockDescriptor(i));
        }
        Block leaves = Block(Oak::descriptor()->getLeavesBlockDescriptor(true));
        Block emptyClearance = Block(Blocks::builtin::Dirt::descriptor()); // used as placeholder for clearance block that is empty
        Block leavesClearance = Block(Blocks::builtin::Stone::descriptor()); // used as placeholder for clearance block that is leaves
        const int xzSize = 5;
        const int ySize = 10;
        Tree retval(this, VectorI(-xzSize, 0, -xzSize), VectorI(xzSize * 2 + 1, ySize, xzSize * 2 + 1));
        std::minstd_rand rg(seed);
        int trunkHeight = std::uniform_int_distribution<int>(4, 6)(rg);
        for(int i = 0; i < trunkHeight; i++)
            retval.setBlock(VectorI(0, i, 0), logs[LogOrientation::Y]);
        int leavesRadius = 3;
        VectorI leavesCenter(0, trunkHeight - 1, 0);
        for(int x = -leavesRadius; x <= leavesRadius; x++)
        {
            for(int y = trunkHeight - leavesRadius / 2; y < trunkHeight + leavesRadius; y++)
            {
                for(int z = -leavesRadius; z <= leavesRadius; z++)
                {
                    VectorI pos(x, y, z);
                    int adjustedRadius = leavesRadius - std::uniform_int_distribution<int>(0, 1)(rg);
                    if(absSquared(pos - leavesCenter) < adjustedRadius * adjustedRadius)
                        retval.setBlock(pos, leaves);
                }
            }
        }
        for(int y = 1; y < trunkHeight + 2; y++)
        {
            int size = 1;
            if(y >= trunkHeight)
                size = 2;
            for(int x = -size; x <= size; x++)
            {
                for(int z = -size; z <= size; z++)
                {
                    VectorI pos(x, y, z);
                    Block b = retval.getBlock(pos);
                    if(!b.good())
                        retval.setBlock(pos, emptyClearance);
                    else if(b.descriptor == leaves.descriptor)
                        retval.setBlock(pos, leavesClearance);
                }
            }
        }
        return std::move(retval);
    }
    /** @brief select the block to use when a tree overlaps blocks in the world
     *
     * @param originalWorldBlock the original block in the world
     * @param treeBlock the block in the tree
     * @param canThrow if this can throw TreeDoesNotFitException
     * @return the selected block or an empty block for the original world block
     * @exception TreeDoesNotFitException throws an exception if the tree doesn't fit here (ex. a dirt block is right above a sapling)
     */
    virtual Block selectBlock(Block originalWorldBlock, Block treeBlock, bool canThrow) const override
    {
        if(!originalWorldBlock.good())
        {
            if(!canThrow)
                return Block();
            throw TreeDoesNotFitException();
        }
        if(!treeBlock.good())
            return Block();
        if(dynamic_cast<const Blocks::builtin::DirtBlock *>(treeBlock.descriptor) != 0)
        {
            if(!canThrow)
                return Block();
            if(dynamic_cast<const Blocks::builtin::WoodLeaves *>(originalWorldBlock.descriptor) != 0)
                return Block();
            if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor())
                throw TreeDoesNotFitException();
            return Block();
        }
        if(dynamic_cast<const Blocks::builtin::Stone *>(treeBlock.descriptor) != 0)
        {
            if(dynamic_cast<const Blocks::builtin::WoodLeaves *>(originalWorldBlock.descriptor) != 0)
                return Block(Oak::descriptor()->getLeavesBlockDescriptor(true));
            if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor() && canThrow)
                throw TreeDoesNotFitException();
            return Block(Oak::descriptor()->getLeavesBlockDescriptor(true));
        }
        if(dynamic_cast<const Blocks::builtin::WoodLog *>(treeBlock.descriptor) != 0)
        {
            if(!canThrow)
                return treeBlock;
            if(dynamic_cast<const Blocks::builtin::WoodLeaves *>(originalWorldBlock.descriptor) != 0)
                return treeBlock;
            if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor())
                throw TreeDoesNotFitException();
            return treeBlock;
        }
        if(dynamic_cast<const Blocks::builtin::WoodLog *>(originalWorldBlock.descriptor) != 0)
            return Block();
        if(dynamic_cast<const Blocks::builtin::WoodLeaves *>(originalWorldBlock.descriptor) != 0)
        {
            if(dynamic_cast<const Blocks::builtin::WoodLog *>(treeBlock.descriptor) != 0)
                return treeBlock;
            return Block();
        }
        if(dynamic_cast<const Blocks::builtin::DirtBlock *>(originalWorldBlock.descriptor) != 0)
        {
            return Block();
        }
        if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor() && canThrow)
            throw TreeDoesNotFitException();
        return treeBlock;
    }
    virtual float getChunkDecoratorCount(BiomeDescriptorPointer biome) const override
    {
        return 5;
    }
};
}
std::vector<TreeDescriptorPointer> Oak::makeTreeDescriptors()
{
    return std::vector<TreeDescriptorPointer>{new SmallOakTree()};
}
}
}
}
}
