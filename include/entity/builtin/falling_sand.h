/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
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
#ifndef FALLING_SAND_H_INCLUDED
#define FALLING_SAND_H_INCLUDED

#include "entity/builtin/falling_block.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
class FallingSand final : public FallingBlock
{
    friend class global_instance_maker<FallingSand>;

protected:
    virtual Block getPlacedBlock() const override;
    virtual Item getDroppedItem() const override;

private:
    FallingSand();

public:
    static const FallingSand *pointer()
    {
        return global_instance_maker<FallingSand>::getInstance();
    }
    static EntityDescriptorPointer descriptor()
    {
        return pointer();
    }
};
class FallingGravel final : public FallingBlock
{
    friend class global_instance_maker<FallingGravel>;

protected:
    virtual Block getPlacedBlock() const override;
    virtual Item getDroppedItem() const override;

private:
    FallingGravel();

public:
    static const FallingGravel *pointer()
    {
        return global_instance_maker<FallingGravel>::getInstance();
    }
    static EntityDescriptorPointer descriptor()
    {
        return pointer();
    }
};
}
}
}
}

#endif // FALLING_SAND_H_INCLUDED
