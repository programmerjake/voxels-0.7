#ifndef FLAG_H_INCLUDED
#define FLAG_H_INCLUDED

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace programmerjake
{
namespace voxels
{
class flag final
{
private:
    mutable std::mutex lock;
    mutable std::condition_variable_any cond;
    std::atomic_bool value;
    mutable std::atomic_size_t waitingCount;
public:
    flag(bool value = false)
        : value(value)
    {
    }
    const flag &operator =(bool v)
    {
        if(value.exchange(v) != v)
        {
            if(waitingCount > 0)
                cond.notify_all();
        }

        return *this;
    }
    const flag &operator =(const flag &r)
    {
        return *this = (bool)r;
    }
    bool exchange(bool v)
    {
        bool retval = value.exchange(v);

        if(retval != v)
        {
            if(waitingCount > 0)
                cond.notify_all();
        }

        return retval;
    }
    operator bool() const
    {
        bool retval = value;
        return retval;
    }
    bool operator !() const
    {
        bool retval = value;
        return !retval;
    }
    void wait(bool v = true) const /// waits until value == v
    {
        if(v == value)
        {
            return;
        }

        waitingCount++;

        lock.lock();

        while(v != value)
        {
            cond.wait(lock);
        }

        lock.unlock();

        waitingCount--;
    }
    void waitThenReset(bool v = true) /// waits until value == v then set value to !v
    {
        if(v == value.exchange(!v))
        {
            if(waitingCount > 0)
                cond.notify_all();
            return;
        }

        waitingCount++;

        lock.lock();

        while(v != value.exchange(!v))
        {
            cond.wait(lock);
        }
        cond.notify_all();
        lock.unlock();
        waitingCount++;
    }
    void set()
    {
        *this = true;
    }
    void reset()
    {
        *this = false;
    }
};
}
}

#endif // FLAG_H_INCLUDED
