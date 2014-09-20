#ifndef UNLOCK_GUARD_H_INCLUDED
#define UNLOCK_GUARD_H_INCLUDED

#include <mutex>

template <typename T>
class unlock_guard
{
    T * plock;
    unlock_guard(const unlock_guard &) = delete;
    const unlock_guard & operator =(const unlock_guard &) = delete;
public:
    explicit unlock_guard(T & lock)
        : plock(&lock)
    {
        plock->unlock();
    }
    unlock_guard(T & lock, std::adopt_lock_t)
        : plock(&lock)
    {
    }
    unlock_guard(unlock_guard && rt)
        : plock(rt.plock)
    {
        rt.plock = nullptr;
    }
    ~unlock_guard()
    {
        if(plock != nullptr)
            plock->lock();
    }
};

#endif // UNLOCK_GUARD_H_INCLUDED
