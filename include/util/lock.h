/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef LOCK_H_INCLUDED
#define LOCK_H_INCLUDED

#include <utility>
#include <type_traits>
#include <memory>
#include <cassert>
#include <set>
#include <unordered_map>
#include <exception>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include "util/tls.h"
#include "util/checked_array.h"

#if defined(TEST_WIN32_KEYED_EVENT_MUTEX_MAIN) || (0 && defined(__CDT_PARSER__))
#define TEST_WIN32_KEYED_EVENT_MUTEX
#endif

#if defined(_WIN32) || defined(TEST_WIN32_KEYED_EVENT_MUTEX)
namespace programmerjake
{
namespace voxels
{
class ConditionVariable;
class RecursiveMutex;
class Mutex final
{
    friend class ConditionVariable;
    friend class RecursiveMutex;
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

private:
    static_assert(alignof(std::atomic_size_t) >= 2, "");
    static constexpr std::size_t KeyedEventHandleCount = (1 << 4) + 1;
    static checked_array<void *, KeyedEventHandleCount> makeGlobalKeyedEvents() noexcept;
    static const checked_array<void *, KeyedEventHandleCount> &getGlobalKeyedEvents() noexcept
    {
        static const checked_array<void *, KeyedEventHandleCount> retval = makeGlobalKeyedEvents();
        return retval;
    }
    static void *getKeyedEventHandle(void *syncPointer) noexcept
    {
        return getGlobalKeyedEvents()[std::hash<void *>()(syncPointer) % KeyedEventHandleCount];
    }
    static std::cv_status waitForKeyedEvent(void *handle,
                                            void *key,
                                            const std::int64_t *timeout) noexcept;
    static std::cv_status releaseKeyedEvent(void *handle,
                                            void *key,
                                            const std::int64_t *timeout) noexcept;
    static std::cv_status waitForKeyedEvent(void *key, const std::int64_t *timeout) noexcept
    {
        return waitForKeyedEvent(getKeyedEventHandle(key), key, timeout);
    }
    static std::cv_status releaseKeyedEvent(void *key, const std::int64_t *timeout) noexcept
    {
        return releaseKeyedEvent(getKeyedEventHandle(key), key, timeout);
    }
    typedef std::uint_least32_t threadIdValue;
    typedef std::atomic_uint_least32_t atomicThreadId;
    static constexpr threadIdValue emptyThreadId = 0;
    static threadIdValue getCurrentThreadId() noexcept;
    void checkpoint() noexcept
#ifdef TEST_WIN32_KEYED_EVENT_MUTEX
        ;
#else
    {
    }
#endif
    template <bool isRecursive>
    void lockImp(std::size_t *nestCount, atomicThreadId *owner) noexcept
    {
        assert(isRecursive == (nestCount != nullptr));
        assert(isRecursive == (owner != nullptr));
        threadIdValue myThreadId;
        if(isRecursive)
        {
            myThreadId = getCurrentThreadId();
            auto ownerValue = owner->load(std::memory_order_relaxed);
            if(ownerValue == myThreadId)
            {
                ++*nestCount;
                return;
            }
        }
        while(true)
        {
            std::size_t oldStateValue = state.load(std::memory_order_relaxed);
            std::size_t newStateValue;
            bool needWait;
            do
            {
                checkpoint();
                if(isOwned(oldStateValue))
                {
                    needWait = true;
                    newStateValue = createState(getWaiterCount(oldStateValue) + 1, true);
                }
                else
                {
                    needWait = false;
                    newStateValue = setOwned(oldStateValue, true);
                }
            } while(!state.compare_exchange_weak(oldStateValue,
                                                 newStateValue,
                                                 std::memory_order_acquire,
                                                 std::memory_order_relaxed));
            checkpoint();
            if(!needWait)
                break;
            waitForKeyedEvent(&state, nullptr);
            checkpoint();
        }
        if(isRecursive)
        {
            assert(owner->load(std::memory_order_relaxed) == emptyThreadId);
            owner->store(myThreadId, std::memory_order_relaxed);
        }
    }
    template <bool isRecursive>
    bool tryLockImp(std::size_t *nestCount, atomicThreadId *owner) noexcept
    {
        assert(isRecursive == (nestCount != nullptr));
        assert(isRecursive == (owner != nullptr));
        threadIdValue myThreadId;
        if(isRecursive)
        {
            myThreadId = getCurrentThreadId();
            auto ownerValue = owner->load(std::memory_order_relaxed);
            if(ownerValue == myThreadId)
            {
                ++*nestCount;
                return true;
            }
        }
        std::size_t oldStateValue = state.load(std::memory_order_relaxed);
        if(isOwned(oldStateValue))
            return false;
        std::size_t newStateValue = setOwned(oldStateValue, true);
        checkpoint();
        bool retval = state.compare_exchange_weak(
            oldStateValue, newStateValue, std::memory_order_acquire, std::memory_order_relaxed);
        checkpoint();
        if(retval && isRecursive)
        {
            assert(owner->load(std::memory_order_relaxed) == emptyThreadId);
            owner->store(myThreadId, std::memory_order_relaxed);
        }
        return retval;
    }
    template <bool isRecursive>
    void unlockImp(std::size_t *nestCount, atomicThreadId *owner) noexcept
    {
        assert(isRecursive == (nestCount != nullptr));
        assert(isRecursive == (owner != nullptr));
        if(isRecursive)
        {
            assert(owner->load(std::memory_order_relaxed) == getCurrentThreadId());
            if((*nestCount)-- != 0)
                return;
            owner->store(emptyThreadId, std::memory_order_relaxed);
        }
        std::size_t oldStateValue = state.load(std::memory_order_relaxed);
        std::size_t newStateValue;
        bool needRelease;
        do
        {
            checkpoint();
            assert(isOwned(oldStateValue));
            needRelease = oldStateValue != createState(0, true);
            if(needRelease)
                newStateValue = createState(getWaiterCount(oldStateValue) - 1, false);
            else
                newStateValue = createState(0, false);
        } while(!state.compare_exchange_weak(
            oldStateValue, newStateValue, std::memory_order_acquire, std::memory_order_relaxed));
        checkpoint();
        if(needRelease)
        {
            releaseKeyedEvent(&state, nullptr);
        }
    }

private:
    std::atomic_size_t state;

private:
    static constexpr bool isOwned(std::size_t stateValue) noexcept
    {
        return stateValue & 1;
    }
    static constexpr std::size_t setOwned(std::size_t stateValue, bool owned) noexcept
    {
        return (stateValue & ~static_cast<std::size_t>(1)) | owned;
    }
    static constexpr std::size_t getWaiterCount(std::size_t stateValue) noexcept
    {
        return stateValue >> 1;
    }
    static constexpr std::size_t setWaiterCount(std::size_t stateValue,
                                                std::size_t waiterCount) noexcept
    {
        return (stateValue & 1) | (waiterCount << 1);
    }
    static constexpr std::size_t createState(std::size_t waiterCount, bool owned) noexcept
    {
        return (waiterCount << 1) | owned;
    }

public:
    constexpr Mutex() noexcept : state(createState(0, false))
    {
    }
    ~Mutex()
    {
        assert(state.load(std::memory_order_relaxed) == createState(0, false));
    }
    void lock() noexcept
    {
        lockImp<false>(nullptr, nullptr);
    }
    bool try_lock() noexcept
    {
        return tryLockImp<false>(nullptr, nullptr);
    }
    void unlock() noexcept
    {
        unlockImp<false>(nullptr, nullptr);
    }
};

class ConditionVariable final
{
    ConditionVariable(const ConditionVariable &) = delete;
    ConditionVariable &operator=(const ConditionVariable &) = delete;

private:
    struct Waiter final
    {
        Waiter *prev = nullptr;
        Waiter *next = nullptr;
        void insert(Waiter *&waiterListHead, Waiter *&waiterListTail) noexcept
        {
            assert(prev == nullptr);
            assert(next == nullptr);
            assert(waiterListHead != this);
            next = waiterListHead;
            if(next)
            {
                assert(next->prev == nullptr);
                next->prev = this;
            }
            else
            {
                assert(waiterListTail == nullptr);
                waiterListTail = this;
            }
            waiterListHead = this;
        }
        void remove(Waiter *&waiterListHead, Waiter *&waiterListTail) noexcept
        {
            assert(prev != nullptr || waiterListHead == this);
            assert(next != nullptr || waiterListTail == this);
            assert(prev != next);
            if(prev)
            {
                assert(prev->next == this);
                prev->next = next;
            }
            else
            {
                waiterListHead = next;
            }
            if(next)
            {
                assert(next->prev == this);
                next->prev = prev;
            }
            else
            {
                waiterListTail = prev;
            }
        }
        bool inList(Waiter *waiterListHead, Waiter *waiterListTail) const noexcept
        {
            return waiterListHead == this || prev != nullptr || next != nullptr;
        }
    };

private:
    Mutex lock;
    Waiter *waiterListHead;
    Waiter *waiterListTail;
    template <typename Duration>
    static std::int64_t convertToNTTime(
        const std::chrono::time_point<std::chrono::system_clock, Duration> &time) noexcept
    {
        std::chrono::duration<std::int64_t, std::ratio<1, 1000LL * 1000LL * 1000LL / 100>>
            duration = std::chrono::
                duration_cast<std::chrono::duration<std::int64_t,
                                                    std::ratio<1, 1000LL * 1000LL * 1000LL / 100>>>(
                    time - std::chrono::system_clock::from_time_t(0));
        duration += std::chrono::seconds(11644473600LL);
        return duration.count();
    }
    template <typename Rep, typename Period>
    static std::int64_t convertToNTTime(const std::chrono::duration<Rep, Period> &time) noexcept
    {
        return convertToNTTime(time + std::chrono::system_clock::now());
    }
    template <
        typename Clock,
        typename Duration,
        typename =
            typename std::enable_if<!std::is_same<Clock, std::chrono::system_clock>::value>::type>
    static std::int64_t convertToNTTime(
        const std::chrono::time_point<Clock, Duration> &time) noexcept
    {
        return convertToNTTime(time - Clock::now());
    }
    std::cv_status waitImp(std::unique_lock<Mutex> &theLock, const std::int64_t *timeout) noexcept
    {
        Waiter waiter;
        lock.lock();
        theLock.unlock();
        waiter.insert(waiterListHead, waiterListTail);
        lock.unlock();
        Mutex::waitForKeyedEvent(static_cast<void *>(&waiter), timeout);
        std::cv_status retval = std::cv_status::no_timeout;
        if(timeout)
        {
            lock.lock();
            if(waiter.inList(waiterListHead, waiterListTail))
            {
                retval = std::cv_status::timeout;
                waiter.remove(waiterListHead, waiterListTail);
            }
            lock.unlock();
        }
        theLock.lock();
        return retval;
    }
    void notifyImp(bool notifyAll) noexcept
    {
        lock.lock();
        if(waiterListHead != nullptr)
        {
            do
            {
                auto waiter = waiterListHead;
                waiter->remove(waiterListHead, waiterListTail);
                Mutex::releaseKeyedEvent(static_cast<void *>(&waiter), nullptr);
            } while(notifyAll && waiterListHead != nullptr);
        }
        lock.unlock();
    }
    std::cv_status timedWait(std::unique_lock<Mutex> &theLock, std::int64_t timeout) noexcept
    {
        return waitImp(theLock, &timeout);
    }
    bool haveAnyWaiters() noexcept
    {
        lock.lock();
        if(waiterListHead)
        {
            lock.unlock();
            return true;
        }
        lock.unlock();
        return false;
    }

public:
    constexpr ConditionVariable() noexcept : lock(),
                                             waiterListHead(nullptr),
                                             waiterListTail(nullptr)
    {
    }
    ~ConditionVariable()
    {
        assert(!haveAnyWaiters());
    }
    void notify_one() noexcept
    {
        notifyImp(false);
    }
    void notify_all() noexcept
    {
        notifyImp(true);
    }
    void wait(std::unique_lock<Mutex> &theLock) noexcept
    {
        waitImp(theLock, nullptr);
    }
    template <typename Fn>
    void wait(std::unique_lock<Mutex> &theLock, Fn fn) noexcept(noexcept(static_cast<bool>(fn())))
    {
        while(!static_cast<bool>(fn()))
            wait(theLock);
    }
    template <typename Rep, typename Period>
    std::cv_status wait_for(std::unique_lock<Mutex> &theLock,
                            const std::chrono::duration<Rep, Period> &time) noexcept
    {
        return timedWait(theLock, convertToNTTime(time));
    }
    template <typename Clock, typename Duration>
    std::cv_status wait_until(std::unique_lock<Mutex> &theLock,
                              const std::chrono::time_point<Clock, Duration> &time) noexcept
    {
        return timedWait(theLock, convertToNTTime(time));
    }
    template <typename Rep, typename Period, typename Fn>
    std::cv_status wait_for(std::unique_lock<Mutex> &theLock,
                            const std::chrono::duration<Rep, Period> &time,
                            Fn fn) noexcept(noexcept(static_cast<bool>(fn())))
    {
        if(!static_cast<bool>(fn()))
        {
            auto ntTime = convertToNTTime(time);
            do
            {
                if(timedWait(theLock, ntTime) == std::cv_status::timeout)
                    return static_cast<bool>(fn());
            } while(!static_cast<bool>(fn()));
        }
        return true;
    }
    template <typename Clock, typename Duration, typename Fn>
    bool wait_until(std::unique_lock<Mutex> &theLock,
                    const std::chrono::time_point<Clock, Duration> &time,
                    Fn fn) noexcept(noexcept(static_cast<bool>(fn())))
    {
        if(!static_cast<bool>(fn()))
        {
            auto ntTime = convertToNTTime(time);
            do
            {
                if(timedWait(theLock, ntTime) == std::cv_status::timeout)
                    return static_cast<bool>(fn());
            } while(!static_cast<bool>(fn()));
        }
        return true;
    }
};

class RecursiveMutex final
{
    RecursiveMutex(const RecursiveMutex &) = delete;
    RecursiveMutex &operator=(const RecursiveMutex &) = delete;

private:
    Mutex baseLock;
    std::size_t nestCount;
    Mutex::atomicThreadId owner;

public:
    RecursiveMutex() noexcept : baseLock(), nestCount(0), owner(Mutex::emptyThreadId)
    {
    }
    void lock() noexcept
    {
        baseLock.lockImp<true>(&nestCount, &owner);
    }
    bool try_lock() noexcept
    {
        return baseLock.tryLockImp<true>(&nestCount, &owner);
    }
    void unlock() noexcept
    {
        baseLock.unlockImp<true>(&nestCount, &owner);
    }
};
}
}
#elif defined(__linux) || defined(__APPLE__) // pthreads is fast enough
#include <pthread.h>
namespace programmerjake
{
namespace voxels
{
class ConditionVariable;
class RecursiveMutex;
class Mutex final
{
    friend class ConditionVariable;
    friend class RecursiveMutex;
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

private:
    ::pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    static void handlePThreadsError(int error) noexcept
    {
        volatile int error2 =
            error; // for debugging; volatile so the variable doesn't get optimized away
        static_cast<void>(error2);
        std::terminate();
    }
    template <int mutexType>
    static const pthread_mutexattr_t *createMutexAttr() noexcept
    {
        static pthread_mutexattr_t retval;
        int error = ::pthread_mutexattr_init(&retval);
        if(error != 0)
            handlePThreadsError(error);
        error = ::pthread_mutexattr_settype(&retval, mutexType);
        if(error != 0)
            handlePThreadsError(error);
        return &retval;
    }
    template <int mutexType>
    static const pthread_mutexattr_t *getMutexAttr() noexcept
    {
        static const pthread_mutexattr_t *retval = createMutexAttr<mutexType>();
        return retval;
    }

public:
    constexpr Mutex() = default;
    ~Mutex()
    {
        int error = ::pthread_mutex_destroy(&mutex);
        if(error != 0)
            handlePThreadsError(error);
    }
    void lock() noexcept
    {
        int error = ::pthread_mutex_lock(&mutex);
        if(error != 0)
            handlePThreadsError(error);
    }
    bool try_lock() noexcept
    {
        int error = ::pthread_mutex_trylock(&mutex);
        if(error == 0)
            return true;
        if(error != EBUSY)
            handlePThreadsError(error);
        return false;
    }
    void unlock() noexcept
    {
        int error = ::pthread_mutex_unlock(&mutex);
        if(error != 0)
            handlePThreadsError(error);
    }
};

class RecursiveMutex final
{
    RecursiveMutex(const RecursiveMutex &) = delete;
    RecursiveMutex &operator=(const RecursiveMutex &) = delete;

private:
    ::pthread_mutex_t mutex;

public:
    RecursiveMutex()
    {
        int error = ::pthread_mutex_init(&mutex, Mutex::getMutexAttr<PTHREAD_MUTEX_RECURSIVE>());
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
    ~RecursiveMutex()
    {
        int error = ::pthread_mutex_destroy(&mutex);
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
    void lock() noexcept
    {
        int error = ::pthread_mutex_lock(&mutex);
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
    bool try_lock() noexcept
    {
        int error = ::pthread_mutex_trylock(&mutex);
        if(error == 0)
            return true;
        if(error != EBUSY)
            Mutex::handlePThreadsError(error);
        return false;
    }
    void unlock() noexcept
    {
        int error = ::pthread_mutex_unlock(&mutex);
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
};

class ConditionVariable final
{
    ConditionVariable(const ConditionVariable &) = delete;
    ConditionVariable &operator=(const ConditionVariable &) = delete;

private:
    ::pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    template <typename Duration>
    static ::timespec convertToTimespec(
        const std::chrono::time_point<std::chrono::system_clock, Duration> &time) noexcept
    {
        auto duration = time - std::chrono::system_clock::from_time_t(0);
        std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        duration -= seconds;
        ::timespec retval;
        retval.tv_sec = static_cast<std::time_t>(seconds.count());
        retval.tv_nsec =
            static_cast<long>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
        if(retval.tv_nsec < 0)
        {
            retval.tv_nsec += 1000000000L;
            retval.tv_sec--;
        }
        else if(retval.tv_nsec >= 1000000000L)
        {
            retval.tv_nsec -= 1000000000L;
            retval.tv_sec++;
        }
        return retval;
    }
    template <typename Rep, typename Period>
    static ::timespec convertToTimespec(const std::chrono::duration<Rep, Period> &time) noexcept
    {
        return convertToTimespec(time + std::chrono::system_clock::now());
    }
    template <
        typename Clock,
        typename Duration,
        typename =
            typename std::enable_if<!std::is_same<Clock, std::chrono::system_clock>::value>::type>
    static ::timespec convertToTimespec(
        const std::chrono::time_point<Clock, Duration> &time) noexcept
    {
        return convertToTimespec(time - Clock::now());
    }
    std::cv_status timedWait(std::unique_lock<Mutex> &theLock, const ::timespec &timespec) noexcept
    {
        int error = ::pthread_cond_timedwait(&condition, &theLock.mutex()->mutex, &timespec);
        if(error == ETIMEDOUT)
            return std::cv_status::timeout;
        if(error != 0)
            Mutex::handlePThreadsError(error);
        return std::cv_status::no_timeout;
    }

public:
    constexpr ConditionVariable() = default;
    ~ConditionVariable()
    {
        int error = ::pthread_cond_destroy(&condition);
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
    void notify_one() noexcept
    {
        int error = ::pthread_cond_signal(&condition);
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
    void notify_all() noexcept
    {
        int error = ::pthread_cond_broadcast(&condition);
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
    void wait(std::unique_lock<Mutex> &theLock) noexcept
    {
        int error = ::pthread_cond_wait(&condition, &theLock.mutex()->mutex);
        if(error != 0)
            Mutex::handlePThreadsError(error);
    }
    template <typename Fn>
    void wait(std::unique_lock<Mutex> &theLock, Fn fn) noexcept(noexcept(static_cast<bool>(fn())))
    {
        while(!static_cast<bool>(fn()))
            wait(theLock);
    }
    template <typename Rep, typename Period>
    std::cv_status wait_for(std::unique_lock<Mutex> &theLock,
                            const std::chrono::duration<Rep, Period> &time) noexcept
    {
        return timedWait(theLock, convertToTimespec(time));
    }
    template <typename Clock, typename Duration>
    std::cv_status wait_until(std::unique_lock<Mutex> &theLock,
                              const std::chrono::time_point<Clock, Duration> &time) noexcept
    {
        return timedWait(theLock, convertToTimespec(time));
    }
    template <typename Rep, typename Period, typename Fn>
    std::cv_status wait_for(std::unique_lock<Mutex> &theLock,
                            const std::chrono::duration<Rep, Period> &time,
                            Fn fn) noexcept(noexcept(static_cast<bool>(fn())))
    {
        if(!static_cast<bool>(fn()))
        {
            auto timespec = convertToTimespec(time);
            do
            {
                if(timedWait(theLock, timespec) == std::cv_status::timeout)
                    return static_cast<bool>(fn());
            } while(!static_cast<bool>(fn()));
        }
        return true;
    }
    template <typename Clock, typename Duration, typename Fn>
    bool wait_until(std::unique_lock<Mutex> &theLock,
                    const std::chrono::time_point<Clock, Duration> &time,
                    Fn fn) noexcept(noexcept(static_cast<bool>(fn())))
    {
        if(!static_cast<bool>(fn()))
        {
            auto timespec = convertToTimespec(time);
            do
            {
                if(timedWait(theLock, timespec) == std::cv_status::timeout)
                    return static_cast<bool>(fn());
            } while(!static_cast<bool>(fn()));
        }
        return true;
    }
};
}
}
#else
#error implement Mutex and ConditionVariable
#endif

namespace programmerjake
{
namespace voxels
{
template <typename T>
void lock_all(T begin, T end)
{
    auto start_lock = begin;
    if(begin == end) // lock nothing
        return;
retry:
    start_lock->lock();
    for(auto i = begin; i != end; i++)
    {
        if(start_lock == i)
            continue;
        try
        {
            if(!i->try_lock())
            {
                for(auto j = begin; j != i; j++)
                {
                    if(start_lock == j)
                        continue;
                    j->unlock();
                }
                start_lock->unlock();
                start_lock = i;
                goto retry;
            }
        }
        catch(...)
        {
            for(auto j = begin; j != i; j++)
            {
                if(start_lock == j)
                    continue;
                j->unlock();
            }
            start_lock->unlock();
            throw;
        }
    }
}

template <typename T>
bool try_lock_all(T begin, T end)
{
    for(auto i = begin; i != end; i++)
    {
        try
        {
            if(!i->try_lock())
            {
                for(auto j = begin; j != i; j++)
                {
                    j->unlock();
                }
                return false;
            }
        }
        catch(...)
        {
            for(auto j = begin; j != i; j++)
            {
                j->unlock();
            }
            throw;
        }
    }
    return true;
}

class generic_lock_wrapper final
{
private:
    void *pLock;
    void (*lockFn)(void *pLock);
    bool (*try_lockFn)(void *pLock);
    void (*unlockFn)(void *pLock);

public:
    template <typename T>
    explicit generic_lock_wrapper(T &theLock)
        : pLock(static_cast<void *>(std::addressof(theLock))), lockFn(), try_lockFn(), unlockFn()
    {
        lockFn = [](void *pLock) -> void
        {
            static_cast<T *>(pLock)->lock();
        };
        try_lockFn = [](void *pLock) -> bool
        {
            return static_cast<T *>(pLock)->try_lock();
        };
        unlockFn = [](void *pLock) -> void
        {
            static_cast<T *>(pLock)->unlock();
        };
    }
    void lock()
    {
        lockFn(pLock);
    }
    bool try_lock()
    {
        return try_lockFn(pLock);
    }
    void unlock()
    {
        unlockFn(pLock);
    }
};

class lock_hierarchy final
{
private:
    struct variables_t final
    {
        std::multiset<std::size_t> lock_levels;
        std::unordered_map<const void *, std::size_t> locks;
        variables_t() : lock_levels(), locks()
        {
        }
    };
    static variables_t &get_variables()
    {
        struct retvalTag
        {
        };
        thread_local_variable<variables_t, retvalTag> retval(TLS::getSlow());
        return retval.get();
    }
    static void handleError(const char *what, std::size_t lock_level, const void *theLock);

public:
    static void check_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        if(variables.locks[theLock] != 0)
        {
            handleError("check_lock: already locked", lock_level, theLock);
        }
        auto iter = variables.lock_levels.lower_bound(lock_level);
        if(iter != variables.lock_levels.end())
        {
            handleError("check_lock: hierarchy violation", lock_level, theLock);
        }
    }
    static void check_recursive_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        if(variables.locks[theLock] != 0)
        {
            return; // already locked
        }
        auto iter = variables.lock_levels.lower_bound(lock_level);
        if(iter != variables.lock_levels.end())
        {
            handleError("check_recursive_lock: hierarchy violation", lock_level, theLock);
        }
    }
    static void add_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        if(variables.locks[theLock]++ == 0)
            variables.lock_levels.insert(lock_level);
    }
    static void remove_lock(std::size_t lock_level, const void *theLock)
    {
        variables_t &variables = get_variables();
        auto iter = variables.lock_levels.find(lock_level);
        if(iter == variables.lock_levels.end())
        {
            handleError("remove_lock: unlock of unlocked lock", lock_level, theLock);
        }
        if(--variables.locks[theLock] == 0)
        {
            variables.lock_levels.erase(iter);
        }
    }
};

template <std::size_t level>
class checked_lock final
{
    Mutex theLock;

public:
    checked_lock() : theLock()
    {
    }
    static constexpr std::size_t lock_level = level;
#ifdef DEBUG_LOCKS
    static constexpr bool DoCheck = true;
#else
    static constexpr bool DoCheck = false;
#endif
    void lock()
    {
        if(DoCheck)
        {
            lock_hierarchy::check_lock(lock_level, this);
            theLock.lock();
            lock_hierarchy::add_lock(lock_level, this);
        }
        else
            theLock.lock();
    }
    bool try_lock()
    {
        if(DoCheck)
        {
            if(theLock.try_lock())
            {
                lock_hierarchy::add_lock(lock_level, this);
                return true;
            }
            return false;
        }
        else
            return theLock.try_lock();
    }
    void unlock()
    {
        if(DoCheck)
        {
            theLock.unlock();
            lock_hierarchy::remove_lock(lock_level, this);
        }
        else
            theLock.unlock();
    }
};

template <std::size_t level>
class checked_recursive_lock final
{
    RecursiveMutex theLock;

public:
    checked_recursive_lock() : theLock()
    {
    }
    static constexpr std::size_t lock_level = level;
#ifdef DEBUG_LOCKS
    static constexpr bool DoCheck = true;
#else
    static constexpr bool DoCheck = false;
#endif
    void lock()
    {
        if(DoCheck)
        {
            lock_hierarchy::check_recursive_lock(lock_level, this);
            theLock.lock();
            lock_hierarchy::add_lock(lock_level, this);
        }
        else
            theLock.lock();
    }
    bool try_lock()
    {
        if(DoCheck)
        {
            if(theLock.try_lock())
            {
                lock_hierarchy::add_lock(lock_level, this);
                return true;
            }
            return false;
        }
        else
            return theLock.try_lock();
    }
    void unlock()
    {
        if(DoCheck)
        {
            theLock.unlock();
            lock_hierarchy::remove_lock(lock_level, this);
        }
        else
            theLock.unlock();
    }
};
}
}

#endif // LOCK_H_INCLUDED
