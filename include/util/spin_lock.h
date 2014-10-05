#ifndef SPIN_LOCK_H_INCLUDED
#define SPIN_LOCK_H_INCLUDED

#include <atomic>
#include <thread>

struct spin_lock final
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    spin_lock()
    {
    }
    bool try_lock()
    {
        if(flag.test_and_set(std::memory_order_acquire))
            return false;
        return true;
    }
    void lock()
    {
        size_t count = 0;
        constexpr size_t skipCount = 1000;
        while(flag.test_and_set(std::memory_order_acquire))
        {
            if(count++ >= skipCount)
            {
                std::this_thread::yield();
                count = skipCount;
            }
        }
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

#endif // SPIN_LOCK_H_INCLUDED
