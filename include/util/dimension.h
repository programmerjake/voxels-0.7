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
#ifndef DIMENSION_H_INCLUDED
#define DIMENSION_H_INCLUDED

#include <cstdint>
#include <cassert>
#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{
enum class Dimension : std::uint8_t
{
    Overworld,
    DEFINE_ENUM_LIMITS(Overworld, Overworld)
};

inline float getZeroBrightnessLevel(Dimension d)
{
    switch(d)
    {
    case Dimension::Overworld:
        return 0;
    }
    assert(false);
}

inline float getDaySkyBrightnessLevel(Dimension d)
{
    switch(d)
    {
    case Dimension::Overworld:
        return 1.0f;
    }
    assert(false);
}

inline float getNightSkyBrightnessLevel(Dimension d)
{
    switch(d)
    {
    case Dimension::Overworld:
        return 4.0f / 15.0f;
    }
    assert(false);
}
}
}

#endif // DIMENSION_H_INCLUDED
