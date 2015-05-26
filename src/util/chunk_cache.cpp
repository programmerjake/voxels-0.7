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
#include "util/chunk_cache.h"
#include "stream/stream.h"
#include "platform/platform.h"
#include <cassert>

namespace programmerjake
{
namespace voxels
{
namespace
{
constexpr std::uint64_t chunkSize = 1 << 10;
}

ChunkCache::ChunkCache()
    : reader(),
    writer(),
    startingChunksMap(),
    fileChunks(),
    theLock()
{
    static_assert(NullChunk == 0, "invalid value for NullChunk");
    fileChunks.resize(1); // fileChunks 0 is unused because it is NullChunk
    auto p = createTemporaryFile();
    reader = std::get<0>(p);
    writer = std::get<1>(p);
}

ChunkCache::~ChunkCache()
{
}

bool ChunkCache::hasChunk(PositionI chunkBasePosition)
{
    std::unique_lock<std::mutex> lockIt(theLock);
    return startingChunksMap.count(chunkBasePosition) != 0;
}

void ChunkCache::getChunk(PositionI chunkBasePosition, std::vector<std::uint8_t> &buffer)
{
    try
    {
        std::unique_lock<std::mutex> lockIt(theLock);
        auto iter = startingChunksMap.find(chunkBasePosition);
        if(iter == startingChunksMap.end())
            throw stream::IOException("ChunkCache: chunk not found");
        buffer.clear();
        std::size_t chunkIndex = std::get<1>(*iter);
        for(; chunkIndex != NullChunk; chunkIndex = fileChunks[chunkIndex].nextChunk)
        {
            static_assert(NullChunk == 0, "invalid value for NullChunk");
            std::int64_t filePosition = static_cast<std::int64_t>(chunkIndex - 1) * chunkSize; // skip null chunk
            assert(fileChunks[chunkIndex].nextChunk == NullChunk || fileChunks[chunkIndex].usedSize == chunkSize);
            assert(fileChunks[chunkIndex].usedSize > 0);
            reader->seek(filePosition, stream::SeekPosition::Start);
            std::size_t bufferWriteStartPosition = buffer.size();
            buffer.resize(bufferWriteStartPosition + fileChunks[chunkIndex].usedSize);
            reader->readAllBytes(&buffer[bufferWriteStartPosition], fileChunks[chunkIndex].usedSize);
        }
    }
    catch(stream::EOFException &)
    {
        throw stream::IOException("ChunkCache: file too short");
    }
}

void ChunkCache::setChunk(PositionI chunkBasePosition, const std::vector<std::uint8_t> &buffer)
{
    std::unique_lock<std::mutex> lockIt(theLock);
    static_assert(NullChunk == std::size_t(), "invalid value for NullChunk");
    std::size_t &startingChunkIndex = startingChunksMap[chunkBasePosition];
    std::size_t prevChunkIndex = NullChunk;
    std::size_t bufferPos = 0;
    while(bufferPos < buffer.size())
    {
        std::size_t chunkIndex;
        if(prevChunkIndex == NullChunk)
            chunkIndex = startingChunkIndex;
        else
            chunkIndex = fileChunks[prevChunkIndex].nextChunk;
        if(chunkIndex == NullChunk)
        {
            chunkIndex = allocChunk();
            if(prevChunkIndex == NullChunk)
                startingChunkIndex = chunkIndex;
            else
                fileChunks[prevChunkIndex].nextChunk = chunkIndex;
        }
        prevChunkIndex = chunkIndex;
        std::size_t currentChunkSize = chunkSize;
        if(currentChunkSize > buffer.size() - bufferPos)
            currentChunkSize = buffer.size() - bufferPos;
        fileChunks[chunkIndex].usedSize = currentChunkSize;
        bufferPos += currentChunkSize;
    }
    std::size_t freeChunkIndex;
    if(prevChunkIndex == NullChunk)
    {
        freeChunkIndex = startingChunkIndex;
        startingChunkIndex = NullChunk;
    }
    else
    {
        freeChunkIndex = fileChunks[prevChunkIndex].nextChunk;
        fileChunks[prevChunkIndex].nextChunk = NullChunk;
    }
    while(freeChunkIndex != NullChunk)
    {
        std::size_t nextChunkIndex = fileChunks[freeChunkIndex].nextChunk;
        freeChunk(freeChunkIndex);
        freeChunkIndex = nextChunkIndex;
    }
    bufferPos = 0;
    std::size_t chunkIndex = startingChunkIndex;
    while(bufferPos < buffer.size())
    {
        assert(chunkIndex != NullChunk);
        std::size_t currentChunkSize = chunkSize;
        if(currentChunkSize > buffer.size() - bufferPos)
            currentChunkSize = buffer.size() - bufferPos;
        fileChunks[chunkIndex].usedSize = currentChunkSize;
        try
        {
            static_assert(NullChunk == 0, "invalid value for NullChunk");
            std::int64_t filePosition = static_cast<std::int64_t>(chunkIndex - 1) * chunkSize; // skip null chunk
            writer->seek(filePosition, stream::SeekPosition::Start);
            writer->writeBytes(&buffer[bufferPos], currentChunkSize);
        }
        catch(stream::IOException &e)
        {
            try
            {
                writer->flush();
            }
            catch(stream::IOException &)
            {
            }
            throw e;
        }
        bufferPos += currentChunkSize;
        chunkIndex = fileChunks[chunkIndex].nextChunk;
    }
    assert(chunkIndex == NullChunk);
    writer->flush();
}

void ChunkCache::freeChunk(std::size_t chunkIndex)
{
    if(chunkIndex == NullChunk)
        return;
    fileChunks[chunkIndex].usedSize = 0;
    fileChunks[chunkIndex].nextChunk = freeChunkListHead;
    freeChunkListHead = chunkIndex;
}

std::size_t ChunkCache::allocChunk()
{
    if(freeChunkListHead != NullChunk)
    {
        std::size_t retval = freeChunkListHead;
        freeChunkListHead = fileChunks[retval].nextChunk;
        fileChunks[retval].usedSize = 0;
        fileChunks[retval].nextChunk = NullChunk;
        return retval;
    }
    std::size_t retval = fileChunks.size();
    fileChunks.emplace_back();
    fileChunks[retval].usedSize = 0;
    fileChunks[retval].nextChunk = NullChunk;
    return retval;
}

}
}
