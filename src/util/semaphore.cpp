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

#include "util/semaphore.h"
#include <thread>
#include "util/util.h"
#include <vector>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <deque>
#include "util/checked_array.h"
#include <random>
#include <sstream>

namespace programmerjake
{
namespace voxels
{
Semaphore::Semaphore(std::size_t count) : atomicCount(count), stateLock(), stateCond()
{
}

std::size_t Semaphore::getCount()
{
    std::size_t retval = atomicCount.load(std::memory_order_relaxed);
    if(retval != 0)
        return retval;
    std::unique_lock<std::mutex> lockedState(stateLock);
    return atomicCount.load(std::memory_order_relaxed);
}

bool Semaphore::try_lock(std::size_t lockCount)
{
    std::size_t expected = atomicCount.load(std::memory_order_relaxed);
    if(expected != 0)
    {
        if(expected < lockCount)
            return false;
        if(atomicCount.compare_exchange_strong(
               expected, expected - lockCount, std::memory_order_acq_rel))
            return true;
    }
    std::unique_lock<std::mutex> lockedState(stateLock, std::try_to_lock);
    if(!lockedState.owns_lock())
        return false;
    std::size_t count = atomicCount.exchange(0, std::memory_order_acq_rel);

    bool retval = false;
    if(count >= lockCount)
    {
        count -= lockCount;
        retval = true;
    }
    atomicCount.store(count, std::memory_order_release);

    return retval;
}

void Semaphore::lock(std::size_t lockCount)
{
    std::size_t expected = atomicCount.load(std::memory_order_relaxed);
    if(expected != 0 && expected >= lockCount)
    {
        if(atomicCount.compare_exchange_strong(
               expected, expected - lockCount, std::memory_order_acq_rel))
            return;
    }
    std::unique_lock<std::mutex> lockedState(stateLock);
    std::size_t count = atomicCount.exchange(0, std::memory_order_acq_rel);
    while(lockCount > 0)
    {
        while(count == 0)
        {
            stateCond.wait(lockedState);
            count = atomicCount.exchange(0, std::memory_order_acq_rel);
        }
        if(count >= lockCount)
        {
            count -= lockCount;
            atomicCount.store(count, std::memory_order_release);
            break;
        }
        else
        {
            lockCount -= count;
            count = 0;
        }
    }
}

void Semaphore::unlock(std::size_t unlockCount)
{
    if(unlockCount == 0)
        return;

    std::size_t expected = atomicCount.load(std::memory_order_relaxed);
    if(expected != 0)
    {
        if(atomicCount.compare_exchange_strong(
               expected, expected + unlockCount, std::memory_order_acq_rel))
            return;
    }

    std::unique_lock<std::mutex> lockedState(stateLock);
    std::size_t count = atomicCount.exchange(0, std::memory_order_acq_rel);
    if(count <= 0)
    {
        stateCond.notify_all();
    }
    count += unlockCount;
    atomicCount.store(count, std::memory_order_release);
}

#if 0 // simple test
namespace
{
void printMessageHelper()
{
    std::cout << std::endl;
}

template <typename T, typename... Args>
void printMessageHelper(T &&arg1, Args &&... args)
{
    std::cout << std::forward<T>(arg1);
    printMessageHelper(std::forward<Args>(args)...);
}

template <typename... Args>
void printMessage(Args &&... args)
{
    static std::mutex lock;
    std::unique_lock<std::mutex> lockIt(lock);
    printMessageHelper(std::forward<Args>(args)...);
}

initializer init1([]()
                  {
                      Semaphore semaphore(10);
                      std::vector<std::thread> threads;
                      std::condition_variable cond;
                      std::mutex lock;
                      for(std::size_t i = 0; i < 100; i++)
                      {
                          threads.push_back(std::thread([&semaphore, i, &cond, &lock]()
                                                        {
                                                            for(std::size_t j = 0; j < 100; j++)
                                                            {
                                                                printMessage(i, ": before lock");
                                                                semaphore.lock();
                                                                printMessage(i, ": after lock");
                                                                printMessage(i, ": unlock");
                                                                semaphore.unlock();
                                                            }
                                                        }));
                      }

                      for(std::size_t j = 0; j < 100; j++)
                      {
                          printMessage("before lock(10)");
                          semaphore.lock(10);
                          printMessage("after lock(10)");
                          printMessage("unlock(10)");
                          semaphore.unlock(10);
                      }

                      while(!threads.empty())
                      {
                          threads.back().join();
                          threads.pop_back();
                      }
                      std::exit(0);
                  });
}
#elif 0 // complex simulation of all paths through Semaphore; inspired by Klee
        // (https://klee.github.io/)
namespace
{
constexpr std::size_t maxConcurrentStateCount = 1000000; // increase to simulate more paths
struct SemaphoreTester final
{
    static int test();
    SemaphoreTester()
    {
        std::exit(test());
    }
};

SemaphoreTester semaphoreTester;

struct State final
{
    typedef std::uint8_t ThreadId;
    static constexpr ThreadId threadCount = 3;
    static constexpr std::size_t startingCount = threadCount - 1;
    static std::size_t getLockCount(ThreadId threadId)
    {
        if(threadId == threadCount - 1)
            return startingCount;
        return 1;
    }
    std::vector<ThreadId> blockedOnMutexThreads;
    std::vector<ThreadId> blockedOnConditionVariableThreads;
    std::vector<ThreadId> schedulingDecisions;
    checked_array<std::size_t, threadCount> threadStateState;
    checked_array<std::size_t, threadCount> threadStateI;
    checked_array<std::size_t, threadCount> threadStateLockCount;
    std::size_t count = startingCount;
    State()
    {
        threadStateState.fill(0);
        blockedOnMutexThreads.reserve(threadCount);
        blockedOnConditionVariableThreads.reserve(threadCount);
        for(std::size_t i = 0; i < threadCount; i++)
            blockedOnMutexThreads.push_back(i);
    }
    void step(ThreadId threadId, std::ostream *trace = nullptr)
    {
        schedulingDecisions.push_back(threadId);
        for(auto iter = blockedOnMutexThreads.begin(); iter != blockedOnMutexThreads.end(); ++iter)
        {
            if(*iter == threadId)
            {
                blockedOnMutexThreads.erase(iter);
                break;
            }
        }
        std::size_t &state = threadStateState[threadId];
        std::size_t &i = threadStateI[threadId];
        std::size_t &lockCount = threadStateLockCount[threadId];
        switch(state)
        {
        case 0:
        {
            for(i = 0; i < 3; i++)
            {
                lockCount = getLockCount(threadId);
                if(trace)
                    *trace << static_cast<std::size_t>(threadId) << ": i = " << i << "\n"
                           << static_cast<std::size_t>(threadId) << ": lock(" << lockCount << ")\n"
                           << static_cast<std::size_t>(threadId) << ": lock mutex\n";
                state = 1;
                blockedOnMutexThreads.push_back(threadId);
                return;
            case 1:
                if(trace)
                    *trace << static_cast<std::size_t>(threadId) << ": lock mutex finished\n";

                while(lockCount > 0)
                {
                    while(count <= 0)
                    {
                        if(trace)
                            *trace << static_cast<std::size_t>(threadId) << ": wait variable\n";
                        state = 2;
                        blockedOnConditionVariableThreads.push_back(threadId);
                        return;
                    case 2:
                        if(trace)
                            *trace << static_cast<std::size_t>(threadId) << ": wait finished\n";
                    }
                    if(count >= lockCount)
                    {
                        count -= lockCount;
                        if(trace)
                            *trace << static_cast<std::size_t>(threadId) << ": count = " << count
                                   << "\n";
                        break;
                    }
                    else
                    {
                        lockCount -= count;
                        count = 0;
                        if(trace)
                            *trace << static_cast<std::size_t>(threadId)
                                   << ": lockCount = " << lockCount << "\n"
                                   << static_cast<std::size_t>(threadId) << ": count = " << count
                                   << "\n";
                    }
                }

                lockCount = getLockCount(threadId);
                if(trace)
                    *trace << static_cast<std::size_t>(threadId) << ": unlock mutex\n"
                           << static_cast<std::size_t>(threadId) << ": unlock(" << lockCount
                           << ")\n" << static_cast<std::size_t>(threadId) << ": lock mutex\n";
                state = 3;
                blockedOnMutexThreads.push_back(threadId);
                return;
            case 3:
                if(trace)
                    *trace << static_cast<std::size_t>(threadId) << ": lock mutex finished\n";
                if(count <= 0)
                {
                    if(trace)
                        *trace << static_cast<std::size_t>(threadId) << ": notify all\n";
                    while(!blockedOnConditionVariableThreads.empty())
                    {
                        blockedOnMutexThreads.push_back(blockedOnConditionVariableThreads.back());
                        blockedOnConditionVariableThreads.pop_back();
                    }
                }
                count += lockCount;
                if(trace)
                    *trace << static_cast<std::size_t>(threadId) << ": count = " << count << "\n"
                           << static_cast<std::size_t>(threadId) << ": unlock mutex\n";
            }
            state = 0;
            if(trace)
                *trace << static_cast<std::size_t>(threadId) << ": terminate thread\n";
            return;
        }
        }
        assert(false);
    }
    bool isDeadlocked() const
    {
        if(!blockedOnMutexThreads.empty())
            return false;
        if(!blockedOnConditionVariableThreads.empty())
            return true;
        return false;
    }
    bool isSequenceTooLong(std::size_t maxSequenceLength) const
    {
        return schedulingDecisions.size() > maxSequenceLength;
    }
    bool isTerminated() const
    {
        return blockedOnMutexThreads.empty() && blockedOnConditionVariableThreads.empty();
    }
    bool isAtStartingCount() const
    {
        return count == startingCount;
    }
    void dump() const
    {
        State state;
        const std::size_t columnWidth = 22;
        const std::size_t columnSpacing = 2;
        const std::size_t totalColumnWidth = columnWidth + columnSpacing;
        for(ThreadId threadId : schedulingDecisions)
        {
            std::ostringstream ss;
            state.step(threadId, &ss);
            std::size_t currentWidth = 0;
            for(char ch : ss.str())
            {
                if(ch == '\n')
                {
                    if(currentWidth != 0)
                        std::cout << "\n";
                    currentWidth = 0;
                    continue;
                }
                if(currentWidth == 0)
                {
                    for(std::size_t i = 0;
                        i < totalColumnWidth * static_cast<std::size_t>(threadId);
                        i++)
                        std::cout << ' ';
                }
                else if(currentWidth >= columnWidth)
                {
                    continue;
                }
                currentWidth++;
                std::cout << ch;
            }
            if(currentWidth != 0)
                std::cout << "\n";
        }
        std::cout << std::endl;
    }
};

int SemaphoreTester::test()
{
    std::deque<State> states;
    states.emplace_back();
    std::default_random_engine re;
    std::size_t loopsTillStatusReport = 0;
    while(!states.empty())
    {
        if(loopsTillStatusReport-- == 0)
        {
            std::cout << "status: state count = " << states.size()
                      << "  sequence length = " << states.front().schedulingDecisions.size()
                      << std::endl;
            loopsTillStatusReport = 1000000;
        }
        State state = std::move(states.front());
        states.pop_front();
        if(state.isDeadlocked())
        {
            std::cout << "deadlock!!!\n";
            states.clear();
            state.dump();
            return 1;
        }
        else if(state.isTerminated())
        {
            if(state.isAtStartingCount())
            {
                if(states.empty())
                {
                    std::cout << "success.\n";
                    state.dump();
                    return 0;
                }
                continue;
            }
            std::cout << "invalid ending count!!!\n";
            states.clear();
            state.dump();
            return 1;
        }
        if(state.isSequenceTooLong(1000))
            continue;
        if(states.size() >= 10000000)
        {
            std::size_t index = 0;
            if(state.blockedOnMutexThreads.size() > 1)
                index = std::uniform_int_distribution<std::size_t>(
                    0, state.blockedOnMutexThreads.size() - 1)(re);
            State::ThreadId threadId = state.blockedOnMutexThreads[index];
            state.step(threadId);
            states.push_back(std::move(state));
        }
        else
        {
            for(std::size_t index = 0; index < state.blockedOnMutexThreads.size(); index++)
            {
                State::ThreadId threadId = state.blockedOnMutexThreads[index];
                if(index == state.blockedOnMutexThreads.size() - 1)
                {
                    state.step(threadId);
                    states.push_back(std::move(state));
                    break;
                }
                else
                {
                    states.push_back(state);
                    states.back().step(threadId);
                }
            }
        }
    }
    return 0;
}
}
#endif
}
}
