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
#ifndef REDSTONE_SIGNAL_H_INCLUDED
#define REDSTONE_SIGNAL_H_INCLUDED

namespace programmerjake
{
namespace voxels
{
struct RedstoneSignal final
{
private:
    static constexpr int max(int a, int b)
    {
        return a > b ? a : b;
    }
public:
    int strongSignalStrength = 0;
    int weakSignalStrength = 0;
    bool canConnectToRedstoneDustDirectly = false;
    bool canConnectToRedstoneDustThroughBlock = false;
    static constexpr int maxSignalStrength = 0xF;
    constexpr RedstoneSignal()
    {
    }
    constexpr RedstoneSignal(int strongSignalStrength, int weakSignalStrength, bool canConnectToRedstoneDustDirectly, bool canConnectToRedstoneDustThroughBlock)
        : strongSignalStrength(strongSignalStrength),
          weakSignalStrength(max(weakSignalStrength, strongSignalStrength)),
          canConnectToRedstoneDustDirectly(canConnectToRedstoneDustDirectly || canConnectToRedstoneDustThroughBlock),
          canConnectToRedstoneDustThroughBlock(canConnectToRedstoneDustThroughBlock)
    {
    }
    constexpr RedstoneSignal(int strongSignalStrength)
        : RedstoneSignal(strongSignalStrength, strongSignalStrength, true, true)
    {
    }
    constexpr RedstoneSignal(bool strongSignalStrength)
        : RedstoneSignal(strongSignalStrength ? maxSignalStrength : 0, strongSignalStrength ? maxSignalStrength : 0, true, true)
    {
    }
    constexpr bool good() const
    {
        return isOnAtAll() || canConnectToRedstoneDustDirectly;
    }
    constexpr RedstoneSignal combine(RedstoneSignal rt) const
    {
        return RedstoneSignal(max(strongSignalStrength, rt.strongSignalStrength),
                              max(weakSignalStrength, rt.weakSignalStrength),
                              canConnectToRedstoneDustDirectly || rt.canConnectToRedstoneDustDirectly,
                              canConnectToRedstoneDustThroughBlock || rt.canConnectToRedstoneDustThroughBlock);
    }
    constexpr RedstoneSignal transmitThroughSolidBlock() const
    {
        return RedstoneSignal(0, weakSignalStrength, canConnectToRedstoneDustThroughBlock, false);
    }
    constexpr int getRedstoneDustSignalStrength() const
    {
        return canConnectToRedstoneDustDirectly ? strongSignalStrength : 0;
    }
    constexpr int getSignalStrength() const
    {
        return weakSignalStrength;
    }
    constexpr bool isOnAtAll() const
    {
        return getSignalStrength() > 0;
    }
};
}
}

#endif // REDSTONE_SIGNAL_H_INCLUDED
