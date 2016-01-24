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
#ifndef TOOL_TOOLS_H_INCLUDED
#define TOOL_TOOLS_H_INCLUDED

#include "item/builtin/tools/tool.h"

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

class Pickaxe : public Tool
{
public:
    Pickaxe(std::wstring name, Mesh meshFace, Mesh entityMesh)
        : Tool(name, meshFace, entityMesh)
    {
    }
    Pickaxe(std::wstring name, TextureDescriptor td)
        : Tool(name, td)
    {
    }
    virtual Item incrementDamage(Item tool, bool toolKindMatches, bool toolCanMine, bool minedInstantly) const override
    {
        if(minedInstantly)
            return tool;
        return addDamage(tool, 1);
    }
};

class Axe : public Tool
{
public:
    Axe(std::wstring name, Mesh meshFace, Mesh entityMesh)
        : Tool(name, meshFace, entityMesh)
    {
    }
    Axe(std::wstring name, TextureDescriptor td)
        : Tool(name, td)
    {
    }
    virtual Item incrementDamage(Item tool, bool toolKindMatches, bool toolCanMine, bool minedInstantly) const override
    {
        if(minedInstantly)
            return tool;
        return addDamage(tool, 1);
    }
};

class Hoe : public Tool
{
public:
    Hoe(std::wstring name, Mesh meshFace, Mesh entityMesh)
        : Tool(name, meshFace, entityMesh)
    {
    }
    Hoe(std::wstring name, TextureDescriptor td)
        : Tool(name, td)
    {
    }
    virtual Item incrementDamage(Item tool, bool toolKindMatches, bool toolCanMine, bool minedInstantly) const override
    {
        return tool;
    }
};

class Shovel : public Tool
{
public:
    Shovel(std::wstring name, Mesh meshFace, Mesh entityMesh)
        : Tool(name, meshFace, entityMesh)
    {
    }
    Shovel(std::wstring name, TextureDescriptor td)
        : Tool(name, td)
    {
    }
    virtual Item incrementDamage(Item tool, bool toolKindMatches, bool toolCanMine, bool minedInstantly) const override
    {
        if(minedInstantly)
            return tool;
        return addDamage(tool, 1);
    }
};

}
}
}
}
}

#endif // TOOL_TOOLS_H_INCLUDED
