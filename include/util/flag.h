#ifndef FLAG_H_INCLUDED
#define FLAG_H_INCLUDED

#include <mutex>
#include <atomic>
#include <condition_variable>

using namespace std;

class flag final
{
private:
    mutex lock;
    condition_variable_any cond;
    atomic_bool value;
public:
    flag(bool value = false)
        : value(value)
    {
    }
    const flag &operator =(bool v)
    {
        if(value.exchange(v) != v)
        {
            cond.notify_all();
        }

        return *this;
    }
    bool exchange(bool v)
    {
        bool retval = value.exchange(v);

        if(retval != v)
        {
            cond.notify_all();
        }

        return retval;
    }
    operator bool()
    {
        bool retval = value;
        return retval;
    }
    bool operator !()
    {
        bool retval = value;
        return !retval;
    }
    void wait(bool v = true) /// waits until value == v
    {
        if(v == value)
        {
            return;
        }

        lock.lock();

        while(v != value)
        {
            cond.wait(lock);
        }

        lock.unlock();
    }
    void waitThenReset(bool v = true) /// waits until value == v then set value to !v
    {
        if(v == value.exchange(!v))
        {
            cond.notify_all();
            return;
        }

        lock.lock();

        while(v != value.exchange(!v))
        {
            cond.wait(lock);
        }
        cond.notify_all();
        lock.unlock();
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

#endif // FLAG_H_INCLUDED
