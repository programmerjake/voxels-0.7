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
#ifndef STREAM_PARALLEL_H_INCLUDED
#define STREAM_PARALLEL_H_INCLUDED

#include "stream/stream.h"
#include <memory>
#include "util/util.h"
#include "util/lock.h"
#include <mutex>
#include <vector>
#include <condition_variable>

namespace programmerjake
{
namespace voxels
{
namespace stream
{
struct NonResizableException final : public IOException
{
    NonResizableException() : IOException("IO Error : non-resizable")
    {
    }
};

struct ReadOnlyException final : public IOException
{
    ReadOnlyException() : IOException("IO Error : write attempted on read-only data")
    {
    }
};

struct WriteOnlyException final : public IOException
{
    WriteOnlyException() : IOException("IO Error : read attempted on write-only data")
    {
    }
};

struct ReadPastEndException final : public IOException
{
    ReadPastEndException() : IOException("IO Error : read attempted past end of data")
    {
    }
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
GCC_PRAGMA(diagnostic ignored "-Wnon-virtual-dtor")
class Parallel : public std::enable_shared_from_this<Parallel>
{
    GCC_PRAGMA(diagnostic pop)
    Parallel(const Parallel &) = delete;
    Parallel &operator=(const Parallel &) = delete;

public:
    Parallel() = default;
    virtual ~Parallel() = default;
    virtual std::uint64_t size() = 0;
    /**
     * resize this Parallel
     * @param newSize the new size
     * @throw NonResizableException if resizing is not supported
     * @throw IOException if resize failed for some other reason
     */
    virtual void resize(std::uint64_t newSize) = 0;
    /**
     * lock a section for reading and return a Reader for the section
     * @param sectionStart the section start
     * @param sectionSize the section size
     * @return the new Reader for the locked section
     * @throw WriteOnlyException if reading is not supported
     * @throw ReadPastEndException if the specified section is past the end of this Parallel
     * @throw IOException if readSection failed for some other reason
     * @note The returned Reader is relative to the start of
     * this section, not the start of this Parallel. For instance,
     * calling <code>Reader::seek(0, SeekPosition::Start)</code>
     * would seek the Reader to the offset <code>start</code> in
     * this Parallel, instead of 0.
     * @warning The returned Reader should be able to be used
     * from a different thread than the calling thread, though
     * only by one thread at a time.
     */
    virtual std::shared_ptr<Reader> readSection(std::uint64_t sectionStart,
                                                std::uint64_t sectionSize) = 0;
    /**
     * lock a section for writing and return a Writer for the section.
     * Tries to expand this Parallel if the specified section is past the end of this Parallel.
     * @param sectionStart the section start
     * @param sectionSize the section size
     * @return the new Writer for the locked section
     * @throw ReadOnlyException if writing is not supported
     * @throw NonResizableException if resizing is not supported
     * @throw IOException if writeSection failed for some other reason
     * @note The returned Writer is relative to the start of
     * this section, not the start of this Parallel. For instance,
     * calling <code>Writer::seek(0, SeekPosition::Start)</code>
     * would seek the Writer to the offset <code>start</code> in
     * this Parallel, instead of 0.
     * @warning The returned Writer should be able to be used
     * from a different thread than the calling thread, though
     * only by one thread at a time.
     */
    virtual std::shared_ptr<Writer> writeSection(std::uint64_t sectionStart,
                                                 std::uint64_t sectionSize) = 0;
    /**
     * lock a section for reading and writing and return a StreamRW for the section
     * Tries to expand this Parallel if the specified section is past the end of this Parallel.
     * @param sectionStart the section start
     * @param sectionSize the section size
     * @return the new StreamRW for the locked section
     * @throw ReadOnlyException if writing is not supported
     * @throw WriteOnlyException if reading is not supported
     * @throw NonResizableException if resizing is not supported
     * @throw IOException if readWriteSection failed for some other reason
     * @note The returned streams are relative to the start of
     * this section, not the start of this Parallel. For instance,
     * calling <code>Reader::seek(0, SeekPosition::Start)</code>
     * would seek the Reader to the offset <code>start</code> in
     * this Parallel, instead of 0.
     * @warning The returned streams should be able to be used
     * from a different thread than the calling thread, though
     * only by one thread at a time.
     */
    virtual std::shared_ptr<StreamRW> readWriteSection(std::uint64_t sectionStart,
                                                       std::uint64_t sectionSize) = 0;
};

class ParallelMemory final : public Parallel
{
private:
    const std::shared_ptr<Parallel> implementation;

public:
    ParallelMemory(std::size_t size);
    virtual std::uint64_t size() override
    {
        return implementation->size();
    }
    virtual void resize(std::uint64_t newSize) override
    {
        implementation->resize(newSize);
    }
    virtual std::shared_ptr<Reader> readSection(std::uint64_t sectionStart,
                                                std::uint64_t sectionSize) override
    {
        return implementation->readSection(sectionStart, sectionSize);
    }
    virtual std::shared_ptr<Writer> writeSection(std::uint64_t sectionStart,
                                                 std::uint64_t sectionSize) override
    {
        return implementation->writeSection(sectionStart, sectionSize);
    }
    virtual std::shared_ptr<StreamRW> readWriteSection(std::uint64_t sectionStart,
                                                       std::uint64_t sectionSize) override
    {
        return implementation->readWriteSection(std::uint64_t sectionStart,
                                                std::uint64_t sectionSize);
    }
};

class SerializedParallel : public Parallel
{
private:
    std::recursive_mutex globalLock;
    std::condition_variable_any globalCond;
    struct Region final
    {
        std::uint64_t start;
        std::uint64_t size;
        bool isReadOnly;
        Region(std::uint64_t start, std::uint64_t size, bool isReadOnly)
            : start(start), size(size), isReadOnly(isReadOnly)
        {
            assert(start + size >= start); // assert not wrapping around
        }
        bool intersects(const Region &rt) const
        {
            if(isReadOnly && rt.isReadOnly)
                return false;
            if(start >= rt.start + rt.size)
                return false;
            if(rt.start >= start + size)
                return false;
            return true;
        }
    };
    std::vector<std::weak_ptr<const Region>> lockedRegions; // the regions that are currently locked
    std::vector<std::shared_ptr<const Region>>
        waitingRegions; // the regions that are waiting to be locked
    std::uint64_t publicSize;
    bool settingPublicSize;
    std::size_t waitingOnPublicSizeCount = 0;
    class MyReader final : public Reader
    {
#error finish
    };

protected:
    std::uint64_t getRequiredSize(std::unique_lock<std::mutex> &theGlobalLock)
    {
        ignore_unused_variable_warning(theGlobalLock);
        std::uint64_t requiredSize = 0;
        for(auto i = lockedRegions.begin(); i != lockedRegions.end();)
        {
            std::shared_ptr<const Region> region = i->lock();
            if(region == nullptr)
            {
                i = lockedRegions.erase(i);
                continue;
            }
            else
                ++i;
            std::uint64_t currentRequiredSize = region->start + region->size;
            if(currentRequiredSize > requiredSize)
                requiredSize = currentRequiredSize;
        }
        for(const std::shared_ptr<const Region> &region : waitingRegions)
        {
            std::uint64_t currentRequiredSize = region->start + region->size;
            if(currentRequiredSize > requiredSize)
                requiredSize = currentRequiredSize;
        }
        return requiredSize;
    }
    bool trySetPublicSize(std::uint64_t newSize, std::unique_lock<std::mutex> &theGlobalLock)
    {
        if(newSize >= getRequiredSize(theGlobalLock))
        {
            publicSize = newSize;
            return true;
        }
        return false;
    }
    void setPublicSize(std::uint64_t newSize, std::unique_lock<std::mutex> &theGlobalLock)
    {
        settingPublicSize = true;
        try
        {
            while(!trySetPublicSize(newSize, theGlobalLock))
            {
                globalCond.wait(theGlobalLock);
            }
        }
        catch(...)
        {
            settingPublicSize = false;
            throw;
        }
        settingPublicSize = false;
    }
    virtual void internalResize(std::uint64_t newSize,
                                std::unique_lock<std::mutex> &theGlobalLock) = 0;
    virtual void checkReadSectionImplemented(std::uint64_t sectionStart,
                                             std::uint64_t sectionSize) const
    {
    }
    virtual void checkWriteSectionImplemented(std::uint64_t sectionStart,
                                              std::uint64_t sectionSize) const
    {
    }
    virtual void checkResizeImplemented(std::uint64_t newSize)
    {
    }
    virtual std::size_t internalReadSection(std::uint8_t *data,
                                            std::uint64_t start,
                                            std::size_t count,
                                            std::unique_lock<std::mutex> &theGlobalLock) = 0;
    virtual std::size_t internalWriteSection(const std::uint8_t *data,
                                             std::uint64_t start,
                                             std::size_t count,
                                             std::unique_lock<std::mutex> &theGlobalLock) = 0;

public:
    explicit SerializedParallel(std::uint64_t initialSize) : globalLock(), publicSize(initialSize)
    {
    }
    virtual std::uint64_t size() override final
    {
        std::unique_lock<std::mutex> lockIt(globalLock);
        return publicSize;
    }
    virtual void resize(std::uint64_t newSize)
    {
        std::unique_lock<std::mutex> lockIt(globalLock);
        internalResize(newSize, lockIt);
    }
    virtual std::shared_ptr<Reader> readSection(std::uint64_t sectionStart,
                                                std::uint64_t sectionSize)
    {
        std::unique_lock<std::mutex> lockIt(globalLock);
        checkReadSectionImplemented(sectionStart, sectionSize);
    }
    virtual std::shared_ptr<Writer> writeSection(std::uint64_t sectionStart,
                                                 std::uint64_t sectionSize)
    {
        return implementation->writeSection(sectionStart, sectionSize);
    }
    virtual std::shared_ptr<StreamRW> readWriteSection(std::uint64_t sectionStart,
                                                       std::uint64_t sectionSize)
    {
        return implementation->readWriteSection(std::uint64_t sectionStart,
                                                std::uint64_t sectionSize);
    }
};

class ParallelFile final : public Parallel
{
private:
    const std::shared_ptr<Parallel> implementation;

public:
    ParallelFile(std::wstring fileName);
    virtual std::uint64_t size() override
    {
        return implementation->size();
    }
    virtual void resize(std::uint64_t newSize) override
    {
        implementation->resize(newSize);
    }
    virtual std::shared_ptr<Reader> readSection(std::uint64_t sectionStart,
                                                std::uint64_t sectionSize) override
    {
        return implementation->readSection(sectionStart, sectionSize);
    }
    virtual std::shared_ptr<Writer> writeSection(std::uint64_t sectionStart,
                                                 std::uint64_t sectionSize) override
    {
        return implementation->writeSection(sectionStart, sectionSize);
    }
    virtual std::shared_ptr<StreamRW> readWriteSection(std::uint64_t sectionStart,
                                                       std::uint64_t sectionSize) override
    {
        return implementation->readWriteSection(std::uint64_t sectionStart,
                                                std::uint64_t sectionSize);
    }
};
}
}
}

#endif // STREAM_PARALLEL_H_INCLUDED
