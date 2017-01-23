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
#ifndef GAME_VERSION_H_INCLUDED
#define GAME_VERSION_H_INCLUDED

#include <cwchar>
#include <string>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
namespace GameVersion
{
extern const std::wstring VERSION;
extern const std::uint32_t FILE_VERSION;
constexpr std::uint32_t NETWORK_VERSION = 0;
constexpr std::uint16_t port = 12345;
#ifdef DEBUG_VERSION
const bool DEBUG = true;
#else
const bool DEBUG = false;
#endif
#if defined(_WIN64) || defined(_WIN32)
const bool MOBILE = false;
#elif defined(__ANDROID__)
const bool MOBILE = true;
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
const bool MOBILE = true;
#else
const bool MOBILE = false;
#endif
#elif defined(__linux) || defined(__unix) || defined(__posix)
const bool MOBILE = false;
#else
#error unknown platform
#endif
};
}
}

#endif // GAME_VERSION_H_INCLUDED
