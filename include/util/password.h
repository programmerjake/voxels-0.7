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
#ifndef PASSWORD_H_INCLUDED
#define PASSWORD_H_INCLUDED

#include "stream/stream.h"
#include <random>
#include "util/checked_array.h"
#include <limits>
#include <chrono>
#include <sstream>
#include <string>

namespace programmerjake
{
namespace voxels
{
namespace Password
{
struct Salt final
{
    checked_array<std::uint8_t, 4 * 4> value;

private:
    struct do_not_generate_tag final
    {
    };
    Salt(do_not_generate_tag) : value()
    {
    }

public:
    Salt() : value()
    {
        std::random_device rd;
        std::size_t bitsLeft = 0;
        std::uint32_t v = 0;
        for(std::uint8_t &byte : value)
        {
            if(bitsLeft < 8)
            {
                v = std::uniform_int_distribution<std::uint32_t>()(rd);
                bitsLeft = 32;
            }
            byte = static_cast<std::uint8_t>(v & 0xFF);
            v >>= 8;
            bitsLeft -= 8;
        }
    }
    std::string asByteString() const
    {
        return std::string(reinterpret_cast<const char *>(&value[0]), value.size());
    }
    std::string asHexString() const
    {
        std::ostringstream ss;
        ss.fill('0');
        ss << std::hex << std::nouppercase;
        for(std::uint8_t byte : value)
        {
            ss.width(2);
            ss << static_cast<unsigned>(byte);
        }
        return ss.str();
    }
    void write(stream::Writer &writer) const
    {
        writer.writeBytes(&value[0], value.size());
    }
    static Salt read(stream::Reader &reader)
    {
        Salt retval = Salt(do_not_generate_tag());
        reader.readAllBytes(&retval.value[0], retval.value.size());
        return retval;
    }
};

std::string sha256HashString(std::string str); // in stream/network.cpp

struct HashedPassword final
{
    Salt salt;
    std::string hash;
    std::uint32_t iterationCount;
    HashedPassword(Salt salt, std::string hash, std::uint32_t iterationCount)
        : salt(salt), hash(hash), iterationCount(iterationCount)
    {
    }
    HashedPassword(std::wstring password,
                   std::chrono::microseconds hashForDuration = std::chrono::microseconds(100000))
        : salt(), // create a new salt
          hash(),
          iterationCount()
    {
        std::string passwordInUtf8 = string_cast<std::string>(std::move(password));
        std::string saltByteString = salt.asByteString();
        std::string h = sha256HashString(saltByteString + passwordInUtf8 + saltByteString);
        auto startTime = std::chrono::steady_clock::now();
        for(iterationCount = 0; iterationCount < std::numeric_limits<std::uint32_t>::max();
            iterationCount++)
        {
            if(iterationCount % 256 == 0)
            {
                if(std::chrono::steady_clock::now() - startTime >= hashForDuration)
                    break;
            }
            h = sha256HashString(saltByteString + h);
        }
        hash = std::move(h);
    }
    bool verifyPassword(std::wstring password) const
    {
        std::string passwordInUtf8 = string_cast<std::string>(std::move(password));
        std::string saltByteString = salt.asByteString();
        std::string h = sha256HashString(saltByteString + passwordInUtf8 + saltByteString);
        for(std::uint32_t i = 0; i < iterationCount; i++)
        {
            h = sha256HashString(saltByteString + h);
        }
        if(h == hash)
            return true;
        return false;
    }
    void write(stream::Writer &writer) const
    {
        stream::write<Salt>(writer, salt);
        stream::write<std::wstring>(writer, string_cast<std::wstring>(hash));
        stream::write<std::uint32_t>(writer, iterationCount);
    }
    static HashedPassword read(stream::Reader &reader)
    {
        Salt salt = stream::read<Salt>(reader);
        std::string hash =
            string_cast<std::string>(static_cast<std::wstring>(stream::read<std::wstring>(reader)));
        std::uint32_t iterationCount = stream::read<std::uint32_t>(reader);
        return HashedPassword(salt, hash, iterationCount);
    }
};
}
}
}

#endif // PASSWORD_H_INCLUDED
