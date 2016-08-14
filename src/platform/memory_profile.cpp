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
#include <cstdlib>
#include <new>
#include <cstddef>
#include <type_traits>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "util/logging.h"
#include "platform/stack_trace.h"
#include <vector>
#include <queue>
#include <sstream>

#if !defined(NDEBUG) && 0
#define LOG_ALLOC
#endif // NDEBUG

namespace programmerjake
{
namespace voxels
{
namespace memory
{
#ifdef LOG_ALLOC
using namespace std; // workaround for max_align_t not being in std namespace
struct MemoryHeader final
{
    std::size_t allocatedSize;
    char data[] alignas(max_align_t);
};
std::atomic_size_t allocatedMemory(false);
void log_alloc(std::size_t count)
{
    static thread_local bool isInCall = false;
    if(isInCall)
        return;
    isInCall = true;
    try
    {
        if(count >= (1 << 10)) // ignore small allocations
        {
            static std::mutex allocationsMapLock;
            typedef std::unordered_map<StackTrace, std::size_t> AllocationsMapType;
            static AllocationsMapType allocationsMap;
            static std::size_t byteCount = 0;
            const std::size_t dumpByteCount = 1 << 27; // 128 MiB
            StackTrace trace = StackTrace::make();
            std::unique_lock<std::mutex> lockIt(allocationsMapLock);
            allocationsMap[std::move(trace)] += count;
            byteCount += count;
            if(byteCount >= dumpByteCount && !getDebugLog().isInPostFunction())
            {
                byteCount %= dumpByteCount;
                auto compareFn = [](typename AllocationsMapType::iterator a,
                                    typename AllocationsMapType::iterator b)
                {
                    return std::get<1>(*a) > std::get<1>(*b);
                };
                std::priority_queue<typename AllocationsMapType::iterator,
                                    std::vector<typename AllocationsMapType::iterator>,
                                    decltype(compareFn)> pqueue(compareFn);
                for(auto i = allocationsMap.begin(); i != allocationsMap.end(); i++)
                {
                    pqueue.push(i);
                    if(pqueue.size() > 5)
                        pqueue.pop();
                }
                std::wostringstream os;
                while(!pqueue.empty())
                {
                    auto i = pqueue.top();
                    pqueue.pop();
                    os << std::get<1>(*i) << ": ";
                    StackTrace(std::get<0>(*i)).dump(os);
                    os << "\n";
                }
                getDebugLog().getPostFunction()(os.str());
            }
        }
    }
    catch(...)
    {
        isInCall = false;
        throw;
    }
    isInCall = false;
}
#endif // LOG_ALLOC
void *allocate(std::size_t count)
{
#ifndef LOG_ALLOC
    return std::malloc(count);
#else
    MemoryHeader *ptr = static_cast<MemoryHeader *>(std::malloc(count + sizeof(MemoryHeader)));
    if(ptr == nullptr)
        return nullptr;
    log_alloc(count);
    allocatedMemory.fetch_add(count, std::memory_order_relaxed);
    ptr->allocatedSize = count;
    return static_cast<void *>(&ptr->data[0]);
#endif // LOG_ALLOC
}
void release(void *v)
{
#ifndef LOG_ALLOC
    std::free(v);
#else
    if(v == nullptr)
        return;
    MemoryHeader *ptr =
        reinterpret_cast<MemoryHeader *>(static_cast<char *>(v) - offsetof(MemoryHeader, data));
    allocatedMemory.fetch_sub(ptr->allocatedSize, std::memory_order_relaxed);
    std::free(static_cast<void *>(ptr));
#endif // LOG_ALLOC
}
}
}
}

void *operator new(std::size_t count, const std::nothrow_t &)
{
    return programmerjake::voxels::memory::allocate(count);
}

void *operator new[](std::size_t count, const std::nothrow_t &)
{
    return operator new(count, std::nothrow);
}

void *operator new(std::size_t count)
{
    void *retval = operator new(count, std::nothrow);
    if(!retval)
    {
        throw std::bad_alloc();
    }
    return retval;
}

void *operator new[](std::size_t count)
{
    return operator new(count);
}

void operator delete(void *ptr, const std::nothrow_t &)
{
    programmerjake::voxels::memory::release(ptr);
}

void operator delete[](void *ptr, const std::nothrow_t &)
{
    operator delete(ptr, std::nothrow);
}

void operator delete(void *ptr)
{
    operator delete(ptr, std::nothrow);
}

void operator delete[](void *ptr)
{
    operator delete(ptr);
}
