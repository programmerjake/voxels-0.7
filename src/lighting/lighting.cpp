/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#include "lighting/lighting.h"
#include "util/util.h"
#include <cstdlib>
#include <iostream>

namespace programmerjake
{
namespace voxels
{
float BlockLighting::evalVertex(
    const checked_array<checked_array<checked_array<float, 3>, 3>, 3> &blockValues, VectorI offset)
{
    float retval = 0;
    for(int dx = 0; dx < 2; dx++)
    {
        for(int dy = 0; dy < 2; dy++)
        {
            for(int dz = 0; dz < 2; dz++)
            {
                VectorI pos = offset + VectorI(dx, dy, dz);
                retval = std::max<float>(retval, blockValues[pos.x][pos.y][pos.z]);
            }
        }
    }
    return retval;
}
#if 0
namespace
{
void dump(bool v)
{
    std::cout << v;
}
void dump(int v)
{
    std::cout << v;
}
void dump(Lighting v)
{
    std::cout << "<" << v.directSkylight << ", " << v.indirectSkylight << ", " << v.indirectArtificalLight << ">";
}
void dump(LightProperties v)
{
    std::cout << "[";
    dump(v.emissiveValue);
    std::cout << ", ";
    dump(v.reduceValue);
    std::cout << "]";
}
template <typename A, typename B>
void dump(const std::pair<A, B> &v)
{
    std::cout << "(";
    dump(std::get<0>(v));
    std::cout << ", ";
    dump(std::get<1>(v));
    std::cout << ")";
}
template <typename T, size_t N>
void dump(const checked_array<T, N> &v)
{
    std::cout << "{";
    const char *separator = "";
    for(const auto &i : v)
    {
        std::cout << separator;
        separator = ", ";
        dump(i);
    }
    std::cout << "}";
}
#define DUMP(v)                       \
    do                                \
    {                                 \
        std::cout << #v << std::endl; \
        dump(v);                      \
        std::cout << std::endl;       \
    } while(0)
}
#endif
BlockLighting::BlockLighting(
    checked_array<checked_array<checked_array<std::pair<LightProperties, Lighting>, 3>, 3>, 3>
        blocks,
    WorldLightingProperties wlp)
    : lightValues()
{
    checked_array<checked_array<checked_array<bool, 3>, 3>, 3> isOpaque, setOpaque;
    for(int x = 0; x < 3; x++)
    {
        for(int y = 0; y < 3; y++)
        {
            for(int z = 0; z < 3; z++)
            {
                LightProperties &lp = std::get<0>(blocks[x][y][z]);
                isOpaque[x][y][z] = (lp.reduceValue.indirectSkylight >= Lighting::maxLight / 2);
                setOpaque[x][y][z] = true;
            }
        }
    }
    for(auto &i : blocks)
    {
        for(auto &j : i)
        {
            for(auto &lv : j)
            {
                std::get<0>(lv).emissiveValue.directSkylight = 0;
                std::get<1>(lv).directSkylight = 0;
            }
        }
    }
    for(int x = 0; x < 3; x++)
        setOpaque[x][1][1] = false;
    for(int y = 0; y < 3; y++)
        setOpaque[1][y][1] = false;
    for(int z = 0; z < 3; z++)
        setOpaque[1][1][z] = false;
    // DUMP(setOpaque);
    // DUMP(isOpaque);
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            const int dz = 0;
            VectorI startPos = VectorI(dx, dy, dz) + VectorI(1);
            for(VectorI propagateDir : {VectorI(-dx, 0, 0), VectorI(0, -dy, 0)})
            {
                VectorI p = startPos + propagateDir;
                if(!isOpaque[p.x][p.y][p.z])
                    setOpaque[startPos.x][startPos.y][startPos.z] = false;
            }
        }
    }
    // DUMP(setOpaque);
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dz = -1; dz <= 1; dz += 2)
        {
            const int dy = 0;
            VectorI startPos = VectorI(dx, dy, dz) + VectorI(1);
            for(VectorI propagateDir : {VectorI(-dx, 0, 0), VectorI(0, 0, -dz)})
            {
                VectorI p = startPos + propagateDir;
                if(!isOpaque[p.x][p.y][p.z])
                    setOpaque[startPos.x][startPos.y][startPos.z] = false;
            }
        }
    }
    // DUMP(setOpaque);
    for(int dy = -1; dy <= 1; dy += 2)
    {
        for(int dz = -1; dz <= 1; dz += 2)
        {
            const int dx = 0;
            VectorI startPos = VectorI(dx, dy, dz) + VectorI(1);
            for(VectorI propagateDir : {VectorI(0, -dy, 0), VectorI(0, 0, -dz)})
            {
                VectorI p = startPos + propagateDir;
                if(!isOpaque[p.x][p.y][p.z])
                    setOpaque[startPos.x][startPos.y][startPos.z] = false;
            }
        }
    }
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            for(int dz = -1; dz <= 1; dz += 2)
            {
                setOpaque[dx + 1][dy + 1][dz + 1] = false;
            }
        }
    }
    for(int x = 0; x < 3; x++)
    {
        for(int y = 0; y < 3; y++)
        {
            for(int z = 0; z < 3; z++)
            {
                if(setOpaque[x][y][z])
                    isOpaque[x][y][z] = true;
            }
        }
    }
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            for(int dz = -1; dz <= 1; dz += 2)
            {
                setOpaque[dx + 1][dy + 1][dz + 1] = true;
            }
        }
    }
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            for(int dz = -1; dz <= 1; dz += 2)
            {
                VectorI startPos = VectorI(dx, dy, dz) + VectorI(1);
                for(VectorI propagateDir :
                    {VectorI(-dx, 0, 0), VectorI(0, -dy, 0), VectorI(0, 0, -dz)})
                {
                    VectorI p = startPos + propagateDir;
                    if(!isOpaque[p.x][p.y][p.z])
                        setOpaque[startPos.x][startPos.y][startPos.z] = false;
                }
            }
        }
    }
    for(int x = 0; x < 3; x++)
    {
        for(int y = 0; y < 3; y++)
        {
            for(int z = 0; z < 3; z++)
            {
                if(setOpaque[x][y][z])
                    isOpaque[x][y][z] = true;
            }
        }
    }
    checked_array<checked_array<checked_array<float, 3>, 3>, 3> blockValues;
    for(size_t x = 0; x < blockValues.size(); x++)
    {
        for(size_t y = 0; y < blockValues[x].size(); y++)
        {
            for(size_t z = 0; z < blockValues[x][y].size(); z++)
            {
                blockValues[x][y][z] = 0;
                if(!isOpaque[x][y][z])
                    blockValues[x][y][z] = std::get<1>(blocks[x][y][z]).toFloat(wlp);
            }
        }
    }
    for(int ox = 0; ox <= 1; ox++)
    {
        for(int oy = 0; oy <= 1; oy++)
        {
            for(int oz = 0; oz <= 1; oz++)
            {
                lightValues[ox][oy][oz] = evalVertex(blockValues, VectorI(ox, oy, oz));
            }
        }
    }
}
#if defined(DEBUG_VERSION) && 0
namespace
{
initializer init1(
    []()
    {
        typedef std::pair<LightProperties, Lighting> LightPair;
        const int cx = 2, cy = 2, cz = 2;
        for(int i = 0; i <= 1; i++)
        {
            checked_array<checked_array<checked_array<LightPair, 3>, 3>, 3> blocks;
            blocks[cx][cy][cz] = LightPair(LightProperties(), Lighting::makeSkyLighting());
            if(i != 0)
                blocks[1][cy][cz] =
                    LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
            blocks[cx][1][cz] =
                LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
            blocks[cx][cy][1] =
                LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
            WorldLightingProperties wlp = WorldLightingProperties(1, Dimension::Overworld);
            BlockLighting bl(blocks, wlp);
            for(int x = 0; x <= 1; x++)
            {
                for(int y = 0; y <= 1; y++)
                {
                    for(int z = 0; z <= 1; z++)
                    {
                        std::cout << VectorI(x, y, z) << " " << bl.eval(VectorF(x, y, z)).r
                                  << std::endl;
                    }
                }
            }
            std::cout << std::endl;
        }
        for(int i = 0; i <= 1; i++)
        {
            checked_array<checked_array<checked_array<LightPair, 3>, 3>, 3> blocks;
            blocks[cx][0][cz] = LightPair(LightProperties(), Lighting::makeSkyLighting());
            blocks[cx][1][cz] = LightPair(LightProperties(), Lighting::makeSkyLighting());
            blocks[cx][2][cz] = LightPair(LightProperties(), Lighting::makeSkyLighting());
            if(i != 0)
            {
                blocks[1][0][cz] =
                    LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
                blocks[1][1][cz] =
                    LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
                blocks[1][2][cz] =
                    LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
            }
            blocks[cx][0][1] =
                LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
            blocks[cx][1][1] =
                LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
            blocks[cx][2][1] =
                LightPair(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
            WorldLightingProperties wlp = WorldLightingProperties(1, Dimension::Overworld);
            BlockLighting bl(blocks, wlp);
            for(int x = 0; x <= 1; x++)
            {
                for(int y = 0; y <= 1; y++)
                {
                    for(int z = 0; z <= 1; z++)
                    {
                        std::cout << VectorI(x, y, z) << " " << bl.eval(VectorF(x, y, z)).r
                                  << std::endl;
                    }
                }
            }
            std::cout << std::endl;
        }
        exit(0);
    });
}
#endif
}
}
