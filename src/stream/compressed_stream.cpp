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
#include "stream/compressed_stream.h"
#include <zlib.h>

namespace
{
#if 0
#define deflate copyit
#define inflate copyit
int copyit(z_streamp s, int)
{
    if(s->avail_in <= 0 || s->avail_out <= 0)
        return Z_BUF_ERROR;
    while(s->avail_in > 0 && s->avail_out > 0)
    {
        *s->next_out++ = *s->next_in++;
        s->avail_in--;
        s->avail_out--;
    }
    return Z_OK;
}
#endif

z_streamp getStream(const shared_ptr<void> &ptr)
{
    return (z_streamp)ptr.get();
}
z_streamp makeDeflateStream()
{
    z_streamp retval = new z_stream;
    retval->zalloc = nullptr;
    retval->zfree = nullptr;
    retval->opaque = nullptr;
    if(deflateInit(retval, 2) != Z_OK)
    {
        string msg = retval->msg;
        delete retval;
        throw stream::ZLibFormatException(msg);
    }
    return retval;
}
void freeDeflateStream(z_streamp stream)
{
    deflateEnd(stream);
    delete stream;
}
void deflateDeleter(void * stream)
{
    freeDeflateStream((z_streamp)stream);
}
z_streamp makeInflateStream()
{
    z_streamp retval = new z_stream;
    retval->zalloc = nullptr;
    retval->zfree = nullptr;
    retval->opaque = nullptr;
    if(inflateInit(retval) != Z_OK)
    {
        string msg = retval->msg;
        delete retval;
        throw stream::ZLibFormatException(msg);
    }
    return retval;
}
void freeInflateStream(z_streamp stream)
{
    inflateEnd(stream);
    delete stream;
}
void inflateDeleter(void * stream)
{
    freeInflateStream((z_streamp)stream);
}
}

namespace stream
{

CompressWriter::CompressWriter(Writer &writer)
    : writer(writer), state(shared_ptr<void>((void *)makeDeflateStream(), deflateDeleter))
{
    buffer.reserve(bufferSize);
    compressedBuffer.resize(bufferSize);
    z_streamp s = getStream(state);
    s->next_out = &compressedBuffer[0];
    s->avail_out = bufferSize;
}

void CompressWriter::writeCompressedBuffer()
{
    z_streamp s = getStream(state);
    if(s->avail_out == bufferSize)
        return;
    uint16_t size = (bufferSize - s->avail_out) & 0xFFFF;
    assert(bufferSize - s->avail_out == size || (size == 0 && bufferSize - s->avail_out == 0x10000));
    stream::write<uint16_t>(writer, size);
    writer.writeBytes(&compressedBuffer[0], bufferSize - s->avail_out);
    s->next_out = &compressedBuffer[0];
    s->avail_out = bufferSize;
}

void CompressWriter::finish()
{
    z_streamp s = getStream(state);
    s->next_in = &buffer[0];
    s->avail_in = buffer.size();
    for(;;)
    {
        switch(deflate(s, Z_SYNC_FLUSH))
        {
        case Z_OK:
            writeCompressedBuffer();
            break;
        case Z_BUF_ERROR:
            buffer.clear();
            return;
        default:
            throw ZLibFormatException(s->msg);
        }
    }
}

void CompressWriter::writeBuffer()
{
    if(buffer.size() == 0)
        return;
    z_streamp s = getStream(state);
    s->next_in = &buffer[0];
    s->avail_in = buffer.size();
    for(;;)
    {
        switch(deflate(s, Z_NO_FLUSH))
        {
        case Z_OK:
            if(CompressWriter::bufferSize - s->avail_out >= 0x100)
                writeCompressedBuffer();
            break;
        case Z_BUF_ERROR:
            buffer.clear();
            return;
        default:
            throw ZLibFormatException(s->msg);
        }
    }
}

ExpandReader::ExpandReader(Reader &reader)
    : reader(reader), state(shared_ptr<void>(makeInflateStream(), inflateDeleter))
{
    buffer.reserve(bufferSize);
    compressedBuffer.resize(bufferSize);
}

void ExpandReader::readCompressedBuffer()
{
    constexpr size_t bytesPerUint16 = 2;
    uint8_t bytes[bytesPerUint16];
    bytes[0] = reader.readByte();
    try
    {
        reader.readBytes(bytes + 1, bytesPerUint16 - 1);
        MemoryReader mreader(bytes);
        size_t size = (uint16_t)stream::read<uint16_t>(mreader);
        if(size == 0)
            size = 0x10000;
        if(size <= 0 || size > bufferSize)
            throw ZLibFormatException("size out of range in ExpandReader::readCompressedBuffer");
        reader.readBytes(&compressedBuffer[0], size);
        z_streamp s = getStream(state);
        s->next_in = &compressedBuffer[0];
        s->avail_in = size;
        moreAvailable = true;
    }
    catch(EOFException &e)
    {
        throw ZLibFormatException("eof reached");
    }
}

void ExpandReader::readBuffer()
{
    if(gotEOF)
        throw EOFException();
    bufferPointer = 0;
    buffer.resize(bufferSize);
    z_streamp s = getStream(state);
    for(;;)
    {
        if(!moreAvailable)
            readCompressedBuffer();
        s->next_out = &buffer[0];
        s->avail_out = bufferSize;
        switch(inflate(s, Z_FINISH))
        {
        case Z_OK:
        case Z_BUF_ERROR:
            if(s->avail_in <= 0)
                moreAvailable = false;
            if(s->avail_out < bufferSize)
            {
                buffer.resize(bufferSize - s->avail_out);
                return;
            }
            assert(!moreAvailable);
            break;
        case Z_STREAM_END:
            gotEOF = true;
            if(s->avail_out < bufferSize)
            {
                buffer.resize(bufferSize - s->avail_out);
                return;
            }
            throw EOFException();
            break;
        default:
            throw ZLibFormatException(s->msg);
        }
    }
}

}

#if 0

#include "util/util.h"
#include <iostream>
#include <cstdlib>

namespace
{
initializer init1([]()
{
    vector<uint8_t> buffer;
    {
        stream::MemoryWriter mwriter;
        stream::CompressWriter writer(mwriter);
        for(int i = 0; i < 10; i++)
        {
            writer.writeString(L"Hello World!");
            writer.flush();
        }
        buffer = std::move(mwriter).getBuffer();
    }
    stream::MemoryReader mreader(std::move(buffer));
    stream::ExpandReader reader(mreader);
    for(int i = 0; i < 10; i++)
        wcout << reader.readString() << endl;
    exit(0);
});
}
#endif // 1
