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
const float eps = 1e-4;

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

#define constexpr_assert(v) (void)((v) ? 0 : throw ::programmerjake::voxels::constexpr_assert_failure([](){assert(! #v);}))

template <typename T>
constexpr T ensureInRange(const T v, const T minV, const T maxV)
{
    return (constexpr_assert(!(v < minV) && !(maxV < v)), v);
}

int ifloor(float v)
{
    return floor(v);
}

int iceil(float v)
{
    return ceil(v);
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
