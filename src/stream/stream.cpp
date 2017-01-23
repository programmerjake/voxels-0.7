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
#if defined(_WIN64) || defined(_WIN32)
#ifdef __MSVCRT_VERSION__
#define FILE_SEEK _fseeki64
#define FILE_TELL _ftelli64
#else
#define FILE_SEEK fseek
#define FILE_TELL ftell
#endif
#define FILE_OPEN fopen
#elif defined(__ANDROID__) || defined(__APPLE__)
#define FILE_SEEK fseeko
#define FILE_TELL ftello
#define FILE_OPEN fopen
#elif defined(__linux)
#define FILE_SEEK fseeko64
#define FILE_TELL ftello64
#define FILE_OPEN fopen64
#elif defined(__unix) || defined(__posix)
#define FILE_SEEK fseeko
#define FILE_TELL ftello
#define FILE_OPEN fopen
#else
#define FILE_SEEK fseek
#define FILE_TELL ftell
#define FILE_OPEN fopen
#endif

#include "stream/stream.h"
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cstdlib>
#include "util/util.h"
#include <stdio.h>
#include <sys/types.h>

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace stream
{
namespace
{
const size_t bufferSize = 1 << 15;

struct Pipe
{
    mutex lock;
    condition_variable_any cond;
    bool closed = false;
    int readerBufferIndex = 0;
    int writerBufferIndex() const
    {
        return 1 - readerBufferIndex;
    }
    bool writerReadyToSwapBuffers = false;
    bool canWrite = true;
    size_t readerPosition = 0;
    size_t bufferSizes[2];
    uint8_t buffers[2][bufferSize];
    Pipe() : lock(), cond(), bufferSizes{0, 0}
    {
    }
};

class PipeReader final : public Reader
{
private:
    shared_ptr<Pipe> pipe;

public:
    PipeReader(shared_ptr<Pipe> pipe) : pipe(pipe)
    {
    }
    virtual ~PipeReader()
    {
        pipe->lock.lock();
        pipe->closed = true;
        pipe->cond.notify_all();
        pipe->lock.unlock();
    }
    virtual uint8_t readByte() override
    {
        assert(pipe->readerBufferIndex >= 0 && pipe->readerBufferIndex <= 1);
        if(pipe->readerPosition < pipe->bufferSizes[pipe->readerBufferIndex])
        {
            return pipe->buffers[pipe->readerBufferIndex][pipe->readerPosition++];
        }
        pipe->lock.lock();
        assert(pipe->readerBufferIndex >= 0 && pipe->readerBufferIndex <= 1);
        pipe->bufferSizes[pipe->readerBufferIndex] = 0;
        pipe->readerPosition = 0;
        while(!pipe->writerReadyToSwapBuffers)
        {
            if(pipe->closed)
            {
                pipe->lock.unlock();
                throw EOFException();
            }
            pipe->cond.wait(pipe->lock);
            assert(pipe->readerBufferIndex >= 0 && pipe->readerBufferIndex <= 1);
        }
        pipe->readerBufferIndex = pipe->writerBufferIndex();
        assert(pipe->readerBufferIndex >= 0 && pipe->readerBufferIndex <= 1);
        assert(pipe->bufferSizes[pipe->readerBufferIndex] > 0);
        uint8_t retval = pipe->buffers[pipe->readerBufferIndex][pipe->readerPosition++];
        pipe->writerReadyToSwapBuffers = false;
        pipe->cond.notify_all();
        pipe->lock.unlock();
        return retval;
    }
    virtual bool dataAvailable() override
    {
        return pipe->readerPosition < pipe->bufferSizes[pipe->readerBufferIndex];
    }
};

class PipeWriter final : public Writer
{
private:
    shared_ptr<Pipe> pipe;

public:
    PipeWriter(shared_ptr<Pipe> pipe) : pipe(pipe)
    {
    }

    virtual ~PipeWriter()
    {
        pipe->lock.lock();
        pipe->closed = true;
        pipe->cond.notify_all();
        pipe->lock.unlock();
    }

    virtual void writeByte(uint8_t v) override
    {
        while(!pipe->canWrite || pipe->bufferSizes[pipe->writerBufferIndex()] >= bufferSize)
        {
            if(!pipe->canWrite)
            {
                pipe->lock.lock();
                while(pipe->writerReadyToSwapBuffers)
                {
                    if(pipe->closed)
                    {
                        pipe->lock.unlock();
                        throw IOException("can't write to closed pipe");
                    }
                    pipe->cond.wait(pipe->lock);
                }
                pipe->lock.unlock();
                pipe->canWrite = true;
            }
            else
                flush();
        }
        pipe->buffers[pipe->writerBufferIndex()][pipe->bufferSizes[pipe->writerBufferIndex()]++] =
            v;
    }

    virtual void flush() override
    {
        if(!pipe->canWrite)
            return;
        if(pipe->bufferSizes[pipe->writerBufferIndex()] == 0)
            return;
        pipe->lock.lock();
        if(pipe->closed)
        {
            pipe->lock.unlock();
            throw IOException("can't write to closed pipe");
        }
        pipe->writerReadyToSwapBuffers = true;
        pipe->canWrite = false;
        pipe->cond.notify_all();
        pipe->lock.unlock();
    }

    virtual bool writeWaits() override
    {
        return pipe->bufferSizes[pipe->writerBufferIndex()] < bufferSize && pipe->canWrite;
    }
};
}

StreamPipe::StreamPipe() : readerInternal(), writerInternal()
{
    shared_ptr<Pipe> pipe = make_shared<Pipe>();
    readerInternal = shared_ptr<Reader>(new PipeReader(pipe));
    writerInternal = shared_ptr<Writer>(new PipeWriter(pipe));
}

uint8_t DumpingReader::readByte()
{
    uint8_t retval = reader.readByte();
    cerr << "Read Byte : " << (unsigned)retval << "\n";
    return retval;
}

using namespace std;

FILE *FileReader::openFile(std::wstring fileName, bool forWriteToo)
{
    std::string str = string_cast<std::string>(std::move(fileName));
    errno = 0;
    MSVC_PRAGMA(warning(suppress : 4996))
    FILE *f = FILE_OPEN(str.c_str(), forWriteToo ? "r+b" : "rb");
    if(f == nullptr)
        IOException::throwErrorFromErrno("fopen");
    return f;
}

std::int64_t FileReader::tell()
{
    errno = 0;
    std::int64_t retval = FILE_TELL(f);
    if(retval < 0)
        IOException::throwErrorFromErrno("ftell");
    return retval;
}

void FileReader::seek(std::int64_t offset, SeekPosition seekPosition)
{
    int whence;
    switch(seekPosition)
    {
    case SeekPosition::Start:
        whence = SEEK_SET;
        break;
    case SeekPosition::Current:
        whence = SEEK_CUR;
        break;
    case SeekPosition::End:
        whence = SEEK_END;
        break;
    default:
        UNREACHABLE();
    }
    errno = 0;
    MSVC_PRAGMA(warning(suppress : 4244))
    if(0 != FILE_SEEK(f, offset, whence))
        IOException::throwErrorFromErrno("fseek");
}

std::size_t FileReader::readBytes(std::uint8_t *array, std::size_t maxCount)
{
    if(maxCount == 0)
        return 0;
    errno = 0;
    std::size_t retval = fread(static_cast<void *>(array), sizeof(std::uint8_t), maxCount, f);
    if(retval < maxCount)
    {
        if(ferror(f))
            IOException::throwErrorFromErrno("fread");
    }
    return retval;
}

FILE *FileWriter::openFile(std::wstring fileName, bool forReadToo)
{
    std::string str = string_cast<std::string>(std::move(fileName));
    MSVC_PRAGMA(warning(suppress : 4996))
    FILE *f = FILE_OPEN(str.c_str(), forReadToo ? "w+b" : "wb");
    if(f == nullptr)
        IOException::throwErrorFromErrno("fopen");
    return f;
}

void FileWriter::writeBytes(const std::uint8_t *array, std::size_t count)
{
    errno = 0;
    fwrite(static_cast<const void *>(array), sizeof(std::uint8_t), count, f);
    if(ferror(f))
    {
        IOException::throwErrorFromErrno("fwrite");
    }
}

std::int64_t FileWriter::tell()
{
    errno = 0;
    std::int64_t retval = FILE_TELL(f);
    if(retval < 0)
        IOException::throwErrorFromErrno("ftell");
    return retval;
}

void FileWriter::seek(std::int64_t offset, SeekPosition seekPosition)
{
    int whence;
    switch(seekPosition)
    {
    case SeekPosition::Start:
        whence = SEEK_SET;
        break;
    case SeekPosition::Current:
        whence = SEEK_CUR;
        break;
    case SeekPosition::End:
        whence = SEEK_END;
        break;
    default:
        UNREACHABLE();
    }
    errno = 0;
    MSVC_PRAGMA(warning(suppress : 4244))
    if(0 != FILE_SEEK(f, offset, whence))
        IOException::throwErrorFromErrno("fseek");
}

#if 0
namespace
{
void readIt(shared_ptr<Reader> preader)
{
    try
    {
        while(true)
        {
            cout << hex << (int)preader->readByte() << endl;
        }
    }
    catch(IOException & e)
    {
        cout << "Error : " << e.what() << endl;
    }
}

initializer init1([]()
{
    shared_ptr<Writer> pwriter;
    thread readerThread;
    {
        StreamPipe pipe;
        readerThread = thread(readIt, pipe.preader());
        pwriter = pipe.pwriter();
    }
    for(int i = 0; i < 0x100; i++)
    {
        pwriter->writeByte(i);
    }
    pwriter->flush();
    readerThread.join();
    exit(0);
});
}
#endif // 1
}
}
}
