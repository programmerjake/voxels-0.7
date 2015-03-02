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
#ifndef FLUID_H_INCLUDED
#define FLUID_H_INCLUDED

#include "block/block.h"
#include <sstream>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
namespace Blocks
{
namespace builtin
{
class Fluid : BlockDescriptor
{
public:
    bool isSource() const
    {
        return level == 0;
    }
protected:
    const unsigned level;
    const bool moving;
    virtual unsigned getMaxLevel(Dimension d) const
    {
        return 7;
    }
    virtual const Fluid *getBlockDescriptorForFluidLevel(unsigned newLevel, bool moving) const = 0;
    virtual bool isSameKind(const Fluid *other) const = 0;
    virtual bool canCreateSourceBlocks() const
    {
        return false;
    }
    virtual float getSideHeight(Dimension d) const
    {
        return interpolate<float>((float)level / (float)getMaxLevel(d), 0.95f, 0.01f);
    }
private:
    static std::wstring makeName(std::wstring baseName, unsigned level, bool moving)
    {
        std::wostringstream ss;
        ss << baseName << L"(level=" << level << L",moving=" << boolalpha << moving << L")";
        return ss.str();
    }
protected:
protected:
    Fluid(std::wstring baseName, unsigned level, bool moving)
        : BlockDescriptor(makeName(baseName, level, moving)), level(level), moving(moving)
    {
        #warning finish
    }
};
}
}
}
}

#endif // FLUID_H_INCLUDED
