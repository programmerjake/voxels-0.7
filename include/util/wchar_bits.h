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
#ifndef WCHAR_BITS_H_INCLUDED
#define WCHAR_BITS_H_INCLUDED

#include <cstdint>
#include <cwchar>

#if WCHAR_MAX == 0xFFFFFFFFU && WCHAR_MIN == 0U
#define WCHAR_BITS (32)
#define WCHAR_SIGN unsigned
#define UNSIGNED_WCHAR wchar_t
#elif WCHAR_MAX == 0x7FFFFFFF && WCHAR_MIN == -0x80000000
#define WCHAR_BITS (32)
#define WCHAR_SIGN signed
#define UNSIGNED_WCHAR std::uint_least32_t
#elif WCHAR_MAX == 0xFFFFU && WCHAR_MIN == 0U
#define WCHAR_BITS (16)
#define WCHAR_SIGN unsigned
#define UNSIGNED_WCHAR wchar_t
#elif WCHAR_MAX == 0x7FFF && WCHAR_MIN == -0x8000
#define WCHAR_BITS (16)
#define WCHAR_SIGN signed
#define UNSIGNED_WCHAR std::uint_least16_t
#else
#error can not detect number of bits in wchar_t
#endif

#endif // WCHAR_BITS_H_INCLUDED
