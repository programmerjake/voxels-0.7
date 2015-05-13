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
#ifndef BLOCK_STRUCT_H_INCLUDED
#define BLOCK_STRUCT_H_INCLUDED

#include <cstdint>
#include <memory>
#include <cwchar>
#include <string>
#include <cassert>
#include <tuple>
#include <stdexcept>
#include <iterator>
#include <utility>
#include <atomic>
#include <type_traits>
#include "util/linked_map.h"
#include "util/vector.h"
#include "render/mesh.h"
#include "util/block_face.h"
#include "render/render_layer.h"
#include "ray_casting/ray_casting.h"
#include "lighting/lighting.h"
#include "util/enum_traits.h"
#include "util/block_update.h"
#include "util/checked_array.h"

#define PACK_BLOCK

namespace programmerjake
{
namespace voxels
{
class BlockDescriptor;
class BlockIterator;
struct WorldLockManager;

typedef const BlockDescriptor *BlockDescriptorPointer;

struct BlockDescriptorIndex
{
private:
    static const BlockDescriptor **blockDescriptorTable;
    static std::size_t blockDescriptorTableSize, blockDescriptorTableAllocated;
    friend class BlockDescriptor;
    explicit BlockDescriptorIndex(std::uint16_t index)
        : index(index)
    {
    }
    static BlockDescriptorIndex addNewDescriptor(const BlockDescriptor *bd) // should only be called before any threads other than the main thread are started
    {
        assert(bd != nullptr);
        if(blockDescriptorTableSize >= blockDescriptorTableAllocated)
        {
            blockDescriptorTableAllocated += 1024;
            const BlockDescriptor **newTable = new const BlockDescriptor *[blockDescriptorTableAllocated];
            for(std::size_t i = 0; i < blockDescriptorTableSize; i++)
                newTable[i] = blockDescriptorTable[i];
            delete []blockDescriptorTable;
            blockDescriptorTable = newTable;
        }
        std::size_t index = blockDescriptorTableSize++;
        if(index >= (std::size_t)NullIndex)
        {
            throw std::runtime_error("too many BlockDescriptor instances");
        }
        blockDescriptorTable[index] = bd;
        return make(index);
    }
public:
    static BlockDescriptorIndex make(std::uint16_t index)
    {
        assert(index == NullIndex || index < blockDescriptorTableSize);
        return BlockDescriptorIndex(index);
    }
    std::uint16_t index;
    constexpr BlockDescriptorIndex()
        : index(NullIndex)
    {
    }
    static constexpr std::uint16_t NullIndex = ~(std::uint16_t)0;
    constexpr BlockDescriptorIndex(std::nullptr_t)
        : index(NullIndex)
    {
    }
    BlockDescriptorIndex(BlockDescriptorPointer bd);
    const BlockDescriptor *get() const
    {
        if(index == NullIndex)
            return nullptr;
        assert(index < blockDescriptorTableSize);
        return blockDescriptorTable[index];
    }
    const BlockDescriptor *operator ->() const
    {
        assert(index < blockDescriptorTableSize);
        return blockDescriptorTable[index];
    }
    const BlockDescriptor &operator *() const
    {
        assert(index < blockDescriptorTableSize);
        return *blockDescriptorTable[index];
    }
    operator const BlockDescriptor *() const
    {
        return get();
    }
    explicit operator bool() const
    {
        return index != NullIndex;
    }
    bool operator !() const
    {
        return index == NullIndex;
    }
    friend bool operator ==(std::nullptr_t, BlockDescriptorIndex v)
    {
        return !v;
    }
    friend bool operator ==(BlockDescriptorIndex v, std::nullptr_t)
    {
        return !v;
    }
    friend bool operator ==(const BlockDescriptor *a, BlockDescriptorIndex b)
    {
        return a == b.get();
    }
    friend bool operator ==(BlockDescriptorIndex a, const BlockDescriptor *b)
    {
        return a.get() == b;
    }
    friend bool operator ==(BlockDescriptorIndex a, BlockDescriptorIndex b)
    {
        return a.index == b.index;
    }
    friend bool operator !=(std::nullptr_t, BlockDescriptorIndex v)
    {
        return static_cast<bool>(v);
    }
    friend bool operator !=(BlockDescriptorIndex v, std::nullptr_t)
    {
        return static_cast<bool>(v);
    }
    friend bool operator !=(const BlockDescriptor *a, BlockDescriptorIndex b)
    {
        return a != b.get();
    }
    friend bool operator !=(BlockDescriptorIndex a, const BlockDescriptor *b)
    {
        return a.get() != b;
    }
    friend bool operator !=(BlockDescriptorIndex a, BlockDescriptorIndex b)
    {
        return a.index != b.index;
    }
};

class BlockDataPointerBase;

struct BlockData
{
    friend class BlockDataPointerBase;
private:
    std::atomic_uint_fast32_t referenceCount;
public:
    BlockData()
        : referenceCount(0)
    {
    }
    virtual ~BlockData()
    {
    }
    BlockData(const BlockData &) = delete;
    const BlockData &operator =(const BlockData &) = delete;
};

class BlockDataPointerBase
{
private:
    BlockData *ptr;
    void addRef() const
    {
        if(ptr != nullptr)
            ptr->referenceCount.fetch_add(1, std::memory_order_relaxed);
    }
    void remRef()
    {
        if(ptr != nullptr)
        {
            if(ptr->referenceCount.fetch_sub(1, std::memory_order_acq_rel) == 0)
            {
                delete ptr;
            }
        }
    }
protected:
    explicit BlockDataPointerBase(BlockData *ptr)
        : ptr(ptr)
    {
        addRef();
    }
    ~BlockDataPointerBase()
    {
        remRef();
    }
    BlockDataPointerBase(const BlockDataPointerBase &rt)
        : ptr(rt.ptr)
    {
        addRef();
    }
    BlockDataPointerBase(BlockDataPointerBase &&rt)
        : ptr(rt.ptr)
    {
        rt.ptr = nullptr;
    }
    const BlockDataPointerBase &operator =(const BlockDataPointerBase &rt)
    {
        rt.addRef();
        remRef();
        ptr = rt.ptr;
        return *this;
    }
    const BlockDataPointerBase &operator =(BlockDataPointerBase &&rt)
    {
        if(rt.ptr == ptr)
            return *this;
        remRef();
        ptr = rt.ptr;
        rt.ptr = nullptr;
        return *this;
    }
    BlockData *get() const
    {
        return ptr;
    }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename T>
struct BlockDataPointer : public BlockDataPointerBase
{
#pragma GCC diagnostic pop
    T *get() const
    {
        return static_cast<T *>(BlockDataPointerBase::get());
    }
    explicit BlockDataPointer(T *ptr = nullptr)
        : BlockDataPointerBase(ptr)
    {
    }
    BlockDataPointer(std::nullptr_t)
        : BlockDataPointerBase(nullptr)
    {
    }
    template <typename U, typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    BlockDataPointer(const BlockDataPointer<U> &rt)
        : BlockDataPointerBase(rt)
    {
    }
    template <typename U, typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    BlockDataPointer(BlockDataPointer<U> &&rt)
        : BlockDataPointerBase(std::move(rt))
    {
    }
    template <typename U, typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    const BlockDataPointer &operator =(const BlockDataPointer<U> &rt)
    {
        BlockDataPointerBase::operator =(rt);
        return *this;
    }
    template <typename U, typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    const BlockDataPointer &operator =(BlockDataPointer<U> &&rt)
    {
        BlockDataPointerBase::operator =(std::move(rt));
        return *this;
    }
    friend bool operator ==(std::nullptr_t, const BlockDataPointer &b)
    {
        return nullptr == b.get();
    }
    friend bool operator ==(const BlockDataPointer &a, std::nullptr_t)
    {
        return a.get() == nullptr;
    }
    friend bool operator !=(std::nullptr_t, const BlockDataPointer &b)
    {
        return nullptr != b.get();
    }
    friend bool operator !=(const BlockDataPointer &a, std::nullptr_t)
    {
        return a.get() != nullptr;
    }
    T &operator *() const
    {
        return *get();
    }
    T *operator ->() const
    {
        return get();
    }
    explicit operator bool() const
    {
        return get() != nullptr;
    }
    bool operator !() const
    {
        return get() == nullptr;
    }
};

template <typename T, typename U>
inline bool operator ==(const BlockDataPointer<T> &a, const BlockDataPointer<U> &b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator !=(const BlockDataPointer<T> &a, const BlockDataPointer<U> &b)
{
    return a.get() != b.get();
}

template <typename T, typename U>
inline BlockDataPointer<T> static_pointer_cast(const BlockDataPointer<U> &v)
{
    return BlockDataPointer<T>(static_cast<T *>(v.get()));
}

template <typename T, typename U>
inline BlockDataPointer<T> const_pointer_cast(const BlockDataPointer<U> &v)
{
    return BlockDataPointer<T>(const_cast<T *>(v.get()));
}

template <typename T, typename U>
inline BlockDataPointer<T> dynamic_pointer_cast(const BlockDataPointer<U> &v)
{
    return BlockDataPointer<T>(dynamic_cast<T *>(v.get()));
}

struct Block final
{
    BlockDescriptorPointer descriptor;
    Lighting lighting;
    BlockDataPointer<BlockData> data;
    Block()
        : descriptor(nullptr), lighting(), data(nullptr)
    {
    }
    Block(BlockDescriptorPointer descriptor, BlockDataPointer<BlockData> data = nullptr)
        : descriptor(descriptor), lighting(), data(data)
    {
    }
    Block(BlockDescriptorPointer descriptor, Lighting lighting, BlockDataPointer<BlockData> data = nullptr)
        : descriptor(descriptor), lighting(lighting), data(data)
    {
    }
    Block(const Block &rt) = default;
    Block(Block &&rt) = default;
    Block &operator =(const Block &rt) = default;
    Block &operator =(Block &&rt) = default;
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
    bool operator ==(const Block &r) const;
    bool operator !=(const Block &r) const
    {
        return !operator ==(r);
    }
    void createNewLighting(Lighting oldLighting);
    static BlockLighting calcBlockLighting(BlockIterator bi, WorldLockManager &lock_manager, WorldLightingProperties wlp);
};

#ifdef PACK_BLOCK
struct PackedBlock final
{
    BlockDescriptorIndex descriptor;
    PackedLighting lighting;
    BlockDataPointer<BlockData> data;
    explicit PackedBlock(const Block &b);
    PackedBlock()
        : descriptor(),
        lighting(),
        data()
    {
    }
    explicit operator Block() const
    {
        return Block(descriptor.get(), (Lighting)lighting, data);
    }
};
#else
typedef Block PackedBlock;
#endif

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
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::BlockDescriptorIndex>
{
    size_t operator()(const programmerjake::voxels::BlockDescriptorIndex v) const
    {
        return (std::size_t)v.index;
    }
};
template <typename T>
struct hash<programmerjake::voxels::BlockDataPointer<T>>
{
    hash<T *> hasher;
    size_t operator()(const programmerjake::voxels::BlockDataPointer<T> &v) const
    {
        return hasher(v.get());
    }
};
template <>
struct hash<programmerjake::voxels::Block>
{
    size_t operator()(const programmerjake::voxels::Block &b) const;
};
}

#endif // BLOCK_STRUCT_H_INCLUDED
