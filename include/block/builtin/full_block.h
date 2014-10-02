#ifndef FULL_BLOCK_H_INCLUDED
#define FULL_BLOCK_H_INCLUDED

#include "block/block.h"
#include "render/generate.h"
#include "texture/texture_descriptor.h"

namespace Blocks
{
namespace builtin
{
class FullBlock : public BlockDescriptor
{
private:
    static Mesh makeFaceMeshNX(TextureDescriptor td)
    {
        return Generate::unitBox(td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshPX(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshNY(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshPY(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor());
    }
    static Mesh makeFaceMeshNZ(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor());
    }
    static Mesh makeFaceMeshPZ(TextureDescriptor td)
    {
        return Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td);
    }
protected:
    FullBlock(wstring name, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY, bool isFaceBlockedNZ, bool isFaceBlockedPZ, TextureDescriptor tdNX, TextureDescriptor tdPX, TextureDescriptor tdNY, TextureDescriptor tdPY, TextureDescriptor tdNZ, TextureDescriptor tdPZ, RenderLayer rl)
        : BlockDescriptor(name, BlockShape(VectorF(0.5f), VectorF(0.5f)), true, isFaceBlockedNX, isFaceBlockedPX, isFaceBlockedNY, isFaceBlockedPY, isFaceBlockedNZ, isFaceBlockedPZ, Mesh(), makeFaceMeshNX(tdNX), makeFaceMeshPX(tdPX), makeFaceMeshNY(tdNY), makeFaceMeshPY(tdPY), makeFaceMeshNZ(tdNZ), makeFaceMeshPZ(tdPZ), rl)
    {
    }
    FullBlock(wstring name, bool areFacesBlocked, TextureDescriptor td, RenderLayer rl = RenderLayer::Opaque)
        : FullBlock(name, areFacesBlocked, areFacesBlocked, areFacesBlocked, areFacesBlocked, areFacesBlocked, areFacesBlocked, td, td, td, td, td, td, rl)
    {
    }
public:
    virtual RayCasting::Collision getRayCollision(const Block &block, BlockIterator blockIterator, World &world, RayCasting::Ray ray) const
    {
        if(ray.dimension() != blockIterator.position().d)
            return RayCasting::Collision(world);
        tuple<bool, float, BlockFace> collision = ray.getAABoxEnterFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1));
        if(!std::get<0>(collision))
            return RayCasting::Collision(world);
        if(std::get<1>(collision) < RayCasting::Ray::eps)
        {
            collision = ray.getAABoxExitFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1));
            if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
                return RayCasting::Collision(world);
            std::get<1>(collision) = RayCasting::Ray::eps;
        }
        return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position());
    }
};
}
}

#endif // FULL_BLOCK_H_INCLUDED
