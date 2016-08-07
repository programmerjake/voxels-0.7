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
#include "util/lock.h"
#include <iostream>

namespace programmerjake
{
namespace voxels
{
void lock_hierarchy::handleError(const char *what, std::size_t lock_level, const void *theLock)
{
    variables_t &variables = get_variables();
    std::cerr << "locking hierarchy error: " << what << " lock_level=" << lock_level
              << " theLock=" << theLock << std::endl;
    std::cerr << "locked levels:\n";
    std::size_t lastV = 0, count = 0;
    const std::size_t skipAtCount = 3;
    for(std::size_t v : variables.lock_levels)
    {
        if(count == 0 || lastV != v)
        {
            if(count > skipAtCount)
            {
                std::cerr << "(" << count - skipAtCount << " not shown)\n";
            }
            lastV = v;
            count = 1;
        }
        else
        {
            count++;
        }
        if(count <= skipAtCount)
            std::cerr << v << "\n";
    }
    if(count > skipAtCount)
    {
        std::cerr << "(" << count - skipAtCount << " not shown)\n";
    }
    std::cerr << std::endl;
    std::terminate();
}
}
}

#if defined(TEST_WIN32_KEYED_EVENT_MUTEX)
#include <cstdlib>
#include <unordered_map>
#include <thread>
#include <random>

