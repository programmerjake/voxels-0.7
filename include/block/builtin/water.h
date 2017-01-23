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
#ifndef WATER_H_INCLUDED
#define WATER_H_INCLUDED

#include "block/builtin/fluid.h"
#include "texture/texture_atlas.h"
#include "util/global_instance_maker.h"
#include "item/builtin/bucket.h"

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
        WaterConstructor() : descriptors()
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
        : Fluid(L"builtin.water",
                newLevel,
                falling,
                LightProperties(Lighting(0, 0, 0), Lighting(2, 3, 3)),
                TextureAtlas::WaterSide0.td(),
                BlockUpdateKind::Water)
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
    virtual Item getFilledBucket() const override
    {
        return Item(Items::builtin::WaterBucket::descriptor());
    }

protected:
    virtual const Fluid *getBlockDescriptorForFluidLevel(unsigned newLevel,
                                                         bool falling) const override
    {
        return global_instance_maker<WaterConstructor>::getInstance()
            ->descriptors[falling ? 1 : 0][newLevel];
    }
    virtual bool isSameKind(const Fluid *other) const override
    {
        return dynamic_cast<const Water *>(other) != nullptr;
    }
    virtual ColorF getColorizeColor(BlockIterator bi, WorldLockManager &lock_manager) const override
    {
        return bi.getBiomeProperties(lock_manager).getWaterColor();
    }
    virtual void writeBlockData(stream::Writer &writer,
                                BlockDataPointer<BlockData> data) const override
    {
    }
    virtual BlockDataPointer<BlockData> readBlockData(stream::Reader &reader) const override
    {
        return nullptr;
    }
};
}
}
}
}

#endif // WATER_H_INCLUDED
