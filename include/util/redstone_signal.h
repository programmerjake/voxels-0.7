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
#ifndef REDSTONE_SIGNAL_H_INCLUDED
#define REDSTONE_SIGNAL_H_INCLUDED

#include <cstdint>
#include <type_traits>

namespace programmerjake
{
namespace voxels
{
struct RedstoneSignalComponent final
{
private:
    template <typename T>
    static constexpr T max(T a, T b)
    {
        return a > b ? a : b;
    }

public:
    static constexpr int strengthBitWidth = 4;
    static constexpr unsigned maxStrength = (1U << strengthBitWidth) - 1;
    unsigned strength : strengthBitWidth;
    bool connected : 1;
    constexpr RedstoneSignalComponent() : strength(0), connected(false)
    {
    }
    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    constexpr explicit RedstoneSignalComponent(T strength)
        : strength(strength), connected(true)
    {
    }
    template <typename T,
              typename = typename std::enable_if<std::is_same<std::nullptr_t, T>::value>::type>
    constexpr explicit RedstoneSignalComponent(T, std::nullptr_t = nullptr)
        : strength(0), connected(false)
    {
    }
    constexpr RedstoneSignalComponent operator|(RedstoneSignalComponent rt) const
    {
        return (connected || rt.connected) ?
                   RedstoneSignalComponent(strength > rt.strength ? strength : rt.strength) :
                   RedstoneSignalComponent(nullptr);
    }
    RedstoneSignalComponent &operator|=(RedstoneSignalComponent rt)
    {
        return operator=(operator|(rt));
    }
};

struct RedstoneSignal final
{
    RedstoneSignalComponent weakComponent;
    RedstoneSignalComponent strongComponent;
    constexpr RedstoneSignal(std::nullptr_t = nullptr)
        : weakComponent(nullptr), strongComponent(nullptr)
    {
    }
    constexpr RedstoneSignal(RedstoneSignalComponent weakComponent,
                             RedstoneSignalComponent strongComponent)
        : weakComponent(weakComponent | strongComponent), strongComponent(strongComponent)
    {
    }
    constexpr explicit RedstoneSignal(RedstoneSignalComponent component)
        : weakComponent(component), strongComponent(component)
    {
    }
    constexpr RedstoneSignal operator|(RedstoneSignal rt) const
    {
        return RedstoneSignal(weakComponent | rt.weakComponent,
                              strongComponent | rt.strongComponent);
    }
    RedstoneSignal &operator|=(RedstoneSignal rt)
    {
        return operator=(operator|(rt));
    }
    constexpr RedstoneSignal makeWeaker() const
    {
        return RedstoneSignal(strongComponent, RedstoneSignalComponent(nullptr));
    }
    RedstoneSignal &weaken()
    {
        return operator=(makeWeaker());
    }
};
}
}

#endif // REDSTONE_SIGNAL_H_INCLUDED
