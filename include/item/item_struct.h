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
#ifndef ITEM_STRUCT_H_INCLUDED
#define ITEM_STRUCT_H_INCLUDED

#include <memory>
#include <cstdint>
#include <utility>
#include <cassert>
#include "render/mesh.h"
#include "util/checked_array.h"

namespace programmerjake
{
namespace voxels
{
class ItemDescriptor;

/// @brief item descriptor pointer
typedef const ItemDescriptor *ItemDescriptorPointer;

/// @brief an item
struct Item final
{
    /// @brief the ItemDescriptor
    ItemDescriptorPointer descriptor;
    /// @brief the associated data
    std::shared_ptr<void> data;
    /** @brief check if this Item is non-empty
     * @return true if this Item is non-empty
     */
    bool good() const
    {
        return descriptor != nullptr;
    }
    /// @brief construct an empty item
    Item()
        : descriptor(nullptr), data()
    {
    }
    /** @brief construct an item
     *
     * @param descriptor the ItemDescriptor
     * @param data the associated data
     *
     */
    explicit Item(ItemDescriptorPointer descriptor, std::shared_ptr<void> data = nullptr)
        : descriptor(descriptor), data(descriptor == nullptr ? std::shared_ptr<void>(nullptr) : data)
    {
    }
private:
    bool dataEqual(const Item &rt) const;
public:
    /** @brief check for item equality
     * @param rt the other Item
     * @return true if this item equals rt
     * @see operator !=
     * @see ItemDescriptorPointer::dataEqual
     */
    bool operator ==(const Item &rt) const
    {
        if(descriptor == nullptr)
            return rt.descriptor == nullptr;
        return descriptor == rt.descriptor && dataEqual(rt);
    }
    /** @brief check for item inequality
     * @param rt the other Item
     * @return true if this item doesn't equal rt
     * @see operator ==
     * @see ItemDescriptorPointer::dataEqual
     */
    bool operator !=(const Item &rt) const
    {
        if(descriptor == nullptr)
            return rt.descriptor != nullptr;
        return descriptor != rt.descriptor || !dataEqual(rt);
    }
};

struct ItemStack final
{
    /// @brief the Item
    Item item;
    /// @brief the Item count
    unsigned count = 0;
    /// @brief the maximum count in a ItemStack
    static constexpr unsigned MaxCount = 64;
    /// @brief construct an empty ItemStack
    ItemStack()
    {
    }
    /** @brief construct an ItemStack
     *
     * @param item the Item
     * @param count the Item count
     *
     */
    ItemStack(Item item, int count = 1)
        : item(count > 0 ? item : Item()), count(item.good() ? count : 0)
    {
        assert(this->count <= MaxCount);
    }
    /** @brief check if this ItemStack is non-empty
     * @return true if this ItemStack is non-empty
     */
    bool good() const
    {
        return count > 0 && item.good();
    }
    /** @brief check for ItemStack equality
     * @param rt the other ItemStack
     * @return true if this item equals rt
     * @see operator !=
     */
    bool operator ==(const ItemStack &rt) const
    {
        if(count == 0)
            return rt.count == 0;
        return count == rt.count && item == rt.item;
    }
    /** @brief check for ItemStack inequality
     * @param rt the other ItemStack
     * @return true if this item doesn't equal rt
     * @see operator ==
     */
    bool operator !=(const ItemStack &rt) const
    {
        if(count == 0)
            return rt.count != 0;
        return count != rt.count || item != rt.item;
    }
    /** @brief insert another item into this ItemStack
     * @param item the Item to insert
     * @return 1 if the item could be inserted, 0 otherwise
     */
    int insert(Item theItem)
    {
        if(!good())
        {
            item = theItem;
            count = 1;
            return 1;
        }
        if(item != theItem)
            return 0;
        if(count >= MaxCount)
            return 0;
        count++;
        return 1;
    }
    /** @brief remove an item from this ItemStack
     * @param item the Item to remove
     * @return 1 if the item could be removed, 0 otherwise
     */
    int remove(Item theItem)
    {
        if(!good())
            return 0;
        if(item != theItem)
            return 0;
        count--;
        if(count == 0)
        {
            item = Item();
        }
        return 1;
    }
    void render(Mesh &dest, float minX, float maxX, float minY, float maxY) const;
};

template <std::size_t W, std::size_t H>
class ItemStackArray final
{
public:
    checked_array<checked_array<ItemStack, H>, W> itemStacks;
    /** @brief insert another item into this ItemStackArray
     * @param item the Item to insert
     * @return 1 if the item could be inserted, 0 otherwise
     */
    int insert(Item theItem)
    {
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                int retval = itemStacks[x][y].insert(theItem);
                if(retval > 0)
                    return retval;
            }
        }
        return 0;
    }
    /** @brief remove an item from this ItemStackArray
     * @param item the Item to remove
     * @return 1 if the item could be removed, 0 otherwise
     */
    int remove(Item theItem)
    {
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                int retval = itemStacks[x][y].remove(theItem);
                if(retval > 0)
                    return retval;
            }
        }
        return 0;
    }
};
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::Item>
{
    size_t operator ()(const programmerjake::voxels::Item &v) const
    {
        return hash<programmerjake::voxels::ItemDescriptorPointer>()(v.descriptor);
    }
};
template <>
struct hash<programmerjake::voxels::ItemStack>
{
    size_t operator ()(const programmerjake::voxels::ItemStack &v) const
    {
        if(v.count == 0)
            return 0;
        return hash<programmerjake::voxels::Item>()(v.item) + 3 * hash<decltype(v.count)>()(v.count);
    }
};
}

#endif // ITEM_STRUCT_H_INCLUDED
