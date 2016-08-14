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
#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "util/event.h"
#include "util/monitored_variable.h"

namespace programmerjake
{
namespace voxels
{
struct HotBarSelectEventArguments : public EventArguments
{
    std::size_t newSelection;
    HotBarSelectEventArguments(std::size_t newSelection = 0) : newSelection(newSelection)
    {
    }
};
struct GameInput
{
    MonitoredBool isCreativeMode;
    MonitoredBool jump; // space
    MonitoredBool fly; // space double tap
    MonitoredVectorF moveDirectionPlayerRelative; // w a s d
    MonitoredBool attack; // Left Mouse Button
    Event action; // Right Mouse Button
    Event hotBarMoveLeft; // scroll left
    Event hotBarMoveRight; // scroll right
    Event hotBarSelect; // touch hot bar or 1-9
    MonitoredBool sneak; // shift
    Event drop; // q
    Event openInventory; // e
    MonitoredBool paused; // p esc
    MonitoredFloat viewTheta;
    MonitoredFloat viewPhi;
    MonitoredFloat viewPsi;
    void copy(const GameInput &rt)
    {
        isCreativeMode.set(rt.isCreativeMode.get());
        jump.set(rt.jump.get());
        fly.set(rt.fly.get());
        moveDirectionPlayerRelative.set(rt.moveDirectionPlayerRelative.get());
        attack.set(rt.attack.get());
        sneak.set(rt.sneak.get());
        paused.set(rt.paused.get());
        viewTheta.set(rt.viewTheta.get());
        viewPhi.set(rt.viewPhi.get());
        viewPsi.set(rt.viewPsi.get());
    }
    GameInput()
        : isCreativeMode(),
          jump(),
          fly(),
          moveDirectionPlayerRelative(),
          attack(),
          action(),
          hotBarMoveLeft(),
          hotBarMoveRight(),
          hotBarSelect(),
          sneak(),
          drop(),
          openInventory(),
          paused(),
          viewTheta(),
          viewPhi(),
          viewPsi()
    {
    }
};
}
}

#endif // INPUT_H_INCLUDED
