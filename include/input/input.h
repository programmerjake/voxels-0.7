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
    MonitoredBool jump;
    MonitoredVector moveDirectionPlayerRelative;
    MonitoredBool attack; // Left Mouse Button
    MonitoredBool action; // Right Mouse Button
    Event HotbarMoveLeft;
    Event HotbarMoveRight;
    Event HotbarSelect;
    #warning finish
};
}
}

#endif // INPUT_H_INCLUDED
