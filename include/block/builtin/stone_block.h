/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef STONE_BLOCK_H_INCLUDED
#define STONE_BLOCK_H_INCLUDED

#include "block/builtin/full_block.h"
#include "item/builtin/tools/tools.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class StoneBlock : public FullBlock
{
protected:
    StoneBlock(std::wstring name, TextureDescriptor td, LightProperties lightProperties = LightProperties(Lighting(), Lighting::makeMaxLight()))
        : FullBlock(name, lightProperties, RayCasting::BlockCollisionMaskGround, true, td)
    {
    }
    virtual bool isHelpingToolKind(Item tool) const override
    {
        return dynamic_cast<const Items::builtin::tools::Pickaxe *>(tool.descriptor) != nullptr;
    }
};
}
}
}
}

#endif // STONE_BLOCK_H_INCLUDED
