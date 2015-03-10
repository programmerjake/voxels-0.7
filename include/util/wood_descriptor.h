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
#ifndef WOOD_DESCRIPTOR_H_INCLUDED
#define WOOD_DESCRIPTOR_H_INCLUDED

#include "texture/texture_descriptor.h"
#include "util/linked_map.h"
#include "util/enum_traits.h"
#include "block/block_struct.h"
#include "entity/entity_struct.h"
#include "item/item_struct.h"
#include <iterator>
#include <string>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
enum class LogOrientation
{
    AllBark,
    X,
    Y,
    Z,
    DEFINE_ENUM_LIMITS(AllBark, Z)
};

class WoodDescriptor;
typedef const WoodDescriptor *WoodDescriptorPointer;
class WoodDescriptor
{
    WoodDescriptor(const WoodDescriptor &) = delete;
    const WoodDescriptor &operator =(const WoodDescriptor &) = delete;
public:
    virtual ~WoodDescriptor() = default;
    const std::wstring name;
private:
    TextureDescriptor logTop, logSide, planks, sapling, leaves;
    enum_array<BlockDescriptorPointer, LogOrientation> logBlockDescriptors;
    BlockDescriptorPointer planksBlockDescriptor;
    BlockDescriptorPointer saplingBlockDescriptor;
    BlockDescriptorPointer leavesBlockDescriptor;
    ItemDescriptorPointer logItemDescriptor;
    ItemDescriptorPointer planksItemDescriptor;
    ItemDescriptorPointer saplingItemDescriptor;
    ItemDescriptorPointer leavesItemDescriptor;
    EntityDescriptorPointer logEntityDescriptor;
    EntityDescriptorPointer planksEntityDescriptor;
    EntityDescriptorPointer saplingEntityDescriptor;
    EntityDescriptorPointer leavesEntityDescriptor;
protected:
    WoodDescriptor(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves);
    void setDescriptors(enum_array<BlockDescriptorPointer, LogOrientation> logBlockDescriptors,
                        BlockDescriptorPointer planksBlockDescriptor,
                        BlockDescriptorPointer saplingBlockDescriptor,
                        BlockDescriptorPointer leavesBlockDescriptor,
                        ItemDescriptorPointer logItemDescriptor,
                        ItemDescriptorPointer planksItemDescriptor,
                        ItemDescriptorPointer saplingItemDescriptor,
                        ItemDescriptorPointer leavesItemDescriptor,
                        EntityDescriptorPointer logEntityDescriptor,
                        EntityDescriptorPointer planksEntityDescriptor,
                        EntityDescriptorPointer saplingEntityDescriptor,
                        EntityDescriptorPointer leavesEntityDescriptor)
    {
        this->logBlockDescriptors = logBlockDescriptors;
        this->planksBlockDescriptor = planksBlockDescriptor;
        this->saplingBlockDescriptor = saplingBlockDescriptor;
        this->leavesBlockDescriptor = leavesBlockDescriptor;
        this->logItemDescriptor = logItemDescriptor;
        this->planksItemDescriptor = planksItemDescriptor;
        this->saplingItemDescriptor = saplingItemDescriptor;
        this->leavesItemDescriptor = leavesItemDescriptor;
        this->logEntityDescriptor = logEntityDescriptor;
        this->planksEntityDescriptor = planksEntityDescriptor;
        this->saplingEntityDescriptor = saplingEntityDescriptor;
        this->leavesEntityDescriptor = leavesEntityDescriptor;
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
    BlockDescriptorPointer getLogBlockDescriptor(LogOrientation logOrientation) const
    {
        return logBlockDescriptors[logOrientation];
    }
    BlockDescriptorPointer getPlanksBlockDescriptor() const
    {
        return planksBlockDescriptor;
    }
    BlockDescriptorPointer getSaplingBlockDescriptor() const
    {
        return saplingBlockDescriptor;
    }
    BlockDescriptorPointer getLeavesBlockDescriptor() const
    {
        return leavesBlockDescriptor;
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
    EntityDescriptorPointer getLogEntityDescriptor() const
    {
        return logEntityDescriptor;
    }
    EntityDescriptorPointer getPlanksEntityDescriptor() const
    {
        return planksEntityDescriptor;
    }
    EntityDescriptorPointer getSaplingEntityDescriptor() const
    {
        return saplingEntityDescriptor;
    }
    EntityDescriptorPointer getLeavesEntityDescriptor() const
    {
        return leavesEntityDescriptor;
    }
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
    class iterator final : public std::iterator<std::forward_iterator_tag, const WoodDescriptorPointer>
    {
    private:
        typename linked_map<std::wstring, WoodDescriptorPointer>::const_iterator iter;
    public:
        explicit iterator(typename linked_map<std::wstring, WoodDescriptorPointer>::const_iterator iter)
            : iter(iter)
        {
        }
        iterator()
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

inline WoodDescriptor::WoodDescriptor(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves)
    : name(name), logTop(logTop), logSide(logSide), planks(planks), sapling(sapling), leaves(leaves)
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
    SimpleWood(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves);
};
}
}
}
}

#endif // WOOD_DESCRIPTOR_H_INCLUDED
