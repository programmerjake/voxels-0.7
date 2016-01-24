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
#include "platform/terminate_handler.h"
#include "util/util.h"
#include <cerrno>
#include <mutex>
#include <thread>

#if _WIN64 || _WIN32
#define TERMINATE_HANDLER_WINDOWS 1
#elif __ANDROID__
#define TERMINATE_HANDLER_ANDROID 1
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define TERMINATE_HANDLER_IPHONE 1
#else
#define TERMINATE_HANDLER_POSIX 1
#endif
#elif __linux || __unix || __posix
#define TERMINATE_HANDLER_POSIX 1
#else
#error unknown platform
#endif

#if TERMINATE_HANDLER_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif TERMINATE_HANDLER_POSIX
#include <semaphore.h>
#include <signal.h>
#elif defined(TERMINATE_HANDLER_ANDROID)
FIXME_MESSAGE(finish terminate handler code for android)
#else
#error terminate handler not implemented for platform
#endif

namespace programmerjake
{
namespace voxels
{
namespace
{
void setOrGetTerminationHandlerFn(std::function<void()> &handler, bool setHandler)
{
    // don't free because otherwise we or the handler function
    // might be called after our static variables are destructed
    static std::mutex *handlerLock = new std::mutex();
    std::unique_lock<std::mutex> lockIt(*handlerLock);
    static std::function<void()> *currentHandler = nullptr;
    if(setHandler)
    {
        if(handler == nullptr)
        {
            if(currentHandler)
                delete currentHandler;
            currentHandler = nullptr;
        }
        else if(currentHandler != nullptr)
        {
            *currentHandler = handler;
        }
        else
        {
            currentHandler = new std::function<void()>(handler);
        }
    }
    else
    {
        if(currentHandler != nullptr)
            handler = *currentHandler;
        else
            handler = std::function<void()>();
    }
}

#if TERMINATE_HANDLER_WINDOWS
BOOL WINAPI terminationHandlerRoutine(DWORD)
{
    std::function<void()> handler;
    setOrGetTerminationHandlerFn(handler, false);
    if(handler == nullptr)
        return FALSE;
    handler();
    return TRUE;
}

int initTerminateHandler()
{
    SetConsoleCtrlHandler(terminationHandlerRoutine, TRUE);
    return 0;
}
#elif TERMINATE_HANDLER_POSIX
int initTerminateHandler()
{
    static sem_t signalSemaphore;
    sem_init(&signalSemaphore, 0, 0);
    std::thread([]()
    {
        while(true)
        {
            while(sem_wait(&signalSemaphore) != 0)
            {
                if(errno == EINTR)
                    continue;
                abort();
            }
            std::function<void()> handler;
            setOrGetTerminationHandlerFn(handler, false);
            if(handler == nullptr)
            {
                std::_Exit(82);
            }
            handler();
        }
    }).detach();
    struct sigaction action = {};
    action.sa_handler = [](int)
    {
        sem_post(&signalSemaphore);
    };
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGHUP, &action, nullptr);
    return 0;
}
#elif defined(TERMINATE_HANDLER_ANDROID)
int initTerminateHandler()
{
    FIXME_MESSAGE(finish terminate handler code for android)
    return 0;
}
#else
#error terminate handler not implemented for platform
#endif
}

void setTerminationRequestHandler(std::function<void()> handler)
{
    if(handler != nullptr) // add exception handler
    {
        auto newHandler = [handler]() noexcept // noexcept handles calling std::terminate
        {
            handler();
        };
        handler = std::function<void()>(newHandler);
    }
    static int unused = initTerminateHandler(); // static so we initialize only once and in only one thread
    ignore_unused_variable_warning(unused);
    setOrGetTerminationHandlerFn(handler, true);
}

}
}
