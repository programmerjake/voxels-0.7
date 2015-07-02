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
#ifndef IOSTREAM_H_INCLUDED
#define IOSTREAM_H_INCLUDED

#include "stream/stream.h"
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>
#include "util/circular_deque.h"

#include "util/wchar_bits.h"

namespace programmerjake
{
namespace voxels
{
namespace stream
{
class ReaderStreamBuf final : public std::wstreambuf /// reads UTF-8
{
    ReaderStreamBuf(const ReaderStreamBuf &) = delete;
    ReaderStreamBuf &operator =(const ReaderStreamBuf &) = delete;
private:
    std::shared_ptr<Reader> preader;
    Reader &reader;
    static constexpr std::size_t bufferSize = 256;
    circularDeque<wchar_t, bufferSize + 1> buffer;
    bool gotEOFOrError = false;
    wchar_t outputBuffer[1] = {};
    std::uint8_t charBuffer[4] = {};
    std::size_t charBufferUsed = 0;
    bool addToBuffer(unsigned ch) // returns if ch is a valid code-point
    {
        if(ch > 0x10FFFF)
            return false;
        if(ch >= 0xD800 && ch <= 0xDFFF) // surrogate pairs
            return false;
#if WCHAR_BITS == 16
        if(ch >= 0x10000U)
        {
            ch -= 0x10000U;
            ch &= 0xFFFFFU;
            buffer.push_back(static_cast<wchar_t>((ch >> 10) + 0xD800U));
            buffer.push_back(static_cast<wchar_t>((ch & 0x3FF) + 0xDC00U));
        }
        else
#endif
        {
            buffer.push_back(static_cast<wchar_t>(ch));
        }
        return true;
    }
    void readAhead(std::size_t forcedReadCount)
    {
        if(gotEOFOrError)
            return;
        while(buffer.size() < buffer.capacity() - 1 // for surrogate pairs when wchar_t is for UTF-16
              && (forcedReadCount > 0 || reader.dataAvailable()))
        {
            try
            {
                if(1 != reader.readBytes(&charBuffer[charBufferUsed], 1))
                {
                    gotEOFOrError = true;
                    return;
                }
                charBufferUsed++;
                if(charBufferUsed >= 2)
                {
                    if((charBuffer[charBufferUsed - 1] & 0xC0) != 0x80) // expected continuation byte
                    {
                        gotEOFOrError = true;
                        return;
                    }
                }
                if(charBuffer[0] < 0x80)
                {
                    if(!addToBuffer(charBuffer[0]))
                    {
                        gotEOFOrError = true;
                        return;
                    }
                    charBufferUsed = 0;
                    if(forcedReadCount > 0)
                        forcedReadCount--;
                }
                else if(charBuffer[0] < 0xC0) // invalid byte
                {
                    gotEOFOrError = true;
                    return;
                }
                else if(charBuffer[0] < 0xE0) // 2 byte sequence
                {
                    if(charBufferUsed >= 2)
                    {
                        unsigned v = charBuffer[0] & 0x1F;
                        v <<= 6;
                        v |= charBuffer[1] & 0x3F;
                        charBufferUsed = 0;
                        if(!addToBuffer(v))
                        {
                            gotEOFOrError = true;
                            return;
                        }
                        if(forcedReadCount > 0)
                            forcedReadCount--;
                    }
                }
                else if(charBuffer[0] < 0xF0) // 3 byte sequence
                {
                    if(charBufferUsed >= 3)
                    {
                        unsigned v = charBuffer[0] & 0x0F;
                        v <<= 6;
                        v |= charBuffer[1] & 0x3F;
                        v <<= 6;
                        v |= charBuffer[2] & 0x3F;
                        charBufferUsed = 0;
                        if(!addToBuffer(v))
                        {
                            gotEOFOrError = true;
                            return;
                        }
                        if(forcedReadCount > 0)
                            forcedReadCount--;
                    }
                }
                else if(charBuffer[0] < 0xF8) // 4 byte sequence
                {
                    if(charBufferUsed >= 4)
                    {
                        unsigned v = charBuffer[0] & 0x0F;
                        v <<= 6;
                        v |= charBuffer[1] & 0x3F;
                        v <<= 6;
                        v |= charBuffer[2] & 0x3F;
                        v <<= 6;
                        v |= charBuffer[3] & 0x3F;
                        charBufferUsed = 0;
                        if(!addToBuffer(v))
                        {
                            gotEOFOrError = true;
                            return;
                        }
                        if(forcedReadCount > 0)
                            forcedReadCount--;
                    }
                }
                else // invalid byte
                {
                    gotEOFOrError = true;
                    return;
                }
            }
            catch(EOFException &)
            {
                gotEOFOrError = true;
                return;
            }
            catch(IOException &)
            {
                gotEOFOrError = true;
                return;
            }
        }
    }
public:
    explicit ReaderStreamBuf(Reader &reader)
        : preader(),
        reader(reader),
        buffer()
    {
    }
    explicit ReaderStreamBuf(std::shared_ptr<Reader> preader)
        : preader(preader),
        reader(*preader),
        buffer()
    {
    }
    Reader &getReader()
    {
        return reader;
    }
    std::shared_ptr<Reader> getPReader()
    {
        return preader;
    }
protected:
    virtual std::streamsize showmanyc() override
    {
        readAhead(0);
        if(buffer.empty())
        {
            if(gotEOFOrError)
                return -1;
            return 0;
        }
        return buffer.size();
    }
    virtual std::wint_t underflow() override
    {
        if(buffer.empty())
        {
            readAhead(1);
            if(buffer.empty())
            {
                return std::char_traits<wchar_t>::eof();
            }
        }
        outputBuffer[0] = buffer.front();
        buffer.pop_front();
        setg(outputBuffer, outputBuffer, &outputBuffer[1]);
        return outputBuffer[0];
    }
};
class ReaderIStream final : public std::wistream
{
    ReaderIStream(const ReaderIStream &) = delete;
    ReaderIStream &operator =(const ReaderIStream &) = delete;
private:
    ReaderStreamBuf sb;
public:
    explicit ReaderIStream(Reader &reader)
        : std::wistream(nullptr), sb(reader)
    {
        rdbuf(&sb);
    }
    explicit ReaderIStream(std::shared_ptr<Reader> reader)
        : std::wistream(nullptr), sb(reader)
    {
        rdbuf(&sb);
    }
    Reader &getReader()
    {
        return sb.getReader();
    }
    std::shared_ptr<Reader> getPReader()
    {
        return sb.getPReader();
    }
};
class WriterStreamBuf final : public std::wstreambuf /// writes UTF-8
{
    WriterStreamBuf(const WriterStreamBuf &) = delete;
    WriterStreamBuf &operator =(const WriterStreamBuf &) = delete;
private:
    std::shared_ptr<Writer> pwriter;
    Writer &writer;
#if WCHAR_BITS == 16
    static constexpr std::uint_fast32_t emptyCharBuffer = ~static_cast<std::uint_fast32_t>(0);
    std::uint_fast32_t charBuffer = emptyCharBuffer;
#endif
    bool writeByte(std::uint8_t v)
    {
        try
        {
            writer.writeByte(v);
        }
        catch(IOException &)
        {
            return false;
        }
        return true;
    }
    bool flush()
    {
        try
        {
            writer.flush();
        }
        catch(IOException &)
        {
            return false;
        }
        return true;
    }
    bool writeCodePoint(std::uint_fast32_t ch)
    {
        if(ch < 0x80U)
        {
            return writeByte(ch);
        }
        if(ch < 0x800U)
        {
            if(!writeByte(((ch >> 6) & 0x1F) | 0xC0))
                return false;
            return writeByte((ch & 0x3F) | 0x80);
        }
        if(ch < 0x10000U)
        {
            if(!writeByte(((ch >> 12) & 0xF) | 0xE0))
                return false;
            if(!writeByte(((ch >> 6) & 0x3F) | 0x80))
                return false;
            return writeByte((ch & 0x3F) | 0x80);
        }
        if(!writeByte(((ch >> 18) & 0x7) | 0xF0))
            return false;
        if(!writeByte(((ch >> 12) & 0x3F) | 0x80))
            return false;
        if(!writeByte(((ch >> 6) & 0x3F) | 0x80))
            return false;
        return writeByte((ch & 0x3F) | 0x80);
    }
    bool writeWChar(wchar_t ch)
    {
#if WCHAR_BITS == 16
        std::uint_fast32_t v = ch;
        v &= 0xFFFFU;
        if(charBuffer != emptyCharBuffer)
        {
            if(v < 0xDC00U || v > 0xDFFFU)
            {
                bool retval = writeCodePoint(charBuffer);
                charBuffer = emptyCharBuffer;
                if(!retval)
                    return false;
            }
            else
            {
                v &= 0x3FFU;
                v |= (charBuffer & 0x3FFU) << 10;
                v += 0x10000U;
                charBuffer = emptyCharBuffer;
                return writeCodePoint(v);
            }
        }
        if(v >= 0xD800U && v <= 0xDBFFU)
        {
            charBuffer = v;
            return true;
        }
        return writeCodePoint(v);
#else
        return writeCodePoint(ch);
#endif // WCHAR_BITS == 16
    }
public:
    explicit WriterStreamBuf(Writer &writer)
        : pwriter(),
        writer(writer)
    {
    }
    explicit WriterStreamBuf(std::shared_ptr<Writer> pwriter)
        : pwriter(pwriter),
        writer(*pwriter)
    {
    }
    Writer &getWriter()
    {
        return writer;
    }
    std::shared_ptr<Writer> getPWriter()
    {
        return pwriter;
    }
protected:
    virtual int sync() override
    {
        if(flush())
            return 0;
        return -1;
    }
    virtual std::wint_t overflow(std::wint_t ch = std::char_traits<wchar_t>::eof()) override
    {
        if(std::char_traits<wchar_t>::not_eof(ch) != ch) // got EOF
        {
            if(flush())
                return L' '; // not EOF : success
            return std::char_traits<wchar_t>::eof();
        }
        if(writeWChar(ch))
            return ch;
        return std::char_traits<wchar_t>::eof();
    }
};
class WriterOStream final : public std::wostream
{
    WriterOStream(const WriterOStream &) = delete;
    WriterOStream &operator =(const WriterOStream &) = delete;
private:
    WriterStreamBuf sb;
public:
    explicit WriterOStream(Writer &writer)
        : std::wostream(nullptr), sb(writer)
    {
        rdbuf(&sb);
    }
    explicit WriterOStream(std::shared_ptr<Writer> writer)
        : std::wostream(nullptr), sb(writer)
    {
        rdbuf(&sb);
    }
    Writer &getWriter()
    {
        return sb.getWriter();
    }
    std::shared_ptr<Writer> getPWriter()
    {
        return sb.getPWriter();
    }
};
}
}
}

#endif // IOSTREAM_H_INCLUDED
