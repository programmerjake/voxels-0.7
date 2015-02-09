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
#include "util/linked_map.h"
#include "util/vector.h"
#include "render/mesh.h"
#include "util/block_face.h"
#include "render/render_layer.h"
#include "ray_casting/ray_casting.h"
#include "lighting/lighting.h"
#include "util/enum_traits.h"
#include "util/block_update.h"

namespace programmerjake
{
namespace voxels
{
class BlockDescriptor;
class BlockIterator;
struct WorldLockManager;

typedef const BlockDescriptor *BlockDescriptorPointer;

struct Block final
{
    BlockDescriptorPointer descriptor;
    std::shared_ptr<void> data;
    Lighting lighting;
    Block()
        : descriptor(nullptr), data(nullptr)
    {
    }
    Block(BlockDescriptorPointer descriptor, std::shared_ptr<void> data = nullptr)
        : descriptor(descriptor), data(data)
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
    static BlockLighting calcBlockLighting(BlockIterator &bi_in, WorldLockManager &lock_manager, WorldLightingProperties wlp);
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
public:
    const std::wstring name;
protected:
    BlockDescriptor(std::wstring name, BlockShape blockShape, LightProperties lightProperties, bool isStaticMesh, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY,
                    bool isFaceBlockedNZ, bool isFaceBlockedPZ, Mesh meshCenter, Mesh meshFaceNX, Mesh meshFacePX, Mesh meshFaceNY, Mesh meshFacePY, Mesh meshFaceNZ, Mesh meshFacePZ, RenderLayer staticRenderLayer);
    BlockDescriptor(std::wstring name, BlockShape blockShape, LightProperties lightProperties, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY, bool isFaceBlockedNZ,
                    bool isFaceBlockedPZ)
        : BlockDescriptor(name, blockShape, lightProperties, false, isFaceBlockedNX, isFaceBlockedPX, isFaceBlockedNY, isFaceBlockedPY, isFaceBlockedNZ, isFaceBlockedPZ, Mesh(), Mesh(), Mesh(), Mesh(),
                          Mesh(), Mesh(), Mesh(), RenderLayer::Opaque)
    {
    }
public:
    virtual ~BlockDescriptor();
    const BlockShape blockShape;
    const LightProperties lightProperties;
    const bool isStaticMesh;
    enum_array<bool, BlockFace> isFaceBlocked;
protected:
    /** generate dynamic mesh
     the generated mesh is at the absolute position of the block
     */
    virtual void renderDynamic(const Block &block, Mesh &dest, BlockIterator &blockIterator, WorldLockManager &lock_manager, RenderLayer rl, const BlockLighting &lighting) const
    {
        assert(false); // shouldn't be called
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
    /** generate mesh
     the generated mesh is at the absolute position of the block
     */
    void render(const Block &block, Mesh &dest, BlockIterator &blockIterator, WorldLockManager &lock_manager, RenderLayer rl, const BlockLighting &lighting) const
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
    virtual Block moveStep(const Block &block, BlockIterator &blockIterator, WorldLockManager &lock_manager) const
    {
        return block;
    }
    virtual bool isDataEqual(const std::shared_ptr<void> &a, const std::shared_ptr<void> &b) const
    {
        return a == b;
    }
    virtual std::size_t hashData(const std::shared_ptr<void> &data) const
    {
        return std::hash<std::shared_ptr<void>>()(data);
    }
    virtual RayCasting::Collision getRayCollision(const Block &block, BlockIterator &blockIterator, WorldLockManager &lock_manager, World &world, RayCasting::Ray ray) const
    {
        return RayCasting::Collision(world);
    }
    virtual Matrix getSelectionBoxTransform(const Block &block) const
    {
        return Matrix::identity();
    }
};

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

inline bool Block::operator==(const Block &r) const
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

inline BlockLighting Block::calcBlockLighting(BlockIterator &bi, WorldLockManager &lock_manager, WorldLightingProperties wlp)
{
    std::array<std::array<std::array<std::pair<LightProperties, Lighting>, 3>, 3>, 3> blocks;
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
