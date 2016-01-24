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
#ifndef WOOD_DESCRIPTOR_H_INCLUDED
#define WOOD_DESCRIPTOR_H_INCLUDED

#include "texture/texture_descriptor.h"
#include "util/linked_map.h"
#include "util/enum_traits.h"
#include "block/block.h"
#include "entity/entity_struct.h"
#include "item/item_struct.h"
#include "util/position.h"
#include <iterator>
#include <string>
#include <cassert>
#include <vector>
#include <memory>
#include <cassert>
#include <stdexcept>
#include "generate/biome/biome.h"
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
enum class LogOrientation : std::uint8_t
{
    AllBark,
    X,
    Y,
    Z,
    DEFINE_ENUM_LIMITS(AllBark, Z)
};

class WoodDescriptor;
class TreeDescriptor;
class World;
struct WorldLockManager;
typedef const WoodDescriptor *WoodDescriptorPointer;
typedef const TreeDescriptor *TreeDescriptorPointer;

class Tree final
{
private:
    std::vector<PackedBlock> blocksArray;
    VectorI arrayOrigin, arraySize;
public:
    TreeDescriptorPointer descriptor;
    Tree(const Tree &) = default;
    Tree &operator =(const Tree &) = default;
    Tree(Tree &&) = default;
    Tree &operator =(Tree &&) = default;
    VectorI getArrayMin() const
    {
        return arrayOrigin;
    }
    VectorI getArraySize() const
    {
        return arraySize;
    }
    VectorI getArrayEnd() const
    {
        return arrayOrigin + arraySize;
    }
    VectorI getArrayMax() const
    {
        return getArrayEnd() - VectorI(1);
    }
    bool good() const
    {
        return descriptor != nullptr;
    }
    Tree()
        : blocksArray(), arrayOrigin(0), arraySize(0), descriptor(nullptr)
    {
    }
    Tree(TreeDescriptorPointer descriptor, VectorI arrayOrigin, VectorI arraySize)
        : blocksArray(arraySize.x * arraySize.y * arraySize.z), arrayOrigin(arrayOrigin), arraySize(arraySize), descriptor(descriptor)
    {
    }
private:
    std::size_t getArrayIndex(VectorI p) const
    {
        p -= arrayOrigin;
        assert(p.x < arraySize.x && p.x >= 0 && p.y < arraySize.y && p.y >= 0 && p.z < arraySize.z && p.z >= 0);
        return p.x * arraySize.y * arraySize.z + p.y * arraySize.z + p.z;
    }
public:
    void setBlock(VectorI position, Block b)
    {
        if(position.x < getArrayMin().x || position.x >= getArrayEnd().x ||
           position.y < getArrayMin().y || position.y >= getArrayEnd().y ||
           position.z < getArrayMin().z || position.z >= getArrayEnd().z)
            return;
        blocksArray[getArrayIndex(position)] = (PackedBlock)b;
    }
    Block getBlock(VectorI position) const
    {
        if(position.x < getArrayMin().x || position.x >= getArrayEnd().x ||
           position.y < getArrayMin().y || position.y >= getArrayEnd().y ||
           position.z < getArrayMin().z || position.z >= getArrayEnd().z)
            return Block();
        return (Block)blocksArray[getArrayIndex(position)];
    }
    bool placeInWorld(PositionI generatePosition, World &world, WorldLockManager &lock_manager) const;
};

class TreeDoesNotFitException final : public std::exception
{
public:
    virtual const char *what() const noexcept override
    {
        return "Tree doesn't fit";
    }
};

