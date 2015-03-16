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
#ifndef SIMPLE_TOOLSET_H_INCLUDED
#define SIMPLE_TOOLSET_H_INCLUDED

#include "item/builtin/tools/tools.h"
#include "recipe/builtin/pattern.h"
#include "item/builtin/wood.h" // for stick

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
namespace tools
{

class SimplePickaxe : public Pickaxe
{
private:
    const ToolLevel toolLevel;
    const float mineDurationFactor;
public:
    SimplePickaxe(std::wstring toolsetName, TextureDescriptor td, ToolLevel toolLevel, float mineDurationFactor, Item recipeMaterial)
        : Pickaxe(L"builtin.tools.simple_pickaxe(toolset=" + toolsetName + L")", td), toolLevel(toolLevel), mineDurationFactor(mineDurationFactor)
    {
        #warning add recipe
    }
    virtual float getMineDurationFactor(Item item) const override
    {
        return mineDurationFactor;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return toolLevel;
    }
};

#warning add more tools to toolset

class SimpleToolset
{
    SimpleToolset(const SimpleToolset &) = delete;
    const SimpleToolset &operator =(const SimpleToolset &) = delete;
public:
    const std::wstring toolsetName;
    const ToolLevel toolLevel;
    const float mineDurationFactor;
protected:
    const Pickaxe *pickaxe;
    static const Pickaxe *makePickaxe(std::wstring toolsetName, ToolLevel toolLevel, float mineDurationFactor, Item recipeMaterial, TextureDescriptor td)
    {
        if(td)
            return new SimplePickaxe(toolsetName, toolLevel, td, mineDurationFactor, recipeMaterial);
        return nullptr;
    }
public:
    SimpleToolset(std::wstring toolsetName, ToolLevel toolLevel, float mineDurationFactor, Item recipeMaterial, const Pickaxe *pickaxe)
        : toolsetName(toolsetName), toolLevel(toolLevel), mineDurationFactor(mineDurationFactor), pickaxe(pickaxe)
    {
    }
    SimpleToolset(std::wstring toolsetName, ToolLevel toolLevel, float mineDurationFactor, Item recipeMaterial, TextureDescriptor pickaxeTexture)
        : toolsetName(toolsetName), toolLevel(toolLevel), mineDurationFactor(mineDurationFactor)
    {
        pickaxe = makePickaxe(toolsetName, toolLevel, mineDurationFactor, recipeMaterial, pickaxeTexture);
    }
    virtual ~SimpleToolset()
    {
        delete pickaxe;
    }
    const Pickaxe *getPickaxe() const
    {
        return pickaxe;
    }
};

}
}
}
}
}

#endif // SIMPLE_TOOLSET_H_INCLUDED
