#ifndef SPIN_LOCK_H_INCLUDED
#define SPIN_LOCK_H_INCLUDED

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

struct simple_spin_lock final
{
    std::atomic_bool flag;
    simple_spin_lock()
        : flag(false)
    {
    }
    bool try_lock()
    {
        if(flag.load(std::memory_order_relaxed))
            return false;
        if(flag.exchange(true, std::memory_order_acquire))
            return false;
        return true;
    }
    void lock()
    {
        for(;;)
        {
            while(flag.load(std::memory_order_relaxed))
            {
            }
            if(!flag.exchange(true, std::memory_order_acquire))
                break;
        }
    }
    void unlock()
    {
        flag.store(false, std::memory_order_release);
    }
};

struct spin_lock final
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    std::atomic_ulong waitingCount;
    std::condition_variable cond;
    std::mutex lockMutex;
    spin_lock()
        : waitingCount(0)
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
        waitingCount++;
        while(flag.test_and_set(std::memory_order_acquire))
        {
            if(count++ >= skipCount)
            {
                std::unique_lock<std::mutex> lockIt(lockMutex);
                cond.wait(lockIt);
                count = skipCount;
            }
        }
        waitingCount--;
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
        if(waitingCount > 0)
            cond.notify_one();
    }
};

#endif // SPIN_LOCK_H_INCLUDED
