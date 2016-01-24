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

#ifndef UTIL_XORSHIFTPLUS_H_
#define UTIL_XORSHIFTPLUS_H_

#include <random>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{

class XorShift128Plus final
{
public:
    typedef std::uint_fast64_t result_type;
private:
    result_type state0, state1;
public:
    static constexpr result_type default_seed = static_cast<result_type>(0x7AB5E995C32E4511ULL);
    explicit XorShift128Plus(result_type seedValue = default_seed) noexcept
        : state0(seedValue), state1(seedValue ^ static_cast<result_type>(0x123897C7232F503FULL))
    {
    }
    void seed(result_type seedValue = default_seed) noexcept
    {
        *this = XorShift128Plus(seedValue);
    }
    result_type operator ()() noexcept
    {
        auto temp = state0;
        state0 = state1;
        state1 = temp;
        state1 ^= state1 << 23;
        state1 &= max();
        state1 ^= state0 ^ (state1 >> 18) ^ (state0 >> 5);
        return state0 + state1;
    }
    void discard(unsigned long long count) noexcept
    {
        for(unsigned long long i = 0; i < count; i++)
        {
            operator ()();
        }
    }
    static constexpr result_type min() noexcept
    {
        return 0;
    }
    static constexpr result_type max() noexcept
    {
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    friend bool operator ==(const XorShift128Plus &l, const XorShift128Plus &r)
    {
        return l.state0 == r.state0 && l.state1 == r.state1;
    }
    friend bool operator !=(const XorShift128Plus &l, const XorShift128Plus &r)
    {
        return l.state0 != r.state0 || l.state1 != r.state1;
    }
};

}
}

#endif /* UTIL_XORSHIFTPLUS_H_ */
