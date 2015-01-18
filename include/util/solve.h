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
#ifndef SOLVE_H_INCLUDED
#define SOLVE_H_INCLUDED

#include <cmath>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
inline int solveLinear(float a/*constant*/, float b/*linear*/, float retval[1])
{
    retval[0] = 0;

    if(std::abs(b) < eps)
    {
        return (std::abs(a) < eps) ? 1 : 0;
    }

    retval[0] = -a / b;
    return 1;
}

inline int solveQuadratic(float a/*constant*/, float b/*linear*/, float c/*quadratic*/, float retval[2])
{
    if(std::abs(c) < eps)
    {
        return solveLinear(a, b, retval);
    }

    float sqrtArg = b * b - 4 * c * a;

    if(sqrtArg < 0)
    {
        return 0;
    }

    if(c < 0)
    {
        a = -a;
        b = -b;
        c = -c;
    }

    float sqrtV = std::sqrt(sqrtArg);
    retval[0] = (-b - sqrtV) / (2 * c);
    retval[1] = (-b + sqrtV) / (2 * c);
    return 2;
}

inline int solveCubic(float a/*constant*/, float b/*linear*/, float c/*quadratic*/, float d/*cubic*/,
               float retval[3])
{
    if(std::abs(d) < eps)
    {
        return solveQuadratic(a, b, c, retval);
    }

    a /= d;
    b /= d;
    c /= d;
    float Q = (c * c - 3 * b) / 9;
    float R = (2 * (c * c * c) - 9 * (c * b) + 27 * a) / 54;
    float R2 = R * R;
    float Q3 = Q * Q * Q;

    if(R2 < Q3)
    {
        float theta = std::acos(R / std::sqrt(Q3));
        float SQ = std::sqrt(Q);
        retval[0] = -2 * SQ * std::cos(theta / 3) - c / 3;
        retval[1] = -2 * SQ * std::cos((theta + 2 * M_PI) / 3) - c / 3;
        retval[2] = -2 * SQ * std::cos((theta - 2 * M_PI) / 3) - c / 3;
        return 3;
    }

    float A = -std::cbrt(std::abs(R) + std::sqrt(R2 - Q3));

    if(R < 0)
    {
        A = -A;
    }

    float B;
    if(A == 0)
    {
        B = 0;
    }
    else
    {
        B = Q / A;
    }

    float AB = A + B;
    retval[0] = AB - c / 3;
    return 1;
}
}
}

#endif // SOLVE_H_INCLUDED
