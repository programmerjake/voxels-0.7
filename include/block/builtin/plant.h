#ifndef BLOCK_PLANT_H_INCLUDED
#define BLOCK_PLANT_H_INCLUDED

#include "block/block.h"
#include "render/generate.h"
#include "world/world.h"
#include "block/builtin/air.h"
#include "block/builtin/dirt_block.h"
#include <cassert>

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{

class Plant : public BlockDescriptor
{
public:
    static Mesh makeDiagonalCrossMesh(TextureDescriptor td, ColorF c = colorizeIdentity())
    {
		const VectorF nxnynz = VectorF(0, 0, 0);
		const VectorF pxnynz = VectorF(1, 0, 0);
		const VectorF nxpynz = VectorF(0, 1, 0);
		const VectorF pxpynz = VectorF(1, 1, 0);
		const VectorF nxnypz = VectorF(0, 0, 1);
		const VectorF pxnypz = VectorF(1, 0, 1);
		const VectorF nxpypz = VectorF(0, 1, 1);
		const VectorF pxpypz = VectorF(1, 1, 1);
		Mesh retval = Generate::quadrilateral(td,
                                 pxnypz, c,
                                 nxnynz, c,
                                 nxpynz, c,
                                 pxpypz, c);
        retval.append(Generate::quadrilateral(td,
                                 nxnypz, c,
                                 pxnynz, c,
                                 pxpynz, c,
                                 nxpypz, c));
        retval.append(reverse(retval));
        return std::move(retval);
    }
    static Mesh makeGridMesh(TextureDescriptor td, float insetDistance = 3.0f / 16)
    {
		Mesh retval = transform(Matrix::scale(1 - 2 * insetDistance, 1, 1)
                                .concat(Matrix::translate(insetDistance, 0, 0)),
                                Generate::unitBox(td, td,
                                                  TextureDescriptor(), TextureDescriptor(),
                                                  TextureDescriptor(), TextureDescriptor()));
        retval.append(transform(Matrix::scale(1, 1, 1 - 2 * insetDistance)
                                .concat(Matrix::translate(0, 0, insetDistance)),
                                Generate::unitBox(TextureDescriptor(), TextureDescriptor(),
                                                  TextureDescriptor(), TextureDescriptor(),
                                                  td, td)));
        retval.append(reverse(retval));
        return std::move(retval);
    }
    const unsigned animationFrame, animationFrameCount;
    const float blockHeight;
protected:
    Plant(std::wstring name, Mesh mesh, unsigned animationFrame, unsigned animationFrameCount, float blockHeight)
        : BlockDescriptor(name, BlockShape(), LightProperties(), RayCasting::BlockCollisionMaskGround, true, false, false, false, false, false, false, mesh, Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), Mesh(), RenderLayer::Opaque), animationFrame(animationFrame), animationFrameCount(animationFrameCount), blockHeight(blockHeight)
    {
        assert(animationFrame < animationFrameCount);
    }
    virtual const Plant *getPlantFrame(unsigned frame) const = 0;
    virtual void dropItems(BlockIterator bi, Block b, World &world, WorldLockManager &lock_manager, Item tool) const = 0;
    virtual bool isPositionValid(BlockIterator bi, Block b, WorldLockManager &lock_manager) const = 0;
public:
    virtual RayCasting::Collision getRayCollision(const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, World &world, RayCasting::Ray ray) const override
    {
        if(ray.dimension() != blockIterator.position().d)
            return RayCasting::Collision(world);
        std::tuple<bool, float, BlockFace> collision = ray.getAABoxEnterFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1, blockHeight, 1));
        if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
        {
            collision = ray.getAABoxExitFace((VectorF)blockIterator.position(), (VectorF)blockIterator.position() + VectorF(1, blockHeight, 1));
            if(!std::get<0>(collision) || std::get<1>(collision) < RayCasting::Ray::eps)
                return RayCasting::Collision(world);
            std::get<1>(collision) = RayCasting::Ray::eps;
            return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), BlockFaceOrNone::None);
        }
        return RayCasting::Collision(world, std::get<1>(collision), blockIterator.position(), toBlockFaceOrNone(std::get<2>(collision)));
    }
    virtual bool isGroundBlock() const override
    {
        return false;
    }
    virtual bool canAttachBlock(Block b, BlockFace attachingFace, Block attachingBlock) const override
    {
        return false;
    }
    virtual float getHardness() const override
    {
        return 0;
    }
    virtual bool isReplaceableByFluid() const
    {
        return true;
    }
    virtual void onReplace(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager) const override
    {
        dropItems(bi, b, world, lock_manager, Item());
    }
    virtual void onBreak(World &world, Block b, BlockIterator bi, WorldLockManager &lock_manager, Item &tool) const override
    {
        dropItems(bi, b, world, lock_manager, tool);
        handleToolDamage(tool);
    }
    virtual void randomTick(const Block &block, World &world, BlockIterator blockIterator, WorldLockManager &lock_manager) const override
    {
        if(animationFrame + 1 >= animationFrameCount)
            return;
        Block newBlock = block;
        newBlock.descriptor = getPlantFrame(animationFrame + 1);
        assert(newBlock.good());
        world.setBlock(blockIterator, lock_manager, newBlock);
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return true;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return ToolLevel_None;
    }
    virtual Matrix getSelectionBoxTransform(const Block &block) const
    {
        return Matrix::scale(1, blockHeight, 1);
    }
    virtual void tick(BlockUpdateSet &blockUpdateSet, World &world, const Block &block, BlockIterator blockIterator, WorldLockManager &lock_manager, BlockUpdateKind kind) const
    {
        if(!isPositionValid(blockIterator, block, lock_manager))
        {
            dropItems(blockIterator, block, world, lock_manager, Item());
            blockUpdateSet.emplace_back(blockIterator.position(), Block(Air::descriptor()));
        }
    }
    virtual bool canPlace(Block b, BlockIterator bi, WorldLockManager &lock_manager) const
    {
        return isPositionValid(bi, b, lock_manager);
    }
};

}
}
}
}

#endif // BLOCK_PLANT_H_INCLUDED