class TreeDescriptor
{
    TreeDescriptor(const TreeDescriptor &) = delete;
    const TreeDescriptor &operator =(const TreeDescriptor &) = delete;
public:
    virtual ~TreeDescriptor() = default;
    const int baseSize;
    const bool canGenerateFromSapling;
    const std::wstring name;
    const std::size_t generateChance;
protected:
    explicit TreeDescriptor(std::wstring name, int baseSize, bool canGenerateFromSapling, std::size_t generateChance)
        : baseSize(baseSize), canGenerateFromSapling(canGenerateFromSapling), name(name), generateChance(generateChance)
    {
    }
public:
    virtual Tree generateTree(std::uint32_t seed) const = 0;
    /** @brief select the block to use when a tree overlaps blocks in the world
     *
     * @param originalWorldBlock the original block in the world
     * @param treeBlock the block in the tree
     * @param canThrow if this can throw TreeDoesNotFitException
     * @return the selected block or an empty block for the original world block
     * @exception TreeDoesNotFitException throws an exception if the tree doesn't fit here (ex. a dirt block is right above a sapling)
     */
    virtual Block selectBlock(Block originalWorldBlock, Block treeBlock, bool canThrow) const;
    virtual float getChunkDecoratorCount(BiomeDescriptorPointer biome) const = 0;
};

class WoodDescriptor
{
    WoodDescriptor(const WoodDescriptor &) = delete;
    const WoodDescriptor &operator =(const WoodDescriptor &) = delete;
public:
    virtual ~WoodDescriptor() = default;
    const std::wstring name;
    const std::vector<TreeDescriptorPointer> trees;
private:
    TextureDescriptor logTop, logSide, planks, sapling, leaves, blockedLeaves;
    enum_array<BlockDescriptorPointer, LogOrientation> logBlockDescriptors;
    BlockDescriptorPointer planksBlockDescriptor;
    checked_array<BlockDescriptorPointer, 2> saplingBlockDescriptors;
    enum_array<BlockDescriptorPointer, bool> leavesBlockDescriptors; // index is if the block can decay
    ItemDescriptorPointer logItemDescriptor;
    ItemDescriptorPointer planksItemDescriptor;
    ItemDescriptorPointer saplingItemDescriptor;
    ItemDescriptorPointer leavesItemDescriptor;
protected:
    WoodDescriptor(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves, TextureDescriptor blockedLeaves, std::vector<TreeDescriptorPointer> trees);
    void setDescriptors(enum_array<BlockDescriptorPointer, LogOrientation> logBlockDescriptors,
                        BlockDescriptorPointer planksBlockDescriptor,
                        checked_array<BlockDescriptorPointer, 2> saplingBlockDescriptors,
                        enum_array<BlockDescriptorPointer, bool> leavesBlockDescriptors, // index is if the block can decay
                        ItemDescriptorPointer logItemDescriptor,
                        ItemDescriptorPointer planksItemDescriptor,
                        ItemDescriptorPointer saplingItemDescriptor,
                        ItemDescriptorPointer leavesItemDescriptor)
    {
        this->logBlockDescriptors = logBlockDescriptors;
        this->planksBlockDescriptor = planksBlockDescriptor;
        this->saplingBlockDescriptors = saplingBlockDescriptors;
        this->leavesBlockDescriptors = leavesBlockDescriptors;
        this->logItemDescriptor = logItemDescriptor;
        this->planksItemDescriptor = planksItemDescriptor;
        this->saplingItemDescriptor = saplingItemDescriptor;
        this->leavesItemDescriptor = leavesItemDescriptor;
    }
public:
    TextureDescriptor getLogTopTexture() const
    {
        return logTop;
    }
    TextureDescriptor getLogSideTexture() const
    {
        return logSide;
    }
    TextureDescriptor getPlanksTexture() const
    {
        return planks;
    }
    TextureDescriptor getSaplingTexture() const
    {
        return sapling;
    }
    TextureDescriptor getLeavesTexture() const
    {
        return leaves;
    }
    TextureDescriptor getBlockedLeavesTexture() const
    {
        return blockedLeaves;
    }
    BlockDescriptorPointer getLogBlockDescriptor(LogOrientation logOrientation) const
    {
        return logBlockDescriptors[logOrientation];
    }
    BlockDescriptorPointer getPlanksBlockDescriptor() const
    {
        return planksBlockDescriptor;
    }
    BlockDescriptorPointer getSaplingBlockDescriptor(unsigned growthStage) const
    {
        return saplingBlockDescriptors[growthStage];
    }
    BlockDescriptorPointer getLeavesBlockDescriptor(bool canDecay) const
    {
        return leavesBlockDescriptors[canDecay];
    }
    ItemDescriptorPointer getLogItemDescriptor() const
    {
        return logItemDescriptor;
    }
    ItemDescriptorPointer getPlanksItemDescriptor() const
    {
        return planksItemDescriptor;
    }
    ItemDescriptorPointer getSaplingItemDescriptor() const
    {
        return saplingItemDescriptor;
    }
    ItemDescriptorPointer getLeavesItemDescriptor() const
    {
        return leavesItemDescriptor;
    }
    virtual void makeLeavesDrops(World &world, BlockIterator bi, WorldLockManager &lock_manager, Item tool) const;
};

