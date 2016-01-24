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
#ifndef MATH_CONSTANTS_H_INCLUDED
#define MATH_CONSTANTS_H_INCLUDED

#include <cmath>

#ifndef M_SQRT3
#define M_SQRT3     1.73205080756887729352 /* sqrt(3) */
#endif // M_SQRT3

// following constants copied from math.h in GNU C library
/* Declarations for math functions.
   Copyright (C) 1991-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef M_E
#define M_E         2.7182818284590452354  /* e */
#endif
#ifndef M_LOG2E
#define M_LOG2E     1.4426950408889634074  /* log_2 e */
#endif // M_LOG2E
#ifndef M_LOG10E
#define M_LOG10E    0.43429448190325182765 /* log_10 e */
#endif // M_LOG10E
#ifndef M_LN2
#define M_LN2       0.69314718055994530942 /* log_e 2 */
#endif // M_LN2
#ifndef M_LN10
#define M_LN10      2.30258509299404568402 /* log_e 10 */
#endif // M_LN10
#ifndef M_PI
#define M_PI        3.14159265358979323846 /* pi */
#endif // M_PI
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923 /* pi/2 */
#endif // M_PI_2
#ifndef M_PI_4
#define M_PI_4      0.78539816339744830962 /* pi/4 */
#endif // M_PI_4
#ifndef M_1_PI
#define M_1_PI      0.31830988618379067154 /* 1/pi */
#endif // M_1_PI
#ifndef M_2_PI
#define M_2_PI      0.63661977236758134308 /* 2/pi */
#endif // M_2_PI
#ifndef M_2_SQRTPI
#define M_2_SQRTPI  1.12837916709551257390 /* 2/sqrt(pi) */
#endif // M_2_SQRTPI
#ifndef M_SQRT2
#define M_SQRT2     1.41421356237309504880 /* sqrt(2) */
#endif // M_SQRT2
#ifndef M_SQRT1_2
#define M_SQRT1_2   0.70710678118654752440 /* 1/sqrt(2) */
#endif // M_SQRT1_2

#endif // MATH_CONSTANTS_H_INCLUDED
