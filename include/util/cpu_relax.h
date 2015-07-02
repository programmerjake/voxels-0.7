/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef CPU_RELAX_H_INCLUDED
#define CPU_RELAX_H_INCLUDED

namespace programmerjake
{
namespace voxels
{
#if defined(__i386__) || defined(__x86_64__)
#if defined(__GNUC__)
inline void cpu_relax()
{
    __asm("rep; nop");
}
#elif 1 // add checks for more compilers
#include <xmmintrin.h>
inline void cpu_relax()
{
    _mm_pause();
}
#else
inline void cpu_relax()
{
}
#endif
#else
inline void cpu_relax()
{
}
#endif
}
}

#endif // CPU_RELAX_H_INCLUDED
