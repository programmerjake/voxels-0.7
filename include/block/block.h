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
#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED

#include <cstdint>
#include <memory>
#include <cwchar>
#include <string>
#include <cassert>
#include <tuple>
#include <stdexcept>
#include <iterator>
#include "util/linked_map.h"
#include "util/vector.h"

using namespace std;

class BlockDescriptor;

typedef const BlockDescriptor *BlockDescriptorPointer;
typedef const BlockDescriptor &BlockDescriptorReference;

struct Block final
{
    BlockDescriptorPointer descriptor;
    int32_t integerData;
    shared_ptr<void> extraData;
    Block()
        : descriptor(nullptr), integerData(0), extraData(nullptr)
    {
    }
    Block(BlockDescriptorPointer descriptor, int32_t integerData = 0, shared_ptr<void> extraData = nullptr)
        : descriptor(descriptor), integerData(integerData), extraData(extraData)
    {
    }
    bool good() const
    {
        return descriptor != nullptr;
    }
    explicit operator bool() const
    {
        return good();
    }
    bool operator !() const
    {
        return !good();
    }
};

struct BlockShape final
{
    VectorF offset, extents;
    BlockShape()
        : offset(0), extents(-1)
    {
    }
    BlockShape(std::nullptr_t)
        : BlockShape()
    {
    }
    BlockShape(VectorF offset, VectorF extents)
        : offset(offset), extents(extents)
    {
        assert(extents.x > 0 && extents.y > 0 && extents.z > 0);
    }
    bool empty() const
    {
        return extents.x <= 0;
    }
};

class BlockDescriptor
{
    BlockDescriptor(const BlockDescriptor &) = delete;
    void operator =(const BlockDescriptor &) = delete;
public:
    const wstring name;
protected:
    BlockDescriptor(wstring name, BlockShape blockShape);
    virtual ~BlockDescriptor();
public:
    const BlockShape blockShape;
};

class BlockDescriptors_t final
{
    friend class BlockDescriptor;
private:
    typedef linked_map<wstring, BlockDescriptorPointer> MapType;
    static MapType *blocksMap;
    void add(BlockDescriptorPointer bd) const;
    void remove(BlockDescriptorPointer bd) const;
public:
    BlockDescriptorPointer operator [](wstring name) const
    {
        if(blocksMap == nullptr)
            return nullptr;
        auto iter = blocksMap->find(name);
        if(iter == blocksMap->end())
            return nullptr;
        return std::get<1>(*iter);
    }
    BlockDescriptorPointer at(wstring name) const
    {
        BlockDescriptorPointer retval = operator [](name);
        if(retval == nullptr)
            throw out_of_range("BlockDescriptor not found");
        return retval;
    }
    class iterator final : public std::iterator<std::forward_iterator_tag, const BlockDescriptorPointer>
    {
        friend class BlockDescriptors_t;
        MapType::const_iterator iter;
        bool empty;
        explicit iterator(MapType::const_iterator iter)
            : iter(iter), empty(false)
        {
        }
    public:
        iterator()
            : iter(), empty(true)
        {
        }
        bool operator ==(const iterator &r) const
        {
            if(empty)
                return r.empty;
            if(r.empty)
                return false;
            return iter == r.iter;
        }
        bool operator !=(const iterator &r) const
        {
            return !operator ==(r);
        }
        const BlockDescriptorPointer &operator *() const
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
    };
    iterator begin() const
    {
        if(blocksMap == nullptr)
            return iterator();
        return iterator(blocksMap->cbegin());
    }
    iterator end() const
    {
        if(blocksMap == nullptr)
            return iterator();
        return iterator(blocksMap->cend());
    }
    iterator cbegin() const
    {
        return begin();
    }
    iterator cend() const
    {
        return end();
    }
    iterator find(wstring name) const
    {
        if(blocksMap == nullptr)
            return iterator();
        return iterator(blocksMap->find(name));
    }
    size_t size() const
    {
        if(blocksMap == nullptr)
            return 0;
        return blocksMap->size();
    }
};

static const BlockDescriptors_t BlockDescriptors;

#endif // BLOCK_H_INCLUDED
