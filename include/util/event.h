#ifndef UTIL_EVENT_H_INCLUDED
#define UTIL_EVENT_H_INCLUDED

#include <functional>
#include <list>
#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{
struct EventArguments
{
    virtual ~EventArguments()
    {
    }
};

class Event
{
public:
    enum ReturnType
    {
        Discard,
        Propagate,
        DEFINE_ENUM_LIMITS(Discard, Propagate)
    };
    enum class Priority
    {
        Low,
        High,
        Default = Low,
        DEFINE_ENUM_LIMITS(Low, High)
    };
private:
    std::list<std::function<ReturnType (EventArguments &args)>> functions;
public:
    explicit Event(std::function<ReturnType (EventArguments &args)> fn = nullptr)
    {
        bind(fn);
    }
    Event(Event &&) = default;
    Event(const Event &) = delete;
    const Event &operator =(Event &&) = default;
    const Event &operator =(const Event &) = delete;
    void bind(std::function<ReturnType (EventArguments &args)> fn, Priority p = Priority::Default)
    {
        if(fn != nullptr)
        {
            if(p == Priority::High)
                functions.push_front(fn);
            else
                functions.push_back(fn);
        }
    }
    ReturnType operator ()(EventArguments &args)
    {
        for(auto fn : functions)
        {
            ReturnType retval = fn(args);
            if(retval != Propagate)
                return retval;
        }
        return Propagate;
    }
};

}
}

#endif // UTIL_EVENT_H_INCLUDED
