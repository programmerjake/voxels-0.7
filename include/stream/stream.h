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
#ifndef STREAM_H
#define STREAM_H

#include <cstdint>
#include <stdexcept>
#include <cwchar>
#include <string>
#include <cstdio>
#include <cstring>
#include <memory>
#include <list>
#include <cmath>
#include <functional>
#include <type_traits>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <utility>
#include "util/string_cast.h"
#include "util/enum_traits.h"
#include "util/circular_deque.h"
#include "util/util.h"
#ifdef DEBUG_STREAM
#include <iostream>
#define DUMP_V(fn, v) do {std::cerr << #fn << ": read " << v << std::endl;} while(false)
#else
#define DUMP_V(fn, v)
#endif

namespace programmerjake
{
namespace voxels
{
typedef float float32_t;
typedef double float64_t;

namespace stream
{

class IOException : public std::runtime_error
{
public:
    explicit IOException(const std::string & msg)
        : std::runtime_error(msg)
    {
    }
    explicit IOException(std::exception *e, bool deleteIt = true)
        : std::runtime_error((dynamic_cast<IOException *>(e) == nullptr) ? std::string("IO Error : ") + e->what() : std::string(e->what()))
    {
        if(deleteIt)
            delete e;
    }
    explicit IOException(std::exception & e, bool deleteIt = false)
        : IOException(&e, deleteIt)
    {
    }
    static void throwErrorFromErrno(std::string functionName)
    {
        throw IOException(functionName + " failed: " + std::strerror(errno));
    }
};

class EOFException final : public IOException
{
public:
    explicit EOFException()
        : IOException("IO Error : reached end of file")
    {
    }
};

class NoStreamsLeftException final : public IOException
{
public:
    explicit NoStreamsLeftException()
        : IOException("IO Error : no streams left")
    {
    }
};

class UTFDataFormatException final : public IOException
{
public:
    explicit UTFDataFormatException()
        : IOException("IO Error : invalid UTF data")
    {
    }
};

class InvalidDataValueException final : public IOException
{
public:
    explicit InvalidDataValueException(std::string msg)
        : IOException(msg)
    {
    }
};

class NonSeekableException final : public IOException
{
public:
    NonSeekableException()
        : IOException("IO Error : non-seekable stream")
    {
    }
};

class SeekOutOfRangeException final : public IOException
{
public:
    SeekOutOfRangeException()
        : IOException("IO Error : seek out-of-range")
    {
    }
};

class Stream
{
    Stream(const Stream &) = delete;
    Stream &operator =(const Stream &) = delete;
private:
    std::unordered_map<std::size_t, std::shared_ptr<void>> valueMap;
    static std::size_t allocateIndex()
    {
        static std::atomic_size_t retval(0);
        return retval++;
    }
    template <typename T, typename TagType>
    static std::size_t getIndex()
    {
        static const std::size_t retval = allocateIndex();
        return retval;
    }
protected:
    Stream()
        : valueMap()
    {
    }
    virtual ~Stream() = default;
public:
    template <typename T, typename TagType>
    std::shared_ptr<T> getAssociatedValue()
    {
        return std::static_pointer_cast<T>(valueMap[getIndex<T, TagType>()]);
    }
    template <typename T, typename TagType>
    void setAssociatedValue(std::shared_ptr<T> value)
    {
        valueMap[getIndex<T, TagType>()] = std::static_pointer_cast<void>(value);
    }
};

enum class SeekPosition
{
    Start,
    Current,
    End
};

class Reader : public Stream
{
private:
    template <typename T>
    static T limitAfterRead(T v, T min, T max)
    {
        using std::to_string;
        if(v < min || v > max)
        {
            throw InvalidDataValueException("read value out of range : " + to_string(v));
        }
        return v;
    }
public:
    Reader()
    {
    }
    Reader(const Reader &) = delete;
    const Reader & operator =(const Reader &) = delete;
    virtual ~Reader()
    {
    }
    virtual std::uint8_t readByte() = 0;
    virtual bool dataAvailable() // return if data is available without waiting
    {
        return false;
    }
    virtual std::int64_t tell()
    {
        throw NonSeekableException();
    }
    virtual void seek(std::int64_t offset, SeekPosition seekPosition)
    {
        throw NonSeekableException();
    }
    virtual std::size_t readAvailableBytes(std::uint8_t *array, std::size_t maxCount)
    {
        std::size_t retval;
        try
        {
            for(retval = 0; retval < maxCount; retval++)
            {
                if(!dataAvailable())
                    return retval;
                *array++ = readByte();
            }
        }
        catch(EOFException &)
        {
            return retval;
        }
        return retval;
    }
    virtual std::size_t readBytes(std::uint8_t *array, std::size_t maxCount)
    {
        std::size_t retval;
        try
        {
            for(retval = 0; retval < maxCount; retval++)
            {
                *array++ = readByte();
            }
        }
        catch(EOFException &)
        {
            return retval;
        }
        return retval;
    }
    void readAllBytes(std::uint8_t *array, std::size_t count)
    {
        std::size_t readCount = readBytes(array, count);
        if(readCount < count)
        {
            throw EOFException();
        }
    }
    std::uint8_t readU8()
    {
        std::uint8_t retval = readByte();
        DUMP_V(readU8, (unsigned)retval);
        return retval;
    }
    std::int8_t readS8()
    {
        std::int8_t retval = readByte();
        DUMP_V(readS8, (int)retval);
        return retval;
    }
    std::uint16_t readU16()
    {
        std::uint16_t v = readU8();
        std::uint16_t retval = (v << 8) | readU8();
        DUMP_V(readU16, retval);
        return retval;
    }
    std::int16_t readS16()
    {
        std::int16_t retval = readU16();
        DUMP_V(readS16, retval);
        return retval;
    }
    std::uint32_t readU32()
    {
        std::uint32_t v = readU16();
        std::uint32_t retval = (v << 16) | readU16();
        DUMP_V(readU32, retval);
        return retval;
    }
    std::int32_t readS32()
    {
        std::int32_t retval = readU32();
        DUMP_V(readS32, retval);
        return retval;
    }
    std::uint64_t readU64()
    {
        std::uint64_t v = readU32();
        std::uint64_t retval = (v << 32) | readU32();
        DUMP_V(readU64, retval);
        return retval;
    }
    std::int64_t readS64()
    {
        std::int64_t retval = readU64();
        DUMP_V(readS64, retval);
        return retval;
    }
    float32_t readF32()
    {
        static_assert(sizeof(float32_t) == sizeof(std::uint32_t), "float32_t is not 32 bits");
        union
        {
            std::uint32_t ival;
            float32_t fval;
        };
        ival = readU32();
        float32_t retval = fval;
        DUMP_V(readF32, retval);
        return retval;
    }
    float64_t readF64()
    {
        static_assert(sizeof(float64_t) == sizeof(std::uint64_t), "float64_t is not 64 bits");
        union
        {
            std::uint64_t ival;
            float64_t fval;
        };
        ival = readU64();
        float64_t retval = fval;
        DUMP_V(readF64, retval);
        return retval;
    }
    bool readBool()
    {
        return readU8() != 0;
    }
    std::wstring readString()
    {
        std::string retval = "";
        for(;;)
        {
            std::uint32_t b1 = readU8();
            if(b1 == 0)
            {
                return string_cast<std::wstring>(retval);
            }
            else if((b1 & 0x80) == 0)
            {
                retval += (char)b1;
            }
            else if((b1 & 0xE0) == 0xC0)
            {
                retval += (char)b1;
                std::uint32_t b2 = readU8();
                if((b2 & 0xC0) != 0x80)
                    throw UTFDataFormatException();
                retval += (char)b2;
            }
            else if((b1 & 0xF0) == 0xE0)
            {
                retval += (char)b1;
                std::uint32_t b2 = readU8();
                if((b2 & 0xC0) != 0x80)
                    throw UTFDataFormatException();
                retval += (char)b2;
                std::uint32_t b3 = readU8();
                if((b3 & 0xC0) != 0x80)
                    throw UTFDataFormatException();
                retval += (char)b3;
            }
            else if((b1 & 0xF8) == 0xF0)
            {
                retval += (char)b1;
                std::uint32_t b2 = readU8();
                if((b2 & 0xC0) != 0x80)
                    throw UTFDataFormatException();
                retval += (char)b2;
                std::uint32_t b3 = readU8();
                if((b3 & 0xC0) != 0x80)
                    throw UTFDataFormatException();
                retval += (char)b3;
                std::uint32_t b4 = readU8();
                if((b4 & 0xC0) != 0x80)
                    throw UTFDataFormatException();
                retval += (char)b4;
                std::uint32_t v = b4 & 0x3F;
                v |= (b3 & 0x3F) << 6;
                v |= (b2 & 0x3F) << 12;
                v |= (b1 & 0x7) << 18;
                if(v >= 0x10FFFF)
                    throw UTFDataFormatException();
            }
            else
                throw UTFDataFormatException();
        }
    }
    std::uint8_t readLimitedU8(std::uint8_t min, std::uint8_t max)
    {
        return limitAfterRead(readU8(), min, max);
    }
    std::int8_t readLimitedS8(std::int8_t min, std::int8_t max)
    {
        return limitAfterRead(readS8(), min, max);
    }
    std::uint16_t readLimitedU16(std::uint16_t min, std::uint16_t max)
    {
        return limitAfterRead(readU16(), min, max);
    }
    std::int16_t readLimitedS16(std::int16_t min, std::int16_t max)
    {
        return limitAfterRead(readS16(), min, max);
    }
    std::uint32_t readLimitedU32(std::uint32_t min, std::uint32_t max)
    {
        return limitAfterRead(readU32(), min, max);
    }
    std::int32_t readLimitedS32(std::int32_t min, std::int32_t max)
    {
        return limitAfterRead(readS32(), min, max);
    }
    std::uint64_t readLimitedU64(std::uint64_t min, std::uint64_t max)
    {
        return limitAfterRead(readU64(), min, max);
    }
    std::int64_t readLimitedS64(std::int64_t min, std::int64_t max)
    {
        return limitAfterRead(readS64(), min, max);
    }
    float32_t readFiniteF32()
    {
        float32_t retval = readF32();
        if(!std::isfinite(retval))
        {
            throw InvalidDataValueException("read value is not finite");
        }
        return retval;
    }
    float64_t readFiniteF64()
    {
        float64_t retval = readF64();
        if(!std::isfinite(retval))
        {
            throw InvalidDataValueException("read value is not finite");
        }
        return retval;
    }
    float32_t readLimitedF32(float32_t min, float32_t max)
    {
        return limitAfterRead(readFiniteF32(), min, max);
    }
    float64_t readLimitedF64(float64_t min, float64_t max)
    {
        return limitAfterRead(readFiniteF64(), min, max);
    }
};

class Writer : public Stream
{
public:
    Writer()
    {
    }
    Writer(const Writer &) = delete;
    const Writer & operator =(const Writer &) = delete;
    virtual ~Writer()
    {
    }
    virtual void writeByte(std::uint8_t) = 0;
    virtual void flush()
    {
    }
    virtual bool writeWaits()
    {
        return true;
    }
    virtual void writeBytes(const std::uint8_t * array, std::size_t count)
    {
        for(size_t i = 0; i < count; i++)
            writeByte(array[i]);
    }
    void writeU8(std::uint8_t v)
    {
        writeByte(v);
    }
    void writeS8(std::int8_t v)
    {
        writeByte(v);
    }
    void writeU16(std::uint16_t v)
    {
        writeU8((std::uint8_t)(v >> 8));
        writeU8((std::uint8_t)(v & 0xFF));
    }
    void writeS16(std::int16_t v)
    {
        writeU16(v);
    }
    void writeU32(std::uint32_t v)
    {
        writeU16((std::uint16_t)(v >> 16));
        writeU16((std::uint16_t)(v & 0xFFFF));
    }
    void writeS32(std::int32_t v)
    {
        writeU32(v);
    }
    void writeU64(std::uint64_t v)
    {
        writeU32((std::uint64_t)(v >> 32));
        writeU32((std::uint64_t)(v & 0xFFFFFFFFU));
    }
    void writeS64(std::int64_t v)
    {
        writeU64(v);
    }
    void writeF32(float32_t v)
    {
        static_assert(sizeof(float32_t) == sizeof(std::uint32_t), "float is not 32 bits");
        union
        {
            std::uint32_t ival;
            float32_t fval;
        };
        fval = v;
        writeU32(ival);
    }
    void writeF64(float64_t v)
    {
        static_assert(sizeof(float64_t) == sizeof(std::uint64_t), "double is not 64 bits");
        union
        {
            std::uint64_t ival;
            float64_t fval;
        };
        fval = v;
        writeU64(ival);
    }
    void writeBool(bool v)
    {
        writeU8(v ? 1 : 0);
    }
    void writeString(std::wstring v)
    {
        std::string str = string_cast<std::string>(v);
        for(char ch : str)
        {
            if(ch != 0)
            {
                writeU8(ch);
            }
            else
            {
                writeU8(0xC0);
                writeU8(0x80);
            }
        }
        writeU8(0);
    }
};

template <typename ReturnType>
struct read_base
{
    read_base(const read_base &) = delete;
    const read_base & operator =(const read_base &) = delete;
protected:
    ReturnType value;
    read_base(ReturnType && value)
        : value(value)
    {
    }
public:
    operator ReturnType &&() &&
    {
        return std::move(value);
    }
};

template <typename T, typename = void>
struct read;

template <typename T>
struct is_value_changed
{
    constexpr bool operator ()(std::shared_ptr<const T>, Stream &) const
    {
        return false;
    }
};

template <typename T, typename = void>
struct write;

template <typename T, typename = void>
struct rw_class_traits;

template <typename T>
struct rw_class_traits<T, typename std::enable_if<std::is_class<T>::value>::type> final
{
private:
    template <typename T2>
    static decltype(T2::read(std::declval<Reader &>())) f(decltype(std::declval<T2 &>().write(std::declval<Writer &>())) *);
    template <typename T2>
    static void f(...);
public:
    typedef decltype(f<T>(nullptr)) value_type;
    static constexpr bool has_rw = !std::is_same<void, value_type>::value && std::is_move_constructible<T>::value;
};

template <typename T>
struct rw_class_traits<T, typename std::enable_if<std::is_enum<T>::value || std::is_arithmetic<T>::value>::type>
{
    static constexpr bool has_rw = true;
    typedef T value_type;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename T>
struct read<T, typename std::enable_if<std::is_class<T>::value && rw_class_traits<T>::has_rw>::type> : public read_base<typename rw_class_traits<T>::value_type>
{
#pragma GCC diagnostic pop
    read(Reader &reader)
        : read_base<typename rw_class_traits<T>::value_type>(T::read(reader))
    {
    }
};

template <typename T>
struct write<T, typename std::enable_if<std::is_class<T>::value && rw_class_traits<T>::has_rw>::type>
{
    write(Writer &writer, T &value)
    {
        value.write(writer);
    }
    write(Writer &writer, T &&value)
    {
        std::move(value).write(writer);
    }
    write(Writer &writer, const T &value)
    {
        value.write(writer);
    }
};

template <typename T, typename = void>
struct read_finite;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename T>
struct read_finite<T, typename std::enable_if<std::is_integral<T>::value>::type> : public stream::read<T>
{
#pragma GCC diagnostic pop
    read_finite(Reader & reader)
        : stream::read<T>(reader)
    {
    }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename T>
inline typename rw_class_traits<T>::value_type read_checked(Reader & reader, std::function<bool(typename rw_class_traits<T>::value_type)> checkFn)
{
#pragma GCC diagnostic pop
    typename rw_class_traits<T>::value_type retval = stream::read<T>(reader);
    if(!checkFn(retval))
    {
        throw InvalidDataValueException("check failed in read_checked");
    }
    return retval;
}

template <typename T>
struct read_limited;

#define DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(typeName, functionSuffix) \
template <> \
struct read<typeName, void> : public read_base<typeName> \
{ \
    read(Reader & reader) \
        : read_base<typeName>(reader.read ## functionSuffix()) \
    { \
    } \
}; \
template <> \
struct read_limited<typeName> : public read_base<typeName> \
{ \
    read_limited(Reader & reader, typeName minV, typeName maxV) \
        : read_base<typeName>(reader.readLimited ## functionSuffix(minV, maxV)) \
    { \
    } \
}; \
template <> \
struct write<typeName, void> \
{ \
    write(Writer & writer, typeName value) \
    { \
        writer.write ## functionSuffix(value); \
    } \
};

#define DEFINE_RW_FUNCTIONS_FOR_BASIC_FLOAT_TYPE(typeName, functionSuffix) \
template <> \
struct read<typeName, void> : public read_base<typeName> \
{ \
    read(Reader & reader) \
        : read_base<typeName>(reader.read ## functionSuffix()) \
    { \
    } \
}; \
template <> \
struct read_limited<typeName> : public read_base<typeName> \
{ \
    read_limited(Reader & reader, typeName minV, typeName maxV) \
        : read_base<typeName>(reader.readLimited ## functionSuffix(minV, maxV)) \
    { \
    } \
}; \
template <> \
struct read_finite<typeName, void> : public read_base<typeName>\
{ \
    read_finite(Reader &reader) \
        : read_base<typeName>(reader.readFinite ## functionSuffix()) \
    { \
    } \
}; \
template <> \
struct write<typeName, void> \
{ \
    write(Writer & writer, typeName value) \
    { \
        writer.write ## functionSuffix(value); \
    } \
};

#define DEFINE_RW_FUNCTIONS_FOR_BASIC_TYPE(typeName, functionSuffix) \
template <> \
struct read<typeName, void> : public read_base<typeName> \
{ \
    read(Reader & reader) \
        : read_base<typeName>(reader.read ## functionSuffix()) \
    { \
    } \
}; \
template <> \
struct write<typeName, void> \
{ \
    write(Writer & writer, typeName value) \
    { \
        writer.write ## functionSuffix(value); \
    } \
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::uint8_t, U8)
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::int8_t, S8)
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::uint16_t, U16)
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::int16_t, S16)
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::uint32_t, U32)
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::int32_t, S32)
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::uint64_t, U64)
DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE(std::int64_t, S64)
DEFINE_RW_FUNCTIONS_FOR_BASIC_FLOAT_TYPE(float32_t, F32)
DEFINE_RW_FUNCTIONS_FOR_BASIC_FLOAT_TYPE(float64_t, F64)
DEFINE_RW_FUNCTIONS_FOR_BASIC_TYPE(std::wstring, String)
DEFINE_RW_FUNCTIONS_FOR_BASIC_TYPE(bool, Bool)
#pragma GCC diagnostic pop

#undef DEFINE_RW_FUNCTIONS_FOR_BASIC_INTEGER_TYPE
#undef DEFINE_RW_FUNCTIONS_FOR_BASIC_FLOAT_TYPE
#undef DEFINE_RW_FUNCTIONS_FOR_BASIC_TYPE

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename T>
struct read<T, typename std::enable_if<std::is_enum<T>::value>::type> : public read_base<T>
{
#pragma GCC diagnostic pop
    read(Reader &reader)
        : read_base<T>((T)(typename enum_traits<T>::rwtype)stream::read_limited<typename enum_traits<T>::rwtype>(reader, (typename enum_traits<T>::rwtype)enum_traits<T>::minimum, (typename enum_traits<T>::rwtype)enum_traits<T>::maximum))
    {
    }
};

template <typename T>
struct write<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
    write(Writer &writer, T value)
    {
        stream::write<typename enum_traits<T>::rwtype>(writer, (typename enum_traits<T>::rwtype)value);
    }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename T>
struct read_nonnull : public read_base<typename rw_class_traits<T>::value_type>
{
#pragma GCC diagnostic pop
    read_nonnull(Reader &reader)
        : read_base<typename rw_class_traits<T>::value_type>((std::shared_ptr<T>)read<T>(reader))
    {
        if(this->value == nullptr)
            throw InvalidDataValueException("read null pointer in read_nonnull");
    }
};

class FileReader final : public Reader
{
    FileReader(const FileReader &) = delete;
    FileReader &operator =(const FileReader &) = delete;
private:
    FILE * f;
public:
    explicit FileReader(std::wstring fileName);
    explicit FileReader(FILE * f)
        : f(f)
    {
        assert(f != nullptr);
    }
    virtual ~FileReader()
    {
        std::fclose(f);
    }
    virtual std::uint8_t readByte() override
    {
        int ch = std::fgetc(f);
        if(ch == EOF)
        {
            if(std::ferror(f))
                throw IOException("IO Error : can't read from file");
            throw EOFException();
        }
        return ch;
    }
    virtual std::int64_t tell() override;
    virtual void seek(std::int64_t offset, SeekPosition seekPosition) override;
    virtual std::size_t readBytes(std::uint8_t *array, std::size_t maxCount) override;
};

class FileWriter final : public Writer
{
    FileWriter(const FileWriter &) = delete;
    FileWriter &operator =(const FileWriter &) = delete;
private:
    FILE * f;
public:
    explicit FileWriter(std::wstring fileName)
        : f(nullptr)
    {
        std::string str = string_cast<std::string>(fileName);
        f = std::fopen(str.c_str(), "wb");
        if(f == nullptr)
            throw IOException(std::string("IO Error : ") + std::strerror(errno));
    }
    explicit FileWriter(FILE * f)
        : f(f)
    {
        assert(f != nullptr);
    }
    virtual ~FileWriter()
    {
        fclose(f);
    }
    virtual void writeByte(std::uint8_t v) override
    {
        if(fputc(v, f) == EOF)
            throw IOException("IO Error : can't write to file");
    }
    virtual void flush() override
    {
        if(EOF == fflush(f))
            throw IOException("IO Error : can't write to file");
    }
    virtual void writeBytes(const std::uint8_t *array, std::size_t count) override;
};

class MemoryReader final : public Reader
{
private:
    std::shared_ptr<const std::uint8_t> mem;
    std::size_t offset;
    const std::size_t length;
public:
    explicit MemoryReader(std::shared_ptr<const std::uint8_t> mem, std::size_t length)
        : mem(mem), offset(0), length(length)
    {
    }
    explicit MemoryReader(std::shared_ptr<const std::vector<std::uint8_t>> mem)
        : mem(mem, mem->data()), offset(0), length(mem->size())
    {
    }
    explicit MemoryReader(const std::vector<std::uint8_t> &mem)
        : mem(), offset(0), length(mem.size())
    {
        std::uint8_t * memory = new std::uint8_t[mem.size()];
        for(std::size_t i = 0; i < mem.size(); i++)
            memory[i] = mem[i];
        this->mem = std::shared_ptr<std::uint8_t>(memory, [](std::uint8_t * memory){delete []memory;});
    }
    explicit MemoryReader(std::vector<std::uint8_t> &&mem)
        : MemoryReader(std::make_shared<std::vector<std::uint8_t>>(std::move(mem)))
    {
    }
    template <std::size_t length>
    explicit MemoryReader(const std::uint8_t (&a)[length])
        : MemoryReader(std::shared_ptr<const std::uint8_t>(&a[0], [](const std::uint8_t *){}), length)
    {
    }
    virtual bool dataAvailable() override
    {
        return offset < length;
    }
    virtual std::uint8_t readByte() override
    {
        if(offset >= length)
            throw EOFException();
        return mem.get()[offset++];
    }
    virtual std::size_t readAvailableBytes(std::uint8_t *array, std::size_t maxCount) override
    {
        if(maxCount > length - offset)
            maxCount = length - offset;
        for(std::size_t i = 0; i < maxCount; i++)
        {
            *array++ = mem.get()[offset++];
        }
        return maxCount;
    }
    virtual std::int64_t tell() override
    {
        return offset;
    }
    virtual void seek(std::int64_t o, SeekPosition seekPosition) override
    {
        switch(seekPosition)
        {
        case SeekPosition::Start:
            if(static_cast<std::size_t>(o) > length || o < 0)
                throw SeekOutOfRangeException();
            offset = o;
            break;
        case SeekPosition::Current:
            if(static_cast<std::size_t>(o + offset) > length || static_cast<std::int64_t>(o + offset) < 0)
                throw SeekOutOfRangeException();
            offset += o;
            break;
        case SeekPosition::End:
            if(o > 0 || static_cast<std::uint64_t>(-o) > length)
                throw SeekOutOfRangeException();
            offset = length + o;
            break;
        default:
            UNREACHABLE();
        }
    }
};

class MemoryWriter final : public Writer
{
private:
    std::vector<std::uint8_t> memory;
public:
    MemoryWriter()
        : memory()
    {
    }
    explicit MemoryWriter(std::size_t expectedLength)
        : memory()
    {
        memory.reserve(expectedLength);
    }
    virtual void writeByte(std::uint8_t v) override
    {
        memory.push_back(v);
    }
    const std::vector<std::uint8_t> & getBuffer() const &
    {
        return memory;
    }
    std::vector<std::uint8_t> && getBuffer() &&
    {
        return std::move(memory);
    }
    virtual bool writeWaits() override
    {
        return false;
    }
    virtual void writeBytes(const std::uint8_t *array, std::size_t count) override
    {
        if(count == 0)
            return;
        memory.insert(memory.end(), array, array + count);
    }
};

class StreamPipe final
{
    StreamPipe(const StreamPipe &) = delete;
    const StreamPipe & operator =(const StreamPipe &) = delete;
private:
    std::shared_ptr<Reader> readerInternal;
    std::shared_ptr<Writer> writerInternal;
public:
    StreamPipe();
    Reader & reader()
    {
        return *readerInternal;
    }
    Writer & writer()
    {
        return *writerInternal;
    }
    std::shared_ptr<Reader> preader()
    {
        return readerInternal;
    }
    std::shared_ptr<Writer> pwriter()
    {
        return writerInternal;
    }
};

class DumpingReader final : public Reader
{
private:
    Reader &reader;
public:
    DumpingReader(Reader& reader)
        : reader(reader)
    {
    }
    virtual std::uint8_t readByte() override;
    virtual bool dataAvailable() override
    {
        return reader.dataAvailable();
    }
};

struct StreamRW
{
    StreamRW()
    {
    }
    StreamRW(const StreamRW &) = delete;
    const StreamRW & operator =(const StreamRW &) = delete;
    virtual ~StreamRW()
    {
    }
    Reader & reader()
    {
        return *preader();
    }
    Writer & writer()
    {
        return *pwriter();
    }
    virtual std::shared_ptr<Reader> preader() = 0;
    virtual std::shared_ptr<Writer> pwriter() = 0;
};

class StreamRWWrapper final : public StreamRW
{
private:
    std::shared_ptr<Reader> preaderInternal;
    std::shared_ptr<Writer> pwriterInternal;
public:
    StreamRWWrapper(std::shared_ptr<Reader> preaderInternal, std::shared_ptr<Writer> pwriterInternal)
        : preaderInternal(preaderInternal), pwriterInternal(pwriterInternal)
    {
    }

