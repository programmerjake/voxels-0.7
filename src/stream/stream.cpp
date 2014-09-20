/*
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
#include "stream/stream.h"
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cstdlib>
#include "util/util.h"

using namespace std;

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
    Pipe()
        : bufferSizes{0, 0}
    {
    }
};

class PipeReader final : public Reader
{
private:
    shared_ptr<Pipe> pipe;
public:
    PipeReader(shared_ptr<Pipe> pipe)
        : pipe(pipe)
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
    PipeWriter(shared_ptr<Pipe> pipe)
        : pipe(pipe)
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
        pipe->buffers[pipe->writerBufferIndex()][pipe->bufferSizes[pipe->writerBufferIndex()]++] = v;
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

StreamPipe::StreamPipe()
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