namespace programmerjake
{
namespace voxels
{
namespace
{
Mutex testMutex;
constexpr std::size_t testThreadCount = 5;
std::size_t getThreadId()
{
    static std::atomic_size_t nextThreadId(0);
    static thread_local std::size_t threadId = nextThreadId++;
    assert(threadId < testThreadCount);
    return threadId;
}
std::atomic_bool threadWaitStates[testThreadCount];
void writeThreadMessagePrefix()
{
    std::cout.width(3);
    std::cout << getThreadId() << " ";
    for(auto &threadWaitState : threadWaitStates)
    {
        if(threadWaitState.load())
            std::cout << "W";
        else
            std::cout << "R";
    }
    std::cout << " ";
}
struct KeyedEvent final
{
    std::mutex lock;
    std::condition_variable cond;
    struct WaitingThread final
    {
        bool needRelease = false;
        bool needWait = false;
        WaitingThread(bool isWait) : needRelease(isWait), needWait(!isWait)
        {
        }
        bool &getReleaseVariable(bool isWait)
        {
            if(isWait)
                return needRelease;
            return needWait;
        }
        bool &getWaitVariable(bool isWait)
        {
            if(isWait)
                return needWait;
            return needRelease;
        }
    };
    struct Counts final
    {
        std::size_t waitCount = 0;
        std::size_t releaseCount = 0;
    };
    std::unordered_map<void *, Counts> keyCountsMap;
    static std::chrono::system_clock::time_point getTimePoint(std::int64_t timeout) noexcept
    {
        if(timeout <= 0)
        {
            auto retval = std::chrono::system_clock::now();
            retval +=
                std::chrono::duration<std::int64_t, std::ratio<1, 1000LL * 1000LL * 1000LL / 100>>(
                    -timeout);
            return retval;
        }
        else
        {
            auto retval = std::chrono::system_clock::from_time_t(0);
            retval +=
                std::chrono::duration<std::int64_t, std::ratio<1, 1000LL * 1000LL * 1000LL / 100>>(
                    timeout);
            retval -= std::chrono::seconds(11644473600LL);
            return retval;
        }
    }
    static void writeCounts(Counts counts) noexcept
    {
        writeThreadMessagePrefix();
        std::cout << "counts: W:" << counts.waitCount << " R:" << counts.releaseCount << std::endl;
    }
    std::cv_status doWait(void *key, const std::int64_t *timeout, bool isWait) noexcept
    {
        std::chrono::system_clock::time_point timePoint;
        if(timeout)
            timePoint = getTimePoint(*timeout);
        std::unique_lock<std::mutex> lockIt(lock);
        threadWaitStates[getThreadId()] = true;
        writeThreadMessagePrefix();
        std::cout << "doWait(..., " << isWait << ")" << std::endl;
        auto &counts = keyCountsMap[key];
        if(isWait)
            counts.waitCount++;
        else
            counts.releaseCount++;
        writeCounts(counts);
        cond.notify_all();
        std::cv_status waitResult = std::cv_status::no_timeout;
        while(isWait ? counts.releaseCount == 0 : counts.waitCount == 0)
        {
            if(waitResult == std::cv_status::timeout)
            {
                if(isWait)
                {
                    assert(counts.waitCount != 0);
                    counts.waitCount--;
                }
                else
                {
                    assert(counts.releaseCount != 0);
                    counts.releaseCount--;
                }
                cond.notify_all();
                threadWaitStates[getThreadId()] = false;
                writeThreadMessagePrefix();
                std::cout << "doWait(..., " << isWait << ") -> timeout" << std::endl;
                writeCounts(counts);
                return std::cv_status::timeout;
            }
            if(timeout)
                waitResult = cond.wait_until(lockIt, timePoint);
            else
                cond.wait(lockIt);
        }
        waitResult = std::cv_status::no_timeout;
        if(isWait)
            counts.releaseCount--;
        else
            counts.waitCount--;
        cond.notify_all();
        threadWaitStates[getThreadId()] = false;
        writeThreadMessagePrefix();
        std::cout << "doWait(..., " << isWait << ") -> no_timeout" << std::endl;
        writeCounts(counts);
        return waitResult;
    }
};
}

checked_array<void *, Mutex::KeyedEventHandleCount> Mutex::makeGlobalKeyedEvents() noexcept
{
    checked_array<void *, KeyedEventHandleCount> retval = {};
    for(std::size_t i = 0; i < retval.size(); i++)
    {
        retval[i] = static_cast<void *>(new KeyedEvent);
    }
    return retval;
}

std::cv_status Mutex::waitForKeyedEvent(void *handle,
                                        void *key,
                                        const std::int64_t *timeout) noexcept
{
    return static_cast<KeyedEvent *>(handle)->doWait(key, timeout, true);
}

std::cv_status Mutex::releaseKeyedEvent(void *handle,
                                        void *key,
                                        const std::int64_t *timeout) noexcept
{
    return static_cast<KeyedEvent *>(handle)->doWait(key, timeout, false);
}

Mutex::threadIdValue Mutex::getCurrentThreadId() noexcept
{
    static atomicThreadId nextThreadId(emptyThreadId + 1);
    static thread_local threadIdValue retval = nextThreadId.fetch_add(1, std::memory_order_relaxed);
    assert(retval != emptyThreadId);
    return retval;
}

void Mutex::checkpoint() noexcept
{
    static std::mutex lock;
    static std::default_random_engine re;
    std::unique_lock<std::mutex> lockIt(lock);
    auto waitTime = std::chrono::milliseconds(std::uniform_int_distribution<int>(5, 20)(re));
    writeThreadMessagePrefix();
    auto stateValue = this->state.load();
    std::cout << "checkpoint owned:" << isOwned(stateValue)
              << " waiters:" << getWaiterCount(stateValue) << std::endl;
    lockIt.unlock();
    while(true)
    {
        auto t = std::chrono::steady_clock::now();
        t += waitTime;
        std::this_thread::sleep_until(t);
        if(std::chrono::steady_clock::now() - t < std::chrono::milliseconds(50))
            break;
    }
}

namespace
{
void test()
{
    for(auto &threadWaitState : threadWaitStates)
    {
        threadWaitState = false;
    }
    std::vector<std::thread> threads;
    threads.resize(testThreadCount);
    std::atomic_bool state;
    for(std::size_t threadIndex = 0; threadIndex < threads.size(); threadIndex++)
    {
        threads[threadIndex] =
            std::thread([&state, threadIndex]
                        {
                            bool locked = false;
                            std::default_random_engine re(threadIndex);
                            for(std::size_t i = 400; i > 0; i == 1 && locked ? 0 : i--)
                            {
                                bool oldLocked = locked;
                                if(locked)
                                {
                                    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                                    assert(state.exchange(false));
                                    testMutex.unlock();
                                    locked = false;
                                }
#if 1
                                else if(std::uniform_int_distribution<int>(0, 1)(re))
                                {
                                    locked = testMutex.try_lock();
                                }
#endif
                                else
                                {
                                    testMutex.lock();
                                    locked = true;
                                }
                                if(locked)
                                {
                                    assert(!state.exchange(true));
                                }
                            }
                        });
    }
    for(std::thread &thread : threads)
    {
        thread.join();
    }
}
struct Tester final
{
    Tester()
    {
        test();
        std::exit(0);
    }
} tester;
}
}
}
#ifdef TEST_WIN32_KEYED_EVENT_MUTEX_MAIN
int main()
{
}
#endif
#elif defined(_WIN32)
#include <windows.h>
#include <ntdef.h>

