/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef COMPRESSED_STREAM_H_INCLUDED
#define COMPRESSED_STREAM_H_INCLUDED

#include "stream/stream.h"
#include <vector>

namespace programmerjake
{
namespace voxels
{
namespace stream
{

class ZLibFormatException final : public IOException
{
public:
    ZLibFormatException(const std::string &msg)
        : IOException("zlib error : " + msg)
    {
    }
};

class ExpandReader final : public Reader
{
private:
    std::shared_ptr<Reader> preader;
    Reader &reader;
    std::shared_ptr<void> state;
    static constexpr std::size_t bufferSize = 1 << 16;
    std::vector<std::uint8_t> buffer, compressedBuffer;
    std::size_t bufferPointer = 0;
    bool moreAvailable = false, gotEOF = false;
    void readBuffer();
    void readCompressedBuffer();
public:
    ExpandReader(std::shared_ptr<Reader> preader)
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
    virtual std::uint8_t readByte() override
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
    std::shared_ptr<Writer> pwriter;
    Writer &writer;
    std::shared_ptr<void> state;
    static constexpr std::size_t bufferSize = 1 << 16;
    std::vector<std::uint8_t> buffer, compressedBuffer;
    void writeBuffer();
    void writeCompressedBuffer();
public:
    CompressWriter(std::shared_ptr<Writer> pwriter)
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
    virtual void writeByte(std::uint8_t v) override
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
}
}

#endif // COMPRESSED_STREAM_H_INCLUDED
