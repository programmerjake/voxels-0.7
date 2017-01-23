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
#ifndef DIRT_VEIN_H_INCLUDED
#define DIRT_VEIN_H_INCLUDED

#include "generate/decorator/mineral_vein.h"
#include "block/builtin/dirt.h"
#include "block/builtin/sand.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Decorators
{
namespace builtin
{
class DirtVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<DirtVeinDecorator>;

private:
    DirtVeinDecorator() : MineralVeinDecorator(L"builtin.dirt_vein", 33, 0, 256, 10, 900)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::Dirt::descriptor());
    }

public:
    static const DirtVeinDecorator *pointer()
    {
        return global_instance_maker<DirtVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};
class GravelVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<GravelVeinDecorator>;

private:
    GravelVeinDecorator() : MineralVeinDecorator(L"builtin.gravel_vein", 33, 0, 256, 8, 900)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::Gravel::descriptor());
    }

public:
    static const GravelVeinDecorator *pointer()
    {
        return global_instance_maker<GravelVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};
}
}
}
}

#endif // DIRT_VEIN_H_INCLUDED
