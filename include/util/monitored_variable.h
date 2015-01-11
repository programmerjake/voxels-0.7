#ifndef MONITORED_VARIABLE_H_INCLUDED
#define MONITORED_VARIABLE_H_INCLUDED

#include "util/event.h"
#include <utility>
#include <string>
#include <stdint>
#include <memory>
#include "util/vector.h"
#include "util/position.h"

namespace programmerjake
{
namespace voxels
{
template <typename T>
class MonitoredVariable final
{
private:
    T variable;
public:
    Event onChange;
    template <typename ...Args>
    explicit MonitoredVariable(Args ...args)
        : variable(std::forward<Args>(args)...)
    {
    }
    T &get()
    {
        return variable;
    }
    const T &get() const
    {
        return variable;
    }
    const T &cget() const
    {
        return variable;
    }
    template <typename R>
    void set(R rt)
    {
        variable = std::forward<R>(rt);
        onChange();
    }
};

typedef MonitoredVariable<float> MonitoredFloat;
typedef MonitoredVariable<double> MonitoredDouble;
typedef MonitoredVariable<bool> MonitoredBool;
typedef MonitoredVariable<std::wstring> MonitoredString;
typedef MonitoredVariable<std::int32_t> MonitoredInt32;
typedef MonitoredVariable<std::int> MonitoredInt;
typedef MonitoredVariable<VectorF> MonitoredVectorF;
typedef MonitoredVariable<PositionF> MonitoredPositionF;
template <typename T>
using MonitoredSharedPtr = MonitoredVariable<std::shared_ptr<T>>;
}
}

#endif // MONITORED_VARIABLE_H_INCLUDED
