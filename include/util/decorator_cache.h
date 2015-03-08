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
#ifndef DECORATOR_CACHE_H_INCLUDED
#define DECORATOR_CACHE_H_INCLUDED

#include "generate/decorator.h"
#include "util/block_chunk.h"
#include <unordered_map>
#include <vector>
#include <cassert>
#include <tuple>
#include <list>

namespace programmerjake
{
namespace voxels
{
class DecoratorCache final
{
private:
    struct ChunkKey final
    {
        PositionI position;
        DecoratorDescriptorPointer descriptor = nullptr;
        ChunkKey()
        {
        }
        ChunkKey(PositionI position, DecoratorDescriptorPointer descriptor)
            : position(position), descriptor(descriptor)
        {
        }
        bool empty() const
        {
            return descriptor == nullptr;
        }
    };
    struct ChunkKeyEqual final
    {
        bool operator()(const ChunkKey &l, const ChunkKey &r) const
        {
            return l.position == r.position && l.descriptor == r.descriptor;
        }
    };
    struct ChunkKeyHash final
    {
        std::size_t operator()(const ChunkKey &v) const
        {
            return std::hash<PositionI>()(v.position) + std::hash<DecoratorDescriptorPointer>()(v.descriptor);
        }
    };
    struct Chunk final
    {
        ChunkKey key;
        std::vector<std::shared_ptr<const DecoratorInstance>> decorators;
        bool empty = true;
        std::list<ChunkKey>::iterator chunksListIterator;
    };
    std::unordered_map<ChunkKey, Chunk, ChunkKeyHash, ChunkKeyEqual> chunks;
    std::list<ChunkKey> chunksList;
    void removeChunk()
    {
        chunks.erase(chunksList.back());
        chunksList.pop_back();
    }
    Chunk &getChunk(ChunkKey key)
    {
        if(chunks.size() >= 50 * DecoratorDescriptors.size())
        {
            removeChunk();
        }
        Chunk &retval = chunks[key];
        if(retval.key.empty())
        {
            retval.key = key;
            retval.chunksListIterator = chunksList.insert(chunksList.begin(), key);
        }
        else
        {
            chunksList.splice(chunksList.begin(), chunksList, retval.chunksListIterator);
        }
        return retval;
    }
public:
    class InstanceRange final
    {
    public:
        typedef typename std::vector<std::shared_ptr<const DecoratorInstance>>::const_iterator iterator;
    private:
        iterator beginValue, endValue;
    public:
        InstanceRange(iterator beginValue, iterator endValue)
            : beginValue(beginValue), endValue(endValue)
        {
        }
        iterator begin() const
        {
            return beginValue;
        }
        iterator end() const
        {
            return endValue;
        }
    };
    std::pair<InstanceRange, bool> getChunkInstances(PositionI chunkBasePosition, DecoratorDescriptorPointer descriptor)
    {
        assert(descriptor != nullptr);
        Chunk &c = getChunk(ChunkKey(chunkBasePosition, descriptor));
        return std::pair<InstanceRange, bool>(InstanceRange(c.decorators.begin(), c.decorators.end()), !c.empty);
    }
    void setChunkInstances(PositionI chunkBasePosition, DecoratorDescriptorPointer descriptor, std::vector<std::shared_ptr<const DecoratorInstance>> newDecorators)
    {
        assert(descriptor != nullptr);
        Chunk &c = getChunk(ChunkKey(chunkBasePosition, descriptor));
        c.decorators = std::move(newDecorators);
        c.empty = false;
    }
};
}
}

#endif // DECORATOR_CACHE_H_INCLUDED