extern "C" {
NTSYSAPI NTSTATUS NTAPI NtCreateKeyedEvent(PHANDLE pHandle,
                                           ACCESS_MASK DesiredAccess,
                                           POBJECT_ATTRIBUTES ObjectAttributes,
                                           ULONG reserved);
NTSYSAPI NTSTATUS NTAPI
    NtWaitForKeyedEvent(HANDLE handle, PVOID key, BOOLEAN alertable, PLARGE_INTEGER timeout);
NTSYSAPI NTSTATUS NTAPI
    NtReleaseKeyedEvent(HANDLE handle, PVOID key, BOOLEAN alertable, PLARGE_INTEGER timeout);
}

namespace programmerjake
{
namespace voxels
{
namespace
{
static void handleNtapiError(NTSTATUS error) noexcept
{
    volatile NTSTATUS error2 =
        error; // for debugging; volatile so the variable doesn't get optimized away
    std::terminate();
}
}

checked_array<void *, Mutex::KeyedEventHandleCount> Mutex::makeGlobalKeyedEvents() noexcept
{
    checked_array<void *, KeyedEventHandleCount> retval = {};
    for(std::size_t i = 0; i < retval.size(); i++)
    {
        HANDLE handle{};
        auto status = ::NtCreateKeyedEvent(&handle, EVENT_ALL_ACCESS, nullptr, 0);
        if(status != STATUS_SUCCESS)
            handleNtapiError(status);
        retval[i] = static_cast<void *>(handle);
    }
    return retval;
}

std::cv_status Mutex::waitForKeyedEvent(void *handle,
                                        void *key,
                                        const std::int64_t *timeout) noexcept
{
    LARGE_INTEGER timeoutValue;
    if(timeout)
        timeoutValue.QuadPart = *timeout;
    auto status = NtWaitForKeyedEvent(
        static_cast<HANDLE>(handle), key, false, timeout ? &timeoutValue : nullptr);
    if(status == STATUS_TIMEOUT && timeout)
        return std::cv_status::timeout;
    if(status != STATUS_SUCCESS)
        handleNtapiError(status);
    return std::cv_status::no_timeout;
}

std::cv_status Mutex::releaseKeyedEvent(void *handle,
                                        void *key,
                                        const std::int64_t *timeout) noexcept
{
    LARGE_INTEGER timeoutValue;
    if(timeout)
        timeoutValue.QuadPart = *timeout;
    auto status = NtReleaseKeyedEvent(
        static_cast<HANDLE>(handle), key, false, timeout ? &timeoutValue : nullptr);
    if(status == STATUS_TIMEOUT && timeout)
        return std::cv_status::timeout;
    if(status != STATUS_SUCCESS)
        handleNtapiError(status);
    return std::cv_status::no_timeout;
}

Mutex::threadIdValue Mutex::getCurrentThreadId() noexcept
{
    return ::GetCurrentThreadId();
}
}
}
#elif defined(__linux) || defined(__APPLE__) // pthreads is fast enough
#else
#error implement Mutex
#endif