class WoodDescriptors_t final
{
    friend class WoodDescriptor;
private:
    static linked_map<std::wstring, WoodDescriptorPointer> *pNameMap;
    static void add(WoodDescriptor *descriptor)
    {
        if(pNameMap == nullptr)
            pNameMap = new linked_map<std::wstring, WoodDescriptorPointer>();
        assert(pNameMap->count(descriptor->name) == 0);
        pNameMap->insert(std::pair<std::wstring, WoodDescriptorPointer>(descriptor->name, descriptor));
    }
public:
    std::size_t size() const
    {
        if(pNameMap == nullptr)
            return 0;
        return pNameMap->size();
    }
GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
    class iterator final : public std::iterator<std::forward_iterator_tag, const WoodDescriptorPointer>
    {
GCC_PRAGMA(diagnostic pop)
    private:
        linked_map<std::wstring, WoodDescriptorPointer>::const_iterator iter;
    public:
        explicit iterator(linked_map<std::wstring, WoodDescriptorPointer>::const_iterator iter)
            : iter(iter)
        {
        }
        iterator()
            : iter()
        {
        }
        const WoodDescriptorPointer &operator *() const
        {
            return std::get<1>(*iter);
        }
        const iterator &operator ++()
        {
            ++iter;
            return *this;
        }
        iterator operator ++(int)
        {
            return iterator(iter++);
        }
        bool operator ==(const iterator &rt) const
        {
            return iter == rt.iter;
        }
        bool operator !=(const iterator &rt) const
        {
            return iter != rt.iter;
        }
    };
    iterator begin() const
    {
        assert(pNameMap != nullptr);
        return iterator(pNameMap->cbegin());
    }
    iterator end() const
    {
        assert(pNameMap != nullptr);
        return iterator(pNameMap->cend());
    }
    iterator find(WoodDescriptorPointer descriptor) const
    {
        if(descriptor == nullptr)
            return end();
        return find(descriptor->name);
    }
    iterator find(std::wstring name) const
    {
        assert(pNameMap != nullptr);
        return iterator(pNameMap->find(name));
    }
};

static constexpr WoodDescriptors_t WoodDescriptors{};

inline WoodDescriptor::WoodDescriptor(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves, TextureDescriptor blockedLeaves, std::vector<TreeDescriptorPointer> trees)
    : name(name),
    trees(std::move(trees)),
    logTop(logTop),
    logSide(logSide),
    planks(planks),
    sapling(sapling),
    leaves(leaves),
    blockedLeaves(blockedLeaves),
    logBlockDescriptors(),
    planksBlockDescriptor(),
    saplingBlockDescriptors(),
    leavesBlockDescriptors(),
    logItemDescriptor(),
    planksItemDescriptor(),
    saplingItemDescriptor(),
    leavesItemDescriptor()
{
    WoodDescriptors_t::add(this);
}

namespace Woods
{
namespace builtin
{
class SimpleWood : public WoodDescriptor
{
public:
    SimpleWood(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves, TextureDescriptor blockedLeaves, std::vector<TreeDescriptorPointer> trees);
};
}
}
}
}

#endif // WOOD_DESCRIPTOR_H_INCLUDED
