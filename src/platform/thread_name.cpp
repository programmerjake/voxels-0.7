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
#include "platform/thread_name.h"
#include "util/string_cast.h"
#if _WIN64 || _WIN32
#ifdef _MSC_VER
#include <windows.h>
namespace programmerjake
{
namespace voxels
{
namespace
{
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void setThreadNameInternal(DWORD dwThreadID, const char *threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
    __try
    {
        RaiseException(MS_VC_EXCEPTION,
                       0,
                       sizeof(info) / sizeof(ULONG_PTR),
                       reinterpret_cast<ULONG_PTR *>(&info));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
}
void setThreadName(std::wstring name)
{
    setThreadNameInternal(GetCurrentThreadId(), string_cast<std::string>(name).c_str());
}
}
}
#else
namespace programmerjake
{
namespace voxels
{
void setThreadName(std::wstring name)
{
}
}
}
#endif // _MSC_VER
#elif __ANDROID__
#include <pthread.h>
namespace programmerjake
{
namespace voxels
{
void setThreadName(std::wstring name)
{
    std::string name2 = string_cast<std::string>(name);
    if(name2.size() > 15)
        name2.erase(15);
    pthread_setname_np(pthread_self(), name2.c_str());
}
}
}
#elif __APPLE__
#include <pthread.h>

namespace programmerjake
{
namespace voxels
{
void setThreadName(std::wstring name)
{
    pthread_setname_np(string_cast<std::string>(name).c_str());
}
}
}
#elif __linux
#include <pthread.h>
namespace programmerjake
{
namespace voxels
{
void setThreadName(std::wstring name)
{
    std::string name2 = string_cast<std::string>(name);
    if(name2.size() > 15)
        name2.erase(15);
    pthread_setname_np(pthread_self(), name2.c_str());
}
}
}
#elif __unix
#error implement setThreadName for other unix
#elif __posix
#error implement setThreadName for other posix
#else
#error unknown platform in setThreadName
#endif
