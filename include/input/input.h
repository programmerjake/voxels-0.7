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
