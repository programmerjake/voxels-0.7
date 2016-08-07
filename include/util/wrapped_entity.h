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
#include "util/block_chunk.h"
#ifndef WRAPPED_ENTITY_H_INCLUDED
#define WRAPPED_ENTITY_H_INCLUDED

#include "util/world_lock_manager.h"
#include "util/intrusive_list.h"
#include "entity/entity_struct.h"
#include "util/object_counter.h"
#include <mutex>

namespace programmerjake
{
namespace voxels
{
struct WrappedEntity final
{
    WrappedEntity(const WrappedEntity &) = delete;
    WrappedEntity &operator =(const WrappedEntity &) = delete;
    ObjectCounter<WrappedEntity, 0> objectCounter;
    Entity entity;
    BlockChunk *currentChunk = nullptr;
    std::uint64_t lastEntityRunCount = 0;
    WrappedEntity()
        : objectCounter(), entity()
    {
    }
    WrappedEntity(Entity e)
        : objectCounter(), entity(e)
    {
    }
    void verify() const; // in block_chunk.cpp
};
}
}

#endif // WRAPPED_ENTITY_H_INCLUDED
