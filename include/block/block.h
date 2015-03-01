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
#include "block/block_struct.h"
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
    virtual void randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const
    {
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
}
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
