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
#ifndef CHUNK_CACHE_H_INCLUDED
#define CHUNK_CACHE_H_INCLUDED

#include <cstdint>
#include <vector>
#include "util/position.h"
#include <cstdio>
#include <memory>
#include <unordered_map>
#include "util/lock.h"

namespace programmerjake
{
namespace voxels
{

namespace stream
{
class Reader;
class Writer;
}

class ChunkCache final
{
    ChunkCache(const ChunkCache &) = delete;
    ChunkCache &operator =(const ChunkCache &) = delete;
public:
    ChunkCache();
    ~ChunkCache();
    bool hasChunk(PositionI chunkBasePosition);
    void getChunk(PositionI chunkBasePosition, std::vector<std::uint8_t> &buffer);
    std::vector<std::uint8_t> getChunk(PositionI chunkBasePosition)
    {
        std::vector<std::uint8_t> retval;
        getChunk(chunkBasePosition, retval);
        return retval;
    }
    void setChunk(PositionI chunkBasePosition, const std::vector<std::uint8_t> &buffer);
private:
    static constexpr std::size_t NullChunk = 0;
    struct FileChunkState final
    {
        std::size_t usedSize = 0;
        std::size_t nextChunk = NullChunk; // chunk 0 is unused
    };
    void freeChunk(std::size_t chunkIndex);
    std::size_t allocChunk();
    std::size_t freeChunkListHead = NullChunk;
    std::shared_ptr<stream::Reader> reader;
    std::shared_ptr<stream::Writer> writer;
    std::unordered_map<PositionI, std::size_t> startingChunksMap;
    std::vector<FileChunkState> fileChunks;
    Mutex theLock;
};

}
}

#endif // CHUNK_CACHE_H_INCLUDED
