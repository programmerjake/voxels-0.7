/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef ITEM_STRUCT_H_INCLUDED
#define ITEM_STRUCT_H_INCLUDED

#include <memory>
#include <cstdint>
#include <utility>
#include <cassert>
#include "render/mesh.h"
#include "util/checked_array.h"
#include "stream/stream.h"

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
    Item(const Item &) = default;
    Item(Item &&) = default;
    Item &operator=(const Item &) = default;
    Item &operator=(Item &&) = default;
    /// @brief the ItemDescriptor
    ItemDescriptorPointer descriptor = nullptr;
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
    Item() : descriptor(nullptr), data()
    {
    }
    /** @brief construct an item
     *
     * @param descriptor the ItemDescriptor
     * @param data the associated data
     *
     */
    explicit Item(ItemDescriptorPointer descriptor, std::shared_ptr<void> data = nullptr)
        : descriptor(descriptor),
          data(descriptor == nullptr ? std::shared_ptr<void>(nullptr) : data)
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
    bool operator==(const Item &rt) const
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
    bool operator!=(const Item &rt) const
    {
        if(descriptor == nullptr)
            return rt.descriptor != nullptr;
        return descriptor != rt.descriptor || !dataEqual(rt);
    }
    unsigned getMaxStackCount() const;
    static Item read(stream::Reader &reader);
    void write(stream::Writer &writer) const;
};

struct ItemStack final
{
    /// @brief the Item
    Item item;
    /// @brief the Item count
    unsigned count = 0;
    /// @brief the maximum count in a ItemStack
    unsigned getMaxCount() const
    {
        return item.getMaxStackCount();
    }
    /// @brief construct an empty ItemStack
    ItemStack() : item()
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
        assert(!this->item.good() || this->count <= getMaxCount());
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
    bool operator==(const ItemStack &rt) const
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
    bool operator!=(const ItemStack &rt) const
    {
        if(count == 0)
            return rt.count != 0;
        return count != rt.count || item != rt.item;
    }
    /** @brief insert another item into this ItemStack
     * @param item the Item to insert
     * @return 1 if the item could be inserted, 0 otherwise
     */
    unsigned insert(Item theItem)
    {
        if(!good())
        {
            item = theItem;
            count = 1;
            return 1;
        }
        if(item != theItem)
            return 0;
        if(count >= getMaxCount())
            return 0;
        count++;
        return 1;
    }
    /** @brief remove an item from this ItemStack
     * @param item the Item to remove
     * @return 1 if the item could be removed, 0 otherwise
     */
    unsigned remove(Item theItem)
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
    unsigned transfer(ItemStack &src, unsigned transferCount)
    {
        if(transferCount > src.count)
            transferCount = src.count;
        if(transferCount > src.getMaxCount() - count)
            transferCount = src.getMaxCount() - count;
        if(transferCount == 0)
            return 0;
        if(!src.good())
            return 0;
        if(!good())
            item = src.item;
        else if(item != src.item)
            return 0;
        count += transferCount;
        src.count -= transferCount;
        if(src.count == 0)
            src = ItemStack();
        return transferCount;
    }
    void render(Mesh &dest, float minX, float maxX, float minY, float maxY) const;
    static ItemStack read(stream::Reader &reader)
    {
        std::uint8_t count = stream::read<std::uint8_t>(reader);
        if(count == 0)
            return ItemStack();
        Item item = stream::read<Item>(reader);
        if(!item.good())
            throw stream::InvalidDataValueException(
                "read empty item after non-zero count for ItemStack");
        if(count > item.getMaxStackCount())
            throw stream::InvalidDataValueException("count too high for item");
        return ItemStack(item, count);
    }
    void write(stream::Writer &writer) const
    {
        assert(static_cast<std::uint8_t>(getMaxCount()) == getMaxCount());
        std::uint8_t count = this->count;
        if(!good())
            count = 0;
        stream::write<std::uint8_t>(writer, static_cast<std::uint8_t>(count));
        if(count == 0)
            return;
        stream::write<Item>(writer, item);
    }
};

template <std::size_t W, std::size_t H>
class ItemStackArray final
{
public:
    ItemStackArray() : itemStacks()
    {
    }
    checked_array<checked_array<ItemStack, H>, W> itemStacks;
    /** @brief insert another item into this ItemStackArray
     * @param item the Item to insert
     * @return 1 if the item could be inserted, 0 otherwise
     */
    unsigned insert(Item theItem)
    {
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                unsigned retval = itemStacks[x][y].insert(theItem);
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
    unsigned remove(Item theItem)
    {
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                unsigned retval = itemStacks[x][y].remove(theItem);
                if(retval > 0)
                    return retval;
            }
        }
        return 0;
    }
    unsigned transfer(ItemStack &src, unsigned count)
    {
        unsigned retval = 0;
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                retval += itemStacks[x][y].transfer(src, count - retval);
                if(count <= retval)
                    return retval;
            }
        }
        return retval;
    }
    void clear()
    {
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                itemStacks[x][y] = ItemStack();
            }
        }
    }
    void write(stream::Writer &writer)
    {
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                stream::write<ItemStack>(writer, itemStacks[x][y]);
            }
        }
    }
    static ItemStackArray read(stream::Reader &reader)
    {
        ItemStackArray retval;
        for(std::size_t y = 0; y < H; y++)
        {
            for(std::size_t x = 0; x < W; x++)
            {
                retval.itemStacks[x][y] = stream::read<ItemStack>(reader);
            }
        }
        return retval;
    }
};
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::Item>
{
    size_t operator()(const programmerjake::voxels::Item &v) const
    {
        return hash<programmerjake::voxels::ItemDescriptorPointer>()(v.descriptor);
    }
};
template <>
struct hash<programmerjake::voxels::ItemStack>
{
    size_t operator()(const programmerjake::voxels::ItemStack &v) const
    {
        if(v.count == 0)
            return 0;
        return hash<programmerjake::voxels::Item>()(v.item)
               + 3 * hash<decltype(v.count)>()(v.count);
    }
};
}

#endif // ITEM_STRUCT_H_INCLUDED
