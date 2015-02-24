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
    bool operator !() const
    {
        return index != NullIndex;
    }
    friend bool operator ==(std::nullptr_t, BlockDescriptorIndex v)
    {
        return v.index == NullIndex;
    }
    friend bool operator ==(BlockDescriptorIndex v, std::nullptr_t)
    {
        return v.index == NullIndex;
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
        return v.index != NullIndex;
    }
    friend bool operator !=(BlockDescriptorIndex v, std::nullptr_t)
    {
        return v.index != NullIndex;
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

template <typename T>
struct BlockDataPointer : public BlockDataPointerBase
{
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
        : descriptor(nullptr), data(nullptr)
    {
    }
    Block(BlockDescriptorPointer descriptor, BlockDataPointer<BlockData> data = nullptr)
        : descriptor(descriptor), data(data)
    {
    }
    Block(BlockDescriptorPointer descriptor, Lighting lighting, BlockDataPointer<BlockData> data = nullptr)
        : descriptor(descriptor), lighting(lighting), data(data)
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

#include "util/block_iterator.h"

namespace programmerjake
{
namespace voxels
{
class BlockDescriptor
{
    BlockDescriptor(const BlockDescriptor &) = delete;
    void operator =(const BlockDescriptor &) = delete;
private:
    BlockDescriptorIndex bdIndex;
public:
    BlockDescriptorIndex getBlockDescriptorIndex() const
    {
        return bdIndex;
    }
    const std::wstring name;
protected:
    BlockDescriptor(std::wstring name, BlockShape blockShape, LightProperties lightProperties, RayCasting::BlockCollisionMask blockRayCollisionMask, bool isStaticMesh, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY,
                    bool isFaceBlockedNZ, bool isFaceBlockedPZ, Mesh meshCenter, Mesh meshFaceNX, Mesh meshFacePX, Mesh meshFaceNY, Mesh meshFacePY, Mesh meshFaceNZ, Mesh meshFacePZ, RenderLayer staticRenderLayer);
    BlockDescriptor(std::wstring name, BlockShape blockShape, LightProperties lightProperties, RayCasting::BlockCollisionMask blockRayCollisionMask, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY, bool isFaceBlockedNZ,
                    bool isFaceBlockedPZ)
        : BlockDescriptor(name, blockShape, lightProperties, blockRayCollisionMask, false, isFaceBlockedNX, isFaceBlockedPX, isFaceBlockedNY, isFaceBlockedPY, isFaceBlockedNZ, isFaceBlockedPZ, Mesh(), Mesh(), Mesh(), Mesh(),
                          Mesh(), Mesh(), Mesh(), RenderLayer::Opaque)
    {
    }
public:
    virtual ~BlockDescriptor();
    const BlockShape blockShape;
    const LightProperties lightProperties;
    const RayCasting::BlockCollisionMask blockRayCollisionMask;
    const bool isStaticMesh;
    enum_array<bool, BlockFace> isFaceBlocked;
protected:
    /** generate dynamic mesh
     the generated mesh is at the absolute position of the block
     */
    virtual void renderDynamic(const Block &block, Mesh &dest, BlockIterator blockIterator, WorldLockManager &lock_manager, RenderLayer rl, const BlockLighting &lighting) const
    {
        assert(false); // shouldn't be called
    }
    virtual bool drawsAnythingDynamic(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
        return true;
    }
    Mesh meshCenter;
    enum_array<Mesh, BlockFace> meshFace;
    const RenderLayer staticRenderLayer;
    static Mesh &getTempRenderMesh()
    {
        static thread_local Mesh retval;
        return retval;
    }
    static void lightMesh(Mesh &mesh, const BlockLighting &lighting)
    {
        BlockLighting bl = lighting;

        for(Triangle &tri : mesh.triangles)
        {
            tri.c1 = bl.lightVertex(tri.p1, tri.c1, tri.n1);
            tri.c2 = bl.lightVertex(tri.p2, tri.c2, tri.n2);
            tri.c3 = bl.lightVertex(tri.p3, tri.c3, tri.n3);
        }
    }
public:
    bool drawsAnything(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
        if(isStaticMesh)
        {
            for(BlockFace bf : enum_traits<BlockFace>())
            {
                BlockIterator i = blockIterator;
                i.moveToward(bf);
                Block b = i.get(lock_manager);

                if(!b)
                {
                    continue;
                }

                if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)])
                {
                    continue;
                }

                if(meshFace[bf].size() == 0)
                    continue;

                return true;
            }
            return false;
        }
        return drawsAnythingDynamic(block, blockIterator, lock_manager);
    }
    /** generate mesh
     the generated mesh is at the absolute position of the block
     */
    void render(const Block &block, Mesh &dest, BlockIterator blockIterator, WorldLockManager &lock_manager, RenderLayer rl, const BlockLighting &lighting) const
    {
        if(isStaticMesh)
        {
            if(rl != staticRenderLayer)
            {
                return;
            }

            bool drewAny = false;
            Matrix tform = Matrix::translate((VectorF)blockIterator.position());
            Mesh &blockMesh = getTempRenderMesh();
            blockMesh.clear();
            for(BlockFace bf : enum_traits<BlockFace>())
            {
                BlockIterator i = blockIterator;
                i.moveToward(bf);
                Block b = i.get(lock_manager);

                if(!b)
                {
                    continue;
                }

                if(b.descriptor->isFaceBlocked[getOppositeBlockFace(bf)])
                {
                    continue;
                }

                blockMesh.append(meshFace[bf]);
                drewAny = true;
            }

            if(drewAny)
            {
                blockMesh.append(meshCenter);
                lightMesh(blockMesh, lighting);
                dest.append(transform(tform, blockMesh));
            }
        }
        else
        {
            renderDynamic(block, dest, blockIterator, lock_manager, rl, lighting);
        }
    }
    virtual Block moveStep(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
        return block;
    }
    virtual bool isDataEqual(const BlockDataPointer<BlockData> &a, const BlockDataPointer<BlockData> &b) const
    {
        return a == b;
    }
    virtual std::size_t hashData(const BlockDataPointer<BlockData> &data) const
    {
        return std::hash<BlockData *>()(data.get());
    }
    virtual RayCasting::Collision getRayCollision(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, World &world, RayCasting::Ray ray) const
    {
        return RayCasting::Collision(world);
    }
    virtual Matrix getSelectionBoxTransform(const Block &block) const
    {
        return Matrix::identity();
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const
    {
    }
};

#ifdef PACK_BLOCK
inline PackedBlock::PackedBlock(const Block &b)
    : descriptor(b.descriptor ? b.descriptor->getBlockDescriptorIndex() : BlockDescriptorIndex(nullptr)), lighting(b.lighting), data(b.data)
{
}
#endif

inline void Block::createNewLighting(Lighting oldLighting)
{
    if(!good())
    {
        lighting = Lighting();
    }
    else
    {
        lighting = descriptor->lightProperties.createNewLighting(oldLighting);
    }
}

inline bool Block::operator ==(const Block &r) const
{
    if(descriptor != r.descriptor)
    {
        return false;
    }

    if(descriptor == nullptr)
    {
        return true;
    }

    if(lighting != r.lighting)
    {
        return false;
    }

    if(data == r.data)
    {
        return true;
    }

    return descriptor->isDataEqual(data, r.data);
}

inline BlockLighting Block::calcBlockLighting(BlockIterator bi, WorldLockManager &lock_manager, WorldLightingProperties wlp)
{
    checked_array<checked_array<checked_array<std::pair<LightProperties, Lighting>, 3>, 3>, 3> blocks;
    for(int x = 0; (size_t)x < blocks.size(); x++)
    {
        for(int y = 0; (size_t)y < blocks[x].size(); y++)
        {
            for(int z = 0; (size_t)z < blocks[x][y].size(); z++)
            {
                BlockIterator curBi = bi;
                curBi.moveBy(VectorI(x - 1, y - 1, z - 1));
                auto l = std::pair<LightProperties, Lighting>(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
                const Block &b = curBi.get(lock_manager);

                if(b)
                {
                    std::get<0>(l) = b.descriptor->lightProperties;
                    std::get<1>(l) = b.lighting;
                }

                blocks[x][y][z] = l;
            }
        }
    }

    return BlockLighting(blocks, wlp);
}
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
    size_t operator()(const programmerjake::voxels::Block &b) const
    {
        if(b.descriptor == nullptr)
        {
            return 0;
        }

        return std::hash<programmerjake::voxels::BlockDescriptorPointer>()(b.descriptor) + b.descriptor->hashData(b.data);
    }
};
}

namespace programmerjake
{
namespace voxels
{
class BlockDescriptors_t final
{
    friend class BlockDescriptor;
private:
    typedef linked_map<std::wstring, BlockDescriptorPointer> MapType;
    static MapType *blocksMap;
    void add(BlockDescriptorPointer bd) const;
    void remove(BlockDescriptorPointer bd) const;
public:
    constexpr BlockDescriptors_t() noexcept
    {
    }
    BlockDescriptorPointer operator [](std::wstring name) const
    {
        if(blocksMap == nullptr)
        {
            return nullptr;
        }

        auto iter = blocksMap->find(name);

        if(iter == blocksMap->end())
        {
            return nullptr;
        }

        return std::get<1>(*iter);
    }
    BlockDescriptorPointer at(std::wstring name) const
    {
        BlockDescriptorPointer retval = operator [](name);

        if(retval == nullptr)
        {
            throw std::out_of_range("BlockDescriptor not found");
        }

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
            {
                return r.empty;
            }

            if(r.empty)
            {
                return false;
            }

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
        {
            return iterator();
        }

        return iterator(blocksMap->cbegin());
    }
    iterator end() const
    {
        if(blocksMap == nullptr)
        {
            return iterator();
        }

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
    iterator find(std::wstring name) const
    {
        if(blocksMap == nullptr)
        {
            return iterator();
        }

        return iterator(blocksMap->find(name));
    }
    std::size_t size() const
    {
        if(blocksMap == nullptr)
        {
            return 0;
        }

        return blocksMap->size();
    }
};

static constexpr BlockDescriptors_t BlockDescriptors;
}
}

#endif // BLOCK_H_INCLUDED
