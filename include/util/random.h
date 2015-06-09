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
#ifndef RANDOM_H_INCLUDED
#define RANDOM_H_INCLUDED

#include <random>
#include <type_traits>
#include <limits>
#include <cstdint>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
namespace Random
{
template <typename RE>
float GenerateFloat(RE &re)
{
    return std::generate_canonical<float, std::numeric_limits<float>::digits>(re);
}
template <typename RE>
float GenerateFloat(RE &re, float maxV)
{
    return maxV * GenerateFloat(re);
}
template <typename RE>
float GenerateFloat(RE &re, float minV, float maxV)
{
    return (maxV - minV) * GenerateFloat(re) + minV;
}
template <typename ResultType, typename RE>
ResultType GenerateInt(RE &re, ResultType minV, ResultType maxV)
{
    typedef typename std::make_unsigned<typename RE::result_type>::type unsigned_RE_result_type;
    typedef typename std::make_unsigned<ResultType>::type unsigned_result_type;
    typedef typename std::conditional<(std::numeric_limits<unsigned_result_type>::digits > std::numeric_limits<unsigned_RE_result_type>::digits), unsigned_result_type, unsigned_RE_result_type>::type intermediate_type;
    assert(maxV >= minV);
    if(maxV <= minV)
        return minV;
    intermediate_type inputRangeSizeMinusOne = (intermediate_type)re.max() - (intermediate_type)re.min();
    assert(inputRangeSizeMinusOne > 0);
    intermediate_type outputRangeSizeMinusOne = (intermediate_type)maxV - (intermediate_type)minV;
    if(inputRangeSizeMinusOne == outputRangeSizeMinusOne)
    {
        return (ResultType)((intermediate_type)minV + (intermediate_type)re() - (intermediate_type)re.min());
    }
    else if(inputRangeSizeMinusOne > outputRangeSizeMinusOne)
    {
        // don't add 1 to inputRangeSizeMinusOne because that might overflow; so we use a roundabout method
        intermediate_type outputRangeSize = outputRangeSizeMinusOne + 1; // outputRangeSize is at least 2 because we check for minV == maxV above
        intermediate_type scaleFactor = inputRangeSizeMinusOne / outputRangeSize;
        intermediate_type ignoreMin = scaleFactor * outputRangeSize - 1;
        if(inputRangeSizeMinusOne % outputRangeSize == outputRangeSizeMinusOne) // if it divides exactly
        {
            scaleFactor++; // adjust scale factor back to true quotient
            ignoreMin += outputRangeSize;
        }
        // ignore numbers greater than ignoreMin

        for(;;)
        {
            intermediate_type v = (intermediate_type)re() - (intermediate_type)re.min();
            if(v > ignoreMin)
                continue;
            return (ResultType)(v / scaleFactor + (intermediate_type)minV);
        }
    }
    intermediate_type inputRangeSize = inputRangeSizeMinusOne + 1;
    intermediate_type lowSubrangeSizeMinusOne = outputRangeSizeMinusOne % inputRangeSize;
    intermediate_type v = GenerateInt<intermediate_type, RE>(re, 0, lowSubrangeSizeMinusOne);
    intermediate_type highSubrangeSizeMinusOne = outputRangeSizeMinusOne / inputRangeSize - 1;
    if(outputRangeSizeMinusOne % inputRangeSize == inputRangeSizeMinusOne)
        highSubrangeSizeMinusOne++;
    v += inputRangeSize * GenerateInt<intermediate_type, RE>(re, 0, highSubrangeSizeMinusOne);
    return (ResultType)(v + minV);
}
template <typename ResultType, typename RE>
ResultType GenerateInt(RE &re, ResultType maxV)
{
    return GenerateInt<ResultType, RE>(re, 0, maxV);
}
}
}
}

#endif // RANDOM_H_INCLUDED
