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
#include "util/wood_descriptor.h"
#include "block/builtin/wood.h"
#include "entity/builtin/item.h"
#include "entity/builtin/items/wood.h"
#include "item/builtin/wood.h"

namespace programmerjake
{
namespace voxels
{
linked_map<std::wstring, WoodDescriptorPointer> *WoodDescriptors_t::pNameMap = nullptr;

namespace Woods
{
namespace builtin
{
SimpleWood::SimpleWood(std::wstring name, TextureDescriptor logTop, TextureDescriptor logSide, TextureDescriptor planks, TextureDescriptor sapling, TextureDescriptor leaves)
    : WoodDescriptor(name, logTop, logSide, planks, sapling, leaves)
{
    enum_array<BlockDescriptorPointer, LogOrientation> logBlockDescriptors;
    BlockDescriptorPointer planksBlockDescriptor = nullptr;
    BlockDescriptorPointer saplingBlockDescriptor = nullptr;
    BlockDescriptorPointer leavesBlockDescriptor = nullptr;
    ItemDescriptorPointer logItemDescriptor = nullptr;
    ItemDescriptorPointer planksItemDescriptor = nullptr;
    ItemDescriptorPointer saplingItemDescriptor = nullptr;
    ItemDescriptorPointer leavesItemDescriptor = nullptr;
    const Entities::builtin::EntityItem *logEntityDescriptor = nullptr;
    const Entities::builtin::EntityItem *planksEntityDescriptor = nullptr;
    const Entities::builtin::EntityItem *saplingEntityDescriptor = nullptr;
    const Entities::builtin::EntityItem *leavesEntityDescriptor = nullptr;
    for(LogOrientation logOrientation : enum_traits<LogOrientation>())
    {
        logBlockDescriptors[logOrientation] = new Blocks::builtin::WoodLog(this, logOrientation);
    }
    logEntityDescriptor = new Entities::builtin::items::WoodLog(this);
    logItemDescriptor = new Items::builtin::WoodLog(this, logBlockDescriptors[LogOrientation::Y], logEntityDescriptor);
    setDescriptors(logBlockDescriptors,
                   planksBlockDescriptor,
                   saplingBlockDescriptor,
                   leavesBlockDescriptor,
                   logItemDescriptor,
                   planksItemDescriptor,
                   saplingItemDescriptor,
                   leavesItemDescriptor,
                   logEntityDescriptor,
                   planksEntityDescriptor,
                   saplingEntityDescriptor,
                   leavesEntityDescriptor);
}
}
}
}
}