    virtual std::shared_ptr<Reader> preader() override
    {
        return preaderInternal;
    }

    virtual std::shared_ptr<Writer> pwriter() override
    {
        return pwriterInternal;
    }
};

class StreamBidirectionalPipe final
{
private:
    StreamPipe pipe1, pipe2;
    std::shared_ptr<StreamRW> port1Internal, port2Internal;
public:
    StreamBidirectionalPipe()
        : pipe1(), pipe2(), port1Internal(), port2Internal()
    {
        port1Internal = std::shared_ptr<StreamRW>(new StreamRWWrapper(pipe1.preader(), pipe2.pwriter()));
        port2Internal = std::shared_ptr<StreamRW>(new StreamRWWrapper(pipe2.preader(), pipe1.pwriter()));
    }
    std::shared_ptr<StreamRW> pport1()
    {
        return port1Internal;
    }
    std::shared_ptr<StreamRW> pport2()
    {
        return port2Internal;
    }
    StreamRW & port1()
    {
        return *port1Internal;
    }
    StreamRW & port2()
    {
        return *port2Internal;
    }
};

struct StreamServer
{
    StreamServer()
    {
    }
    StreamServer(const StreamServer &) = delete;
    const StreamServer & operator =(const StreamServer &) = delete;
    virtual ~StreamServer()
    {
    }
    virtual std::shared_ptr<StreamRW> accept() = 0;
};

class StreamServerWrapper final : public StreamServer
{
private:
    std::list<std::shared_ptr<StreamRW>> streams;
    std::shared_ptr<StreamServer> nextServer;
public:
    StreamServerWrapper(std::list<std::shared_ptr<StreamRW>> streams, std::shared_ptr<StreamServer> nextServer = nullptr)
        : streams(streams), nextServer(nextServer)
    {
    }
    StreamServerWrapper(std::vector<std::shared_ptr<StreamRW>> streams, std::shared_ptr<StreamServer> nextServer = nullptr)
        : streams(streams.begin(), streams.end()), nextServer(nextServer)
    {
    }
    virtual std::shared_ptr<StreamRW> accept() override
    {
        if(streams.empty())
        {
            if(nextServer == nullptr)
                throw NoStreamsLeftException();
            return nextServer->accept();
        }
        std::shared_ptr<StreamRW> retval = streams.front();
        streams.pop_front();
        return retval;
    }
};

template <std::size_t BufferSize = 32768>
class BufferedReader final : public Reader
{
    std::shared_ptr<Reader> preader;
    circularDeque<std::uint8_t, BufferSize + 1> buffer;
    void readChunk()
    {
        for(;;)
        {
            if(buffer.size() >= buffer.capacity())
                return;
            if(preader->dataAvailable())
                buffer.push_back(preader->readByte());
            else
                return;
        }
    }
public:
    BufferedReader(std::shared_ptr<Reader> preader)
        : preader(preader)
    {
    }
    virtual bool dataAvailable() override
    {
        return !buffer.empty() || preader->dataAvailable();
    }
    virtual std::uint8_t readByte() override
    {
        if(buffer.empty())
            readChunk();
        if(buffer.empty())
            return preader->readByte();
        std::uint8_t retval = buffer.front();
        buffer.pop_front();
        return retval;
    }
};

template <std::size_t BufferSize = 32768>
class BufferedWriter final : public Writer
{
    std::shared_ptr<Writer> pwriter;
    circularDeque<std::uint8_t, BufferSize + 1> buffer;
    void writeBuffer(bool writeAll)
    {
        while(!buffer.empty() && (writeAll || buffer.size() >= buffer.capacity() || !pwriter->writeWaits()))
        {
            pwriter->writeByte(buffer.front());
            buffer.pop_front();
        }
    }
public:
    BufferedWriter(std::shared_ptr<Writer> pwriter)
        : pwriter(pwriter)
    {
    }
    virtual bool writeWaits() override
    {
        return buffer.size() >= buffer.capacity() && pwriter->writeWaits();
    }
    virtual void flush() override
    {
        writeBuffer(true);
        pwriter->flush();
    }
    virtual void writeByte(std::uint8_t v) override
    {
        if(buffer.size() >= buffer.capacity() / 2)
            writeBuffer(false);
        buffer.push_back(v);
    }
};

}
}
}

#endif // STREAM_H
