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
#ifndef THREE_STATE_BOOL_H_INCLUDED
#define THREE_STATE_BOOL_H_INCLUDED

#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{

enum class ThreeStateBool
{
    False = 0, // must be 0 so casting to and from bool is false
    True = 1, // must be 1 so casting to and from bool is true
    Unknown = 2,
    DEFINE_ENUM_LIMITS(False, Unknown)
};

inline ThreeStateBool defaultIfUnknown(ThreeStateBool v, bool defaultValue)
{
    switch(v)
    {
    case ThreeStateBool::False:
    case ThreeStateBool::True:
        return v;
    default:
        if(defaultValue)
            return ThreeStateBool::True;
        return ThreeStateBool::False;
    }
}

inline ThreeStateBool defaultIfUnknown(ThreeStateBool v, ThreeStateBool defaultValue)
{
    switch(v)
    {
    case ThreeStateBool::False:
    case ThreeStateBool::True:
        return v;
    default:
        return defaultValue;
    }
}

inline bool toBool(ThreeStateBool v, bool defaultValue)
{
    switch(v)
    {
    case ThreeStateBool::False:
        return false;
    case ThreeStateBool::True:
        return true;
    default:
        return defaultValue;
    }
}

inline ThreeStateBool toThreeStateBool(bool v)
{
    return v ? ThreeStateBool::True : ThreeStateBool::False;
}

inline ThreeStateBool invert(ThreeStateBool v)
{
    switch(v)
    {
    case ThreeStateBool::True:
        return ThreeStateBool::False;
    case ThreeStateBool::False:
        return ThreeStateBool::True;
    default:
        return ThreeStateBool::Unknown;
    }
}

}
}

#endif // THREE_STATE_BOOL_H_INCLUDED
