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
#ifndef COMPRESSED_STREAM_H_INCLUDED
#define COMPRESSED_STREAM_H_INCLUDED

#include "stream/stream.h"
#include <vector>

using namespace std;

namespace stream
{

class ZLibFormatException final : public IOException
{
public:
    ZLibFormatException(const string &msg)
        : IOException("zlib error : " + msg)
    {
    }
};

class ExpandReader final : public Reader
{
private:
    shared_ptr<Reader> preader;
    Reader &reader;
    shared_ptr<void> state;
    static constexpr size_t bufferSize = 1 << 16;
    vector<uint8_t> buffer, compressedBuffer;
    size_t bufferPointer = 0;
    bool moreAvailable = false, gotEOF = false;
    void readBuffer();
    void readCompressedBuffer();
public:
    ExpandReader(shared_ptr<Reader> preader)
        : ExpandReader(*preader)
    {
        this->preader = preader;
    }
    ExpandReader(Reader &reader);
    virtual ~ExpandReader()
    {
    }
    virtual bool dataAvailable() override
    {
        if(bufferPointer < buffer.size())
            return true;
        return false;
    }
    virtual uint8_t readByte() override
    {
        if(dataAvailable())
            return buffer[bufferPointer++];
        readBuffer();
        return buffer[bufferPointer++];
    }
};

class CompressWriter final : public Writer
{
private:
    shared_ptr<Writer> pwriter;
    Writer &writer;
    shared_ptr<void> state;
    static constexpr size_t bufferSize = 1 << 16;
    vector<uint8_t> buffer, compressedBuffer;
    void writeBuffer();
    void writeCompressedBuffer();
public:
    CompressWriter(shared_ptr<Writer> pwriter)
        : CompressWriter(*pwriter)
    {
        this->pwriter = pwriter;
    }
    CompressWriter(Writer &writer);
    virtual ~CompressWriter()
    {
    }
    void finish();
    virtual void flush() override
    {
        finish();
        writer.flush();
    }
    virtual void writeByte(uint8_t v) override
    {
        if(writeWaits())
        {
            writeBuffer();
        }
        buffer.push_back(v);
    }
    virtual bool writeWaits() override
    {
        if(buffer.size() >= bufferSize)
            return true;
        return false;
    }
};

}

#endif // COMPRESSED_STREAM_H_INCLUDED
