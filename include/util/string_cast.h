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
#ifndef STRING_CAST_H_INCLUDED
#define STRING_CAST_H_INCLUDED

#include <cwchar>
#include <string>
#include <cstdint>

#include "util/wchar_bits.h"

namespace programmerjake
{
namespace voxels
{
template <typename T>
T string_cast(std::string);

template <typename T>
T string_cast(std::wstring);

template <>
inline std::string string_cast<std::string>(std::string str)
{
    return str;
}

template <>
inline std::wstring string_cast<std::wstring>(std::wstring str)
{
    return str;
}

template <>
inline std::string string_cast<std::string>(std::wstring wstr)
{
    std::string retval;
    for(size_t i = 0; i < wstr.size(); i++)
    {
        unsigned value = static_cast<unsigned>(static_cast<UNSIGNED_WCHAR>(wstr[i]));
#if WCHAR_BITS == 16
        if(value >= 0xD800U && value <= 0xDBFFU && i + 1 < wstr.size())
        {
            unsigned nextValue = static_cast<unsigned>(static_cast<UNSIGNED_WCHAR>(wstr[i + 1]));
            if(nextValue >= 0xDC00U && nextValue <= 0xDFFFU)
            {
                i++;
                value -= 0xD800U;
                nextValue -= 0xDC00U;
                value <<= 10;
                value += nextValue;
            }
        }
#endif
        value &= 0x1FFFFFU;
        if(value <= 0x7FU)
        {
            retval += static_cast<char>(value);
        }
        else if(value <= 0x7FFU)
        {
            retval += static_cast<char>((value >> 6) | 0xC0U);
            retval += static_cast<char>((value & 0x3FU) | 0x80U);
        }
        else if(value <= 0xFFFFU)
        {
            retval += static_cast<char>((value >> 12) | 0xE0U);
            retval += static_cast<char>(((value >> 6) & 0x3FU) | 0x80U);
            retval += static_cast<char>((value & 0x3FU) | 0x80U);
        }
        else
        {
            retval += static_cast<char>((value >> 18) | 0xF0U);
            retval += static_cast<char>(((value >> 12) & 0x3FU) | 0x80U);
            retval += static_cast<char>(((value >> 6) & 0x3FU) | 0x80U);
            retval += static_cast<char>((value & 0x3FU) | 0x80U);
        }
    }
    return retval;
}

template <>
inline std::wstring string_cast<std::wstring>(std::string str)
{
    std::wstring retval;
    for(size_t i = 0; i < str.size(); i++)
    {
        unsigned value = static_cast<unsigned char>(str[i]);
        if((value & 0xF0) == 0xF0) // 4 or more byte
        {
            value &= 0x7;
            if(i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80)
            {
                i++;
                value <<= 6;
                value |= 0x3F & static_cast<unsigned char>(str[i]);
                if(i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80)
                {
                    i++;
                    value <<= 6;
                    value |= 0x3F & static_cast<unsigned char>(str[i]);
                    if(i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80)
                    {
                        i++;
                        value <<= 6;
                        value |= 0x3F & static_cast<unsigned char>(str[i]);
                    }
                }
            }
        }
        else if((value & 0xF0) == 0xE0) // 3 byte
        {
            value &= 0xF;
            if(i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80)
            {
                i++;
                value <<= 6;
                value |= 0x3F & static_cast<unsigned char>(str[i]);
                if(i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80)
                {
                    i++;
                    value <<= 6;
                    value |= 0x3F & static_cast<unsigned char>(str[i]);
                }
            }
        }
        else if((value & 0xE0) == 0xC0) // 2 byte
        {
            value &= 0x1F;
            if(i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80)
            {
                i++;
                value <<= 6;
                value |= 0x3F & static_cast<unsigned char>(str[i]);
            }
        }
#if WCHAR_BITS == 16
        if(value >= 0x10000U)
        {
            value -= 0x10000U;
            value &= 0xFFFFFU;
            retval += static_cast<wchar_t>((value >> 10) + 0xD800U);
            retval += static_cast<wchar_t>((value & 0x3FF) + 0xDC00U);
        }
        else
#endif
        {
            retval += static_cast<wchar_t>(value);
        }
    }
    return retval;
}
}
}

#endif // STRING_CAST_H_INCLUDED
