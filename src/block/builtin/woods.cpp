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
    SmallOakTree(const SmallOakTree &) = delete;
    SmallOakTree &operator =(const SmallOakTree &) = delete;
public:
    DecoratorDescriptorPointer decorator;
    SmallOakTree()
        : TreeDescriptor(L"builtin.small_oak", 1, true, 9),
        decorator()
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
        Block saplingReplace = Block(Blocks::builtin::Cobblestone::descriptor()); // used as placeholder for log block that can break saplings
        const int xzSize = 5;
        const int ySize = 10;
        Tree retval(this, VectorI(-xzSize, 0, -xzSize), VectorI(xzSize * 2 + 1, ySize, xzSize * 2 + 1));
        std::minstd_rand rg(seed);
        rg.discard(10);
        int trunkHeight = std::uniform_int_distribution<int>(4, 6)(rg);
        int leavesRadius = 3;
        VectorI leavesCenter(0, trunkHeight - 1, 0);
        for(int x = -leavesRadius; x <= leavesRadius; x++)
        {
            for(int y = trunkHeight - leavesRadius / 2; y < trunkHeight + leavesRadius; y++)
            {
                for(int z = -leavesRadius; z <= leavesRadius; z++)
                {
                    VectorI pos(x, y, z);
                    float adjustedRadius = leavesRadius - std::uniform_real_distribution<float>(0, 0.5f)(rg);
                    if(absSquared(pos - leavesCenter) < adjustedRadius * adjustedRadius)
                        retval.setBlock(pos, leaves);
                }
            }
        }
        for(int i = 0; i < trunkHeight; i++)
            retval.setBlock(VectorI(0, i, 0), logs[LogOrientation::Y]);
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
        retval.setBlock(VectorI(0, 0, 0), saplingReplace);
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
        if(dynamic_cast<const Blocks::builtin::Cobblestone *>(treeBlock.descriptor) != 0)
        {
            if(!canThrow)
                return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
            const Blocks::builtin::Sapling *saplingDescriptor = dynamic_cast<const Blocks::builtin::Sapling *>(originalWorldBlock.descriptor);
            if(saplingDescriptor != nullptr)
            {
                if(saplingDescriptor->getWoodDescriptor() == Oak::pointer())
                    return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
                throw TreeDoesNotFitException();
            }
            if(dynamic_cast<const Blocks::builtin::WoodLeaves *>(originalWorldBlock.descriptor) != 0)
                return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
            if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor())
                throw TreeDoesNotFitException();
            return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
        }
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
            if(dynamic_cast<const Blocks::builtin::WoodLog *>(originalWorldBlock.descriptor) != 0)
                return Block();
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
        return 9 / 2.0f;
    }
};
class LargeOakTree : public TreeDescriptor
{
    LargeOakTree(const LargeOakTree &) = delete;
    LargeOakTree &operator =(const LargeOakTree &) = delete;
public:
    DecoratorDescriptorPointer decorator;
    LargeOakTree()
        : TreeDescriptor(L"builtin.large_oak", 1, true, 1),
        decorator()
    {
        decorator = new Decorators::builtin::TreeDecorator(this);
    }
private:
    static void generateLeafGroup(VectorI position, int radius, Tree &tree)
    {
        Block leaves = Block(Oak::descriptor()->getLeavesBlockDescriptor(true));
        for(int dx = -radius; dx <= radius; dx++)
        {
            for(int dy = -radius; dy <= radius; dy++)
            {
                for(int dz = -radius; dz <= radius; dz++)
                {
                    if(dx * dx + dy * dy + dz * dz < (radius + 1) * (radius + 1))
                    {
                        VectorI p = position + VectorI(dx, dy, dz);
                        Block b = tree.getBlock(p);
                        if(!b.good())
                            tree.setBlock(p, leaves);
                    }
                }
            }
        }
    }
    static void generateBranch(VectorI position, Tree &tree, std::minstd_rand &rg)
    {
        enum_array<Block, LogOrientation> logs;
        for(LogOrientation i : enum_traits<LogOrientation>())
        {
            logs[i] = Block(Oak::descriptor()->getLogBlockDescriptor(i));
        }
        VectorI delta = VectorI(0, 0, 0);
        for(int i = 0; i < 3; i++)
        {
            Block b = tree.getBlock(position);
            for(const Block &log : logs)
                if(b.descriptor == log.descriptor)
                    return;
            tree.setBlock(position, logs[LogOrientation::Y]);
            generateLeafGroup(position, 2, tree);
            if(position.y >= tree.getArrayMax().y)
                return;
            if(std::uniform_int_distribution<int>(0, 4)(rg) < 1)
                return;
            do
            {
                delta += VectorI(std::uniform_int_distribution<int>(-1, 1)(rg),
                                 0,
                                 std::uniform_int_distribution<int>(-1, 1)(rg));
            }
            while(delta == VectorI(0));
            VectorI moveDirection = delta;
            delta.y = 1;
            if(std::abs(moveDirection.x) > std::abs(moveDirection.y) &&
               std::abs(moveDirection.x) > std::abs(moveDirection.z))
            {
                moveDirection = VectorI(moveDirection.x < 0 ? -1 : 1, 0, 0);
            }
            else if(std::abs(moveDirection.y) > std::abs(moveDirection.z))
            {
                moveDirection = VectorI(0, moveDirection.y < 0 ? -1 : 1, 0);
            }
            else
            {
                moveDirection = VectorI(0, 0, moveDirection.z < 0 ? -1 : 1);
            }
            position += moveDirection;
        }
    }
    static bool isLogBlock(VectorI position, Tree &tree, const enum_array<Block, LogOrientation> &logs)
    {
        Block b = tree.getBlock(position);
        for(const Block &log : logs)
        {
            if(log.descriptor == b.descriptor)
                return true;
        }
        return false;
    }
    static LogOrientation getLogOrientation(VectorI position, Tree &tree, const enum_array<Block, LogOrientation> &logs)
    {
        if(isLogBlock(position - VectorI(0, 1, 0), tree, logs) ||
           isLogBlock(position + VectorI(0, 1, 0), tree, logs))
            return LogOrientation::Y;
        if(isLogBlock(position - VectorI(1, 0, 0), tree, logs) ||
           isLogBlock(position + VectorI(1, 0, 0), tree, logs))
            return LogOrientation::X;
        if(isLogBlock(position - VectorI(0, 0, 1), tree, logs) ||
           isLogBlock(position + VectorI(0, 0, 1), tree, logs))
            return LogOrientation::Z;
        return LogOrientation::AllBark;
    }
    static void fixLogOrientations(Tree &tree)
    {
        enum_array<Block, LogOrientation> logs;
        for(LogOrientation i : enum_traits<LogOrientation>())
        {
            logs[i] = Block(Oak::descriptor()->getLogBlockDescriptor(i));
        }
        VectorI p = tree.getArrayMin();
        for(p.x = tree.getArrayMin().x; p.x < tree.getArrayEnd().x; p.x++)
        {
            for(p.y = tree.getArrayMin().y; p.y < tree.getArrayEnd().y; p.y++)
            {
                for(p.z = tree.getArrayMin().z; p.z < tree.getArrayEnd().z; p.z++)
                {
                    if(isLogBlock(p, tree, logs))
                    {
                        tree.setBlock(p, logs[getLogOrientation(p, tree, logs)]);
                    }
                }
            }
        }
    }
public:
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
        Block saplingReplace = Block(Blocks::builtin::Cobblestone::descriptor()); // used as placeholder for log block that can break saplings
        const int xzSize = 7;
        const int ySize = 17;
        Tree retval(this, VectorI(-xzSize, 0, -xzSize), VectorI(xzSize * 2 + 1, ySize, xzSize * 2 + 1));
        std::minstd_rand rg(seed);
        rg.discard(10);
        int trunkHeight = std::uniform_int_distribution<int>(5, 15)(rg);
        for(int y = 0; y < trunkHeight; y++)
        {
            if((y >= (trunkHeight + 3) / 3 && y >= 5 && std::uniform_int_distribution<int>(0, 2)(rg) == 0) || y == trunkHeight - 1)
            {
                generateBranch(VectorI(0, y, 0), retval, rg);
            }
            retval.setBlock(VectorI(0, y, 0), logs[LogOrientation::Y]);
        }
        fixLogOrientations(retval);
        for(int y = 1; y < trunkHeight + 2; y++)
        {
            int size = 0;
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
        retval.setBlock(VectorI(0, 0, 0), saplingReplace);
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
        if(dynamic_cast<const Blocks::builtin::Cobblestone *>(treeBlock.descriptor) != 0)
        {
            if(!canThrow)
                return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
            const Blocks::builtin::Sapling *saplingDescriptor = dynamic_cast<const Blocks::builtin::Sapling *>(originalWorldBlock.descriptor);
            if(saplingDescriptor != nullptr)
            {
                if(saplingDescriptor->getWoodDescriptor() == Oak::pointer())
                    return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
                throw TreeDoesNotFitException();
            }
            if(dynamic_cast<const Blocks::builtin::WoodLeaves *>(originalWorldBlock.descriptor) != 0)
                return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
            if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor())
                throw TreeDoesNotFitException();
            return Block(Oak::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
        }
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
            if(dynamic_cast<const Blocks::builtin::WoodLog *>(originalWorldBlock.descriptor) != 0)
                return Block();
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
        return 1 / 2.0f;
    }
};
class BirchTree : public TreeDescriptor
{
    BirchTree(const BirchTree &) = delete;
    BirchTree &operator =(const BirchTree &) = delete;
public:
    DecoratorDescriptorPointer decorator;
    const bool isTallVariant;
    BirchTree(bool isTallVariant)
        : TreeDescriptor(isTallVariant ? L"builtin.tall_birch" : L"builtin.birch", 1, !isTallVariant, 9),
        decorator(),
        isTallVariant(isTallVariant)
    {
        decorator = new Decorators::builtin::TreeDecorator(this);
    }
    virtual Tree generateTree(std::uint32_t seed) const override
    {
        enum_array<Block, LogOrientation> logs;
        for(LogOrientation i : enum_traits<LogOrientation>())
        {
            logs[i] = Block(Birch::descriptor()->getLogBlockDescriptor(i));
        }
        Block leaves = Block(Birch::descriptor()->getLeavesBlockDescriptor(true));
        Block emptyClearance = Block(Blocks::builtin::Dirt::descriptor()); // used as placeholder for clearance block that is empty
        Block leavesClearance = Block(Blocks::builtin::Stone::descriptor()); // used as placeholder for clearance block that is leaves
        Block saplingReplace = Block(Blocks::builtin::Cobblestone::descriptor()); // used as placeholder for log block that can break saplings
        const int tallVariantExtraHeight = 2;
        const int xzSize = 2;
        const int ySize = isTallVariant ? 8 + tallVariantExtraHeight : 8;
        Tree retval(this, VectorI(-xzSize, 0, -xzSize), VectorI(xzSize * 2 + 1, ySize, xzSize * 2 + 1));
        std::minstd_rand rg(seed);
        rg.discard(10);
        int trunkHeight = std::uniform_int_distribution<int>(5, 7)(rg);
        if(isTallVariant)
            trunkHeight += tallVariantExtraHeight;
        for(int x = -1; x <= 1; x++)
        {
            retval.setBlock(VectorI(x, trunkHeight, 0), leaves);
        }
        for(int z = -1; z <= 1; z++)
        {
            retval.setBlock(VectorI(0, trunkHeight, z), leaves);
        }
        for(int x = -1; x <= 1; x++)
        {
            retval.setBlock(VectorI(x, trunkHeight - 1, 0), leaves);
        }
        for(int z = -1; z <= 1; z++)
        {
            retval.setBlock(VectorI(0, trunkHeight - 1, z), leaves);
        }
        if(std::uniform_int_distribution<int>(0, 1)(rg) == 0)
        {
            retval.setBlock(VectorI(-1, trunkHeight - 1, -1), leaves);
        }
        if(std::uniform_int_distribution<int>(0, 1)(rg) == 0)
        {
            retval.setBlock(VectorI(1, trunkHeight - 1, -1), leaves);
        }
        if(std::uniform_int_distribution<int>(0, 1)(rg) == 0)
        {
            retval.setBlock(VectorI(-1, trunkHeight - 1, 1), leaves);
        }
        if(std::uniform_int_distribution<int>(0, 1)(rg) == 0)
        {
            retval.setBlock(VectorI(1, trunkHeight - 1, 1), leaves);
        }
        for(int y = trunkHeight - 3; y <= trunkHeight - 2; y++)
        {
            int size = 2;
            for(int x = -size; x <= size; x++)
            {
                for(int z = -size; z <= size; z++)
                {
                    if(std::abs(x) == size && std::abs(z) == size)
                    {
                        if(std::uniform_int_distribution<int>(0, 1)(rg) == 0)
                            continue;
                    }
                    retval.setBlock(VectorI(x, y, z), leaves);
                }
            }
        }
        for(int i = 0; i < trunkHeight; i++)
            retval.setBlock(VectorI(0, i, 0), logs[LogOrientation::Y]);
        for(int y = 1; y < trunkHeight + 2; y++)
        {
            int size = 2;
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
        retval.setBlock(VectorI(0, 0, 0), saplingReplace);
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
        if(dynamic_cast<const Blocks::builtin::Cobblestone *>(treeBlock.descriptor) != 0)
        {
            if(!canThrow)
                return Block(Birch::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
            const Blocks::builtin::Sapling *saplingDescriptor = dynamic_cast<const Blocks::builtin::Sapling *>(originalWorldBlock.descriptor);
            if(saplingDescriptor != nullptr)
            {
                if(saplingDescriptor->getWoodDescriptor() == Birch::pointer())
                    return Block(Birch::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
                throw TreeDoesNotFitException();
            }
            if(dynamic_cast<const Blocks::builtin::WoodLeaves *>(originalWorldBlock.descriptor) != 0)
                return Block(Birch::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
            if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor())
                throw TreeDoesNotFitException();
            return Block(Birch::descriptor()->getLogBlockDescriptor(LogOrientation::Y));
        }
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
                return Block(Birch::descriptor()->getLeavesBlockDescriptor(true));
            if(originalWorldBlock.descriptor != Blocks::builtin::Air::descriptor() && canThrow)
                throw TreeDoesNotFitException();
            if(dynamic_cast<const Blocks::builtin::WoodLog *>(originalWorldBlock.descriptor) != 0)
                return Block();
            return Block(Birch::descriptor()->getLeavesBlockDescriptor(true));
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
        if(isTallVariant)
            return 0;
        return 10 / 2.0f;
    }
};
}
std::vector<TreeDescriptorPointer> Oak::makeTreeDescriptors()
{
    return std::vector<TreeDescriptorPointer>{new SmallOakTree(), new LargeOakTree()};
}
std::vector<TreeDescriptorPointer> Birch::makeTreeDescriptors()
{
    return std::vector<TreeDescriptorPointer>{new BirchTree(false), new BirchTree(true)};
}
std::vector<TreeDescriptorPointer> Spruce::makeTreeDescriptors()
{
    return std::vector<TreeDescriptorPointer>{};
}
std::vector<TreeDescriptorPointer> Jungle::makeTreeDescriptors()
{
    return std::vector<TreeDescriptorPointer>{};
}
}
}
}
}
