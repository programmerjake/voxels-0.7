/*
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
#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include "util/event.h"
#include "util/monitored_variable.h"

namespace programmerjake
{
namespace voxels
{
struct GameInput
{
    MonitoredBool isCreativeMode;
    MonitoredBool jump; // space
    MonitoredVector moveDirectionPlayerRelative; // w a s d
    MonitoredBool attack; // Left Mouse Button
    Event action; // Right Mouse Button
    Event HotbarMoveLeft; // scroll left
    Event HotbarMoveRight; // scroll right
    Event HotbarSelect; // touch hotbar
    MonitoredBool sneak; // shift
    Event drop; // q
    Event openInventory; // e
    MonitoredBool paused; // p esc
};
}
}

#endif // INPUT_H_INCLUDED
