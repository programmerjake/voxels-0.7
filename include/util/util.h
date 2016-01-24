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
#ifndef UTIL_H
#define UTIL_H

#include <cmath>
#include <cstdint>
#include <cwchar>
#include <string>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
constexpr float eps = 1e-4;

template <typename T>
constexpr T limit(const T v, const T minV, const T maxV)
{
    return maxV < v ? maxV : (v < minV ? minV : v);
}

struct constexpr_assert_failure final
{
    template <typename FnType>
    explicit constexpr_assert_failure(FnType fn)
    {
        fn();
    }
};

#ifndef NDEBUG
#define UNREACHABLE() do {assert(!"unreachable"); throw std::runtime_error("unreachable");} while(0)
#define assume(v) assert(v)
#else
#if defined(__GNUC__) || defined(__GNUG__)
#define UNREACHABLE() __builtin_unreachable()
#elif defined(__HP_cc) || defined(__HP_aCC)
#warning UNREACHABLE() unsupported on Hewlett-Packard C/aC++
#define UNREACHABLE() do {assert(!"unreachable"); throw std::runtime_error("unreachable");} while(0)
#elif defined(__IBMC__) || defined(__IBMCPP__)
#warning UNREACHABLE() unsupported on IBM XL C/C++
#define UNREACHABLE() do {assert(!"unreachable"); throw std::runtime_error("unreachable");} while(0)
#elif defined(_MSC_VER)
#define UNREACHABLE() __assume(false)
#elif defined(__PGI)
#warning UNREACHABLE() unsupported on Portland Group PGCC/PGCPP
#define UNREACHABLE() do {assert(!"unreachable"); throw std::runtime_error("unreachable");} while(0)
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#warning UNREACHABLE() unsupported on Oracle Solaris Studio
#define UNREACHABLE() do {assert(!"unreachable"); throw std::runtime_error("unreachable");} while(0)
#else
#warning UNREACHABLE() unsupported on unknown compiler
#define UNREACHABLE() do {assert(!"unreachable"); throw std::runtime_error("unreachable");} while(0)
#endif
#define assume(v) do {if(v) (void)0; else UNREACHABLE();} while(0)
#endif

#ifdef NDEBUG
#define constexpr_assert(v) ((void)0)
#define assume ((v) ? (void)0 : (void))
#else
#define constexpr_assert(v) ((void)((v) ? 0 : throw ::programmerjake::voxels::constexpr_assert_failure([](){assert(!#v);})))
#endif

#if defined(__GNUC__) || defined(__GNUG__)
#define GCC_PRAGMA(x) PRAGMA(GCC x)
#define FIXME_MESSAGE(x) PRAGMA(GCC warning #x)
template <typename T>
constexpr void ignore_unused_variable_warning(const T &) __attribute__((always_inline));
template <typename T>
constexpr void ignore_unused_variable_warning(const T &)
{
}
#elif defined(__HP_cc) || defined(__HP_aCC)
template <typename T>
constexpr void ignore_unused_variable_warning(const T &)
{
}
#elif defined(__IBMC__) || defined(__IBMCPP__)
template <typename T>
constexpr void ignore_unused_variable_warning(const T &)
{
}
#elif defined(_MSC_VER)
#define PRAGMA(x) __pragma(x)
#define MSVC_PRAGMA(x) __pragma(x)
template <typename T>
__forceinline constexpr void ignore_unused_variable_warning(const T &)
{
}
#elif defined(__PGI)
template <typename T>
constexpr void ignore_unused_variable_warning(const T &)
{
}
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
template <typename T>
constexpr void ignore_unused_variable_warning(const T &)
{
}
#else
template <typename T>
constexpr void ignore_unused_variable_warning(const T &)
{
}
#endif

#ifndef GCC_PRAGMA
#define GCC_PRAGMA(x)
#endif

#ifndef MSVC_PRAGMA
#define MSVC_PRAGMA(x)
#endif

#ifndef PRAGMA
#define PRAGMA(x) _Pragma(#x)
#endif

#ifndef FIXME_MESSAGE
#define FIXME_MESSAGE(x) PRAGMA(message("FIXME - " #x))
#endif

template <typename T>
constexpr T ensureInRange(const T v, const T minV, const T maxV)
{
    return (constexpr_assert(!(v < minV) && !(maxV < v)), v);
}

inline int ifloor(float v)
{
    return static_cast<int>(floor(v));
}

inline int iceil(float v)
{
    return static_cast<int>(ceil(v));
}

template <typename T>
constexpr int sgn(T v)
{
    return v < 0 ? -1 : (0 < v ? 1 : 0);
}

template <typename T>
constexpr T interpolate(float t, T a, T b)
{
    return a + t * (b - a);
}

class initializer
{
private:
    void (*finalizeFn)();
    initializer(const initializer &rt) = delete;
    void operator =(const initializer &rt) = delete;
public:
    initializer(void (*initFn)(), void (*finalizeFn)() = nullptr)
        : finalizeFn(finalizeFn)
    {
        initFn();
    }
    ~initializer()
    {
        if(finalizeFn)
        {
            finalizeFn();
        }
    }
};

class finalizer
{
private:
    void (*finalizeFn)();
    finalizer(const finalizer &rt) = delete;
    void operator =(const finalizer &rt) = delete;
public:
    finalizer(void (*finalizeFn)())
        : finalizeFn(finalizeFn)
    {
        assert(finalizeFn);
    }
    ~finalizer()
    {
        finalizeFn();
    }
};
}
}

#endif // UTIL_H
