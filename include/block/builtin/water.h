#ifndef WATER_H_INCLUDED
#define WATER_H_INCLUDED

#include "block/builtin/fluid.h"
#include "texture/texture_atlas.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class Water : public Fluid
{
private:
    struct WaterConstructor
    {
        friend class global_instance_maker<WaterConstructor>;
        checked_array<checked_array<Water *, 8>, 2> descriptors;
        WaterConstructor()
        {
            for(int falling = 0; falling <= 1; falling++)
            {
                for(unsigned newLevel = 0; newLevel < descriptors[0].size(); newLevel++)
                {
                    descriptors[falling][newLevel] = new Water(newLevel, falling != 0);
                }
            }
        }
        ~WaterConstructor()
        {
            for(auto &v : descriptors)
            {
                for(Water *descriptor : v)
                {
                    delete descriptor;
                }
            }
        }
    };
    Water(unsigned newLevel = 0, bool falling = false)
        : Fluid(L"builtin.water", newLevel, falling, LightProperties(Lighting(0, 0, 0), Lighting(2, 3, 3)), TextureAtlas::WaterSide0.td())
    {
    }
public:
    static const Water *pointer()
    {
        return global_instance_maker<WaterConstructor>::getInstance()->descriptors[0][0];
    }
    static BlockDescriptorPointer descriptor()
    {
        return pointer();
    }
protected:
    virtual const Fluid *getBlockDescriptorForFluidLevel(unsigned newLevel, bool falling) const override
    {
        return global_instance_maker<WaterConstructor>::getInstance()->descriptors[falling ? 1 : 0][newLevel];
    }
    virtual bool isSameKind(const Fluid *other) const override
    {
        return dynamic_cast<const Water *>(other) != nullptr;
    }
    virtual ColorF getColorizeColor(BlockIterator bi, WorldLockManager &lock_manager) const override
    {
        return bi.getBiomeProperties(lock_manager).getWaterColor();
    }
};
}
}
}
}

#endif // WATER_H_INCLUDED
