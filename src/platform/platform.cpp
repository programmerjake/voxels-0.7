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
#define _FILE_OFFSET_BITS 64
#if _WIN64 || _WIN32
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max
#endif
#include "util/game_version.h"
#include "platform/platform.h"
#include "util/global_instance_maker.h"
#include "util/matrix.h"
#include "util/vector.h"
#include "util/string_cast.h"
#include <cwchar>
#include <string>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <deque>
#include <condition_variable>
#include <cctype>
#include "platform/audio.h"
#include "platform/thread_priority.h"
#include "util/logging.h"
#include "render/generate.h"
#include "util/tls.h"
#include <csignal>
#include <cstdio>
#include <ctime>
#ifdef _MSC_VER
#include <SDL.h>
#include <SDL_main.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#endif // _MSC_VER
#ifdef __IPHONEOS__
#define GRAPHICS_OPENGL_ES
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#elif defined(__ANDROID__)
#define GRAPHICS_OPENGL_ES
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#define GRAPHICS_OPENGL
#include <GL/gl.h>
#endif
#if(defined(_WIN64) || defined(_WIN32)) && !defined(_MSC_VER)
#include <GL/glext.h>
#else
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#endif

#ifndef SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK
#define SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK "SDL_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK"
#endif // SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK

#ifndef SDL_HINT_SIMULATE_INPUT_EVENTS
#define SDL_HINT_SIMULATE_INPUT_EVENTS "SDL_SIMULATE_INPUT_EVENTS"
#endif // SDL_HINT_SIMULATE_INPUT_EVENTS

#ifndef SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH
#define SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH "SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH"
#endif // SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH

using namespace std;

namespace programmerjake
{
namespace voxels
{
double Display::realtimeTimer()
{
    return static_cast<double>(chrono::duration_cast<chrono::nanoseconds>(
                                   chrono::system_clock::now().time_since_epoch()).count()) * 1e-9;
}

namespace
{
struct RWOpsException : public stream::IOException
{
    RWOpsException(string str) : IOException(str)
    {
    }
};

class RWOpsReader final : public stream::Reader
{
    RWOpsReader(const RWOpsReader &) = delete;
    RWOpsReader &operator=(const RWOpsReader &) = delete;

private:
    SDL_RWops *rw;

public:
    RWOpsReader(SDL_RWops *rw) : rw(rw)
    {
        if(rw == nullptr)
            throw RWOpsException("invalid RWOps");
    }
    virtual uint8_t readByte() override
    {
        SDL_ClearError(); // for error detection
        uint8_t retval;
        if(0 == SDL_RWread(rw, &retval, sizeof(retval), 1))
        {
            const char *str = SDL_GetError();
            if(str[0]) // non-empty string : error
                throw RWOpsException(str);
            throw stream::EOFException();
        }
        return retval;
    }
    virtual std::size_t readBytes(std::uint8_t *array, std::size_t maxCount) override
    {
        SDL_ClearError(); // for error detection
        std::size_t count =
            SDL_RWread(rw, static_cast<void *>(array), sizeof(std::uint8_t), maxCount);
        if(0 == count)
        {
            const char *str = SDL_GetError();
            if(str[0]) // non-empty string : error
                throw RWOpsException(str);
        }
        return count;
    }
    ~RWOpsReader()
    {
        SDL_RWclose(rw);
    }
    virtual std::int64_t tell() override
    {
        if(rw->seek == nullptr)
            throw stream::NonSeekableException();
        std::int64_t retval = SDL_RWtell(rw);
        if(retval < 0)
            throw stream::IOException(std::string("SDL_RWtell failed on seekable stream: ")
                                      + SDL_GetError());
        return retval;
    }
    virtual void seek(std::int64_t offset, stream::SeekPosition seekPosition) override
    {
        if(rw->seek == nullptr)
            throw stream::NonSeekableException();
        int whence;
        switch(seekPosition)
        {
        case stream::SeekPosition::Start:
            whence = RW_SEEK_SET;
            break;
        case stream::SeekPosition::Current:
            whence = RW_SEEK_CUR;
            break;
        case stream::SeekPosition::End:
            whence = RW_SEEK_END;
            break;
        default:
            UNREACHABLE();
        }
        if(SDL_RWseek(rw, offset, whence) < 0)
            throw stream::IOException(std::string("SDL_RWseek failed on seekable stream: ")
                                      + SDL_GetError());
    }
};
}

static void startSDL();
}
}

#if _WIN64 || _WIN32
#include <cstring>
#include <cwchar>
#include <windows.h>
namespace programmerjake
{
namespace voxels
{
static wchar_t *ResourcePrefix = nullptr;
static wstring getExecutablePath();

static void initfn();

static wstring getResourceFileName(wstring resource, bool useFallbackPath)
{
    if(useFallbackPath)
    {
        return wstring(L"res/" + resource);
    }
    initfn();
    return wstring(ResourcePrefix) + resource;
}

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

static wstring getExecutablePath()
{
    wchar_t buf[PATH_MAX + 1];
    DWORD rv = GetModuleFileNameW(NULL, &buf[0], PATH_MAX + 1);
    if(rv >= PATH_MAX + 1)
    {
        throw runtime_error(string("can't get executable path"));
    }
    buf[rv] = '\0';
    return wstring(&buf[0]);
}

static void initfn()
{
    static bool ran = false;
    if(ran)
        return;
    ran = true;
    wstring p = getExecutablePath();
    size_t pos = p.find_last_of(L"/\\");
    if(pos == wstring::npos)
        p = L"";
    else
        p = p.substr(0, pos + 1);
    p = p + wstring(L"res/");
    ResourcePrefix = new wchar_t[p.size() + 1];
    for(size_t i = 0; i < p.size(); i++)
        ResourcePrefix[i] = p[i];
    ResourcePrefix[p.size()] = L'\0';
}

static initializer initializer1([]()
                                {
                                    initfn();
                                });

shared_ptr<stream::Reader> getResourceReader(wstring resource)
{
    startSDL();
    string fname = string_cast<std::string>(getResourceFileName(resource, false));
    try
    {
        return make_shared<RWOpsReader>(SDL_RWFromFile(fname.c_str(), "rb"));
    }
    catch(RWOpsException &e)
    {
        fname = string_cast<std::string>(getResourceFileName(resource, true));
        return make_shared<RWOpsReader>(SDL_RWFromFile(fname.c_str(), "rb"));
    }
}
static int platformSetup()
{
    MSVC_PRAGMA(warning(suppress : 4996))
    if(getenv("SDL_AUDIODRIVER") == nullptr)
    {
        SetEnvironmentVariableA("SDL_AUDIODRIVER", "winmm");
    }
    return 0;
}
}
}
#elif __ANDROID__
#include <unistd.h>
#include <climits>
#include <cerrno>
#include <cstring>
#include <cwchar>
namespace programmerjake
{
namespace voxels
{
shared_ptr<stream::Reader> getResourceReader(wstring resource)
{
    startSDL();
    string fname = string_cast<string>(L"res/" + resource);
    try
    {
        return make_shared<RWOpsReader>(SDL_RWFromFile(fname.c_str(), "rb"));
    }
    catch(RWOpsException &e)
    {
        throw RWOpsException("can't open resource : " + string_cast<string>(resource));
    }
}
static int platformSetup()
{
    return 0;
}
}
}
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#error implement getResourceReader and platformSetup for iPhone
#else
#error implement getResourceReader and platformSetup for OS X
#endif
#elif __linux
#include <unistd.h>
#include <climits>
#include <cerrno>
#include <cstring>
#include <cwchar>
namespace programmerjake
{
namespace voxels
{
static atomic_bool setResourcePrefix(false);
static wstring *pResourcePrefix = nullptr;
static wstring getExecutablePath();
static void calcResourcePrefix();

static wstring getResourceFileName(wstring resource, bool useFallbackPath)
{
    if(useFallbackPath)
    {
        return wstring(L"res/") + resource;
    }
    calcResourcePrefix();
    return *pResourcePrefix + resource;
}

static wstring getExecutablePath()
{
    char buf[PATH_MAX + 1];
    ssize_t rv = readlink("/proc/self/exe", &buf[0], PATH_MAX);
    if(rv == -1)
    {
        throw runtime_error(string("can't get executable path : ") + strerror(errno));
    }
    buf[rv] = '\0';
    return string_cast<wstring>(&buf[0]);
}

static void calcResourcePrefix()
{
    if(setResourcePrefix.exchange(true))
        return;
    wstring p = getExecutablePath();
    size_t pos = p.find_last_of(L"/\\");
    if(pos == wstring::npos)
        p = L"";
    else
        p = p.substr(0, pos + 1);
    pResourcePrefix = new wstring(p + L"res/");
}

shared_ptr<stream::Reader> getResourceReader(wstring resource)
{
    startSDL();
    string fname = string_cast<string>(getResourceFileName(resource, false));
    try
    {
        return make_shared<RWOpsReader>(SDL_RWFromFile(fname.c_str(), "rb"));
    }
    catch(RWOpsException &e)
    {
        fname = string_cast<string>(getResourceFileName(resource, true));
        return make_shared<RWOpsReader>(SDL_RWFromFile(fname.c_str(), "rb"));
    }
}
static int platformSetup()
{
    return 0;
}
}
}
#elif __unix
#error implement getResourceReader and platformSetup for other unix
#elif __posix
#error implement getResourceReader and platformSetup for other posix
#else
#error unknown platform in getResourceReader
#endif

namespace programmerjake
{
namespace voxels
{
namespace
{
char *getPreferencesPath()
{
    startSDL();
    return SDL_GetPrefPath("programmerjake", "voxels");
}

std::wstring makeUserSpecificFilePath(std::wstring name)
{
    static char *preferencesPath = getPreferencesPath();
    if(!preferencesPath)
        return name;
    return string_cast<std::wstring>(preferencesPath) + name;
}
}

std::shared_ptr<stream::Reader> readUserSpecificFile(std::wstring name)
{
    return std::make_shared<stream::FileReader>(makeUserSpecificFilePath(name));
}

std::shared_ptr<stream::Writer> createOrWriteUserSpecificFile(std::wstring name)
{
    return std::make_shared<stream::FileWriter>(makeUserSpecificFilePath(name));
}

namespace
{
atomic_bool simulatingTouchInput(false);
struct TouchSimulationTouch final
{
    const int id;
    float x, y;
    TouchSimulationTouch(int id, float x, float y) : id(id), x(x), y(y)
    {
    }
};
struct TouchSimulationState final
{
    std::unordered_map<int, TouchSimulationTouch> touches;
    int draggingId = -1;
    TouchSimulationState() : touches()
    {
    }
};
std::shared_ptr<TouchSimulationState> touchSimulationState;
}

static std::size_t processorCount = 1;

static int xResInternal, yResInternal;

static SDL_Window *window = nullptr;
static SDL_GLContext glcontext = nullptr;
static atomic_bool runningGraphics(false), runningSDL(false), runningAudio(false);
static atomic_int SDLUseCount(0);
static atomic_bool addedAtExits(false);
#if 0
static std::mutex renderThreadLock;
static std::condition_variable renderThreadCond;
static std::thread renderThread;
static std::atomic_bool renderThreadRunning(false);
struct RenderThreadFunction final
{
    RenderThreadFunction(const RenderThreadFunction &) = default;
    RenderThreadFunction &operator =(const RenderThreadFunction &) = default;
    RenderThreadFunction(RenderThreadFunction &&) = default;
    RenderThreadFunction &operator =(RenderThreadFunction &&) = default;
    std::function<void()> fn;
    std::condition_variable *cond = nullptr;
    enum State
    {
        Waiting,
        Done,
        Aborted
    };
    State *state;
    RenderThreadFunction(std::function<void()> fn, std::condition_variable *cond, State *state)
        : fn(fn), cond(cond), state(state)
    {
    }
};
static std::deque<RenderThreadFunction> renderThreadFnQueue;

static void renderThreadRunFn()
{
    std::unique_lock<std::mutex> lockIt(renderThreadLock);
    for(;;)
    {
        while(renderThreadFnQueue.empty())
        {
            renderThreadCond.wait(lockIt);
        }
        RenderThreadFunction fn = renderThreadFnQueue.front();
        if(fn.fn == nullptr)
        {
            renderThreadFnQueue.pop_front();
            for(RenderThreadFunction fn : renderThreadFnQueue)
            {
                if(fn.state != nullptr)
                    *fn.state = RenderThreadFunction::Aborted;
                if(fn.cond != nullptr)
                    fn.cond->notify_all();
            }
            renderThreadFnQueue.clear();
            if(fn.state != nullptr)
                *fn.state = RenderThreadFunction::Done;
            if(fn.cond != nullptr)
                fn.cond->notify_all();
            break;
        }
        lockIt.unlock();
        fn.fn();
        lockIt.lock();
        if(fn.state != nullptr)
            *fn.state = RenderThreadFunction::Done;
        if(fn.cond != nullptr)
            fn.cond->notify_all();
        renderThreadFnQueue.pop_front();
        if(renderThreadFnQueue.empty())
            renderThreadCond.notify_all();
    }
    renderThreadRunning = false;
}

static void stopRenderThread()
{
    std::unique_lock<std::mutex> lockIt(renderThreadLock);
    if(!renderThreadRunning)
        return;
    renderThreadFnQueue.push_back(RenderThreadFunction(nullptr, nullptr, nullptr));
    lockIt.unlock();
    renderThread.join();
}

static bool runOnRenderThread(std::function<void()> fn)
{
    assert(fn != nullptr);
    std::unique_lock<std::mutex> lockIt(renderThreadLock);
    if(!renderThreadRunning)
    {
        if(!renderThreadRunning.exchange(true))
        {
            renderThread = thread(renderThreadRunFn);
        }
    }
    RenderThreadFunction::State state = RenderThreadFunction::Waiting;
    std::condition_variable cond;
    renderThreadFnQueue.push_back(RenderThreadFunction(fn, &cond, &state));
    while(state == RenderThreadFunction::Waiting)
        cond.wait(lockIt);
    if(state == RenderThreadFunction::Done)
        return true;
    return false;
}

class RunOnRenderThreadStatus final
{
private:
    RenderThreadFunction::State state = RenderThreadFunction::Waiting;
    std::condition_variable cond;
    bool did_wait = true;
public:
    RunOnRenderThreadStatus(const RunOnRenderThreadStatus &) = delete;
    RunOnRenderThreadStatus() = default;
    RunOnRenderThreadStatus &operator =(const RunOnRenderThreadStatus &) = delete;
    RenderThreadFunction makeRenderThreadFunction(std::function<void()> fn)
    {
        did_wait = false;
        state = RenderThreadFunction::Waiting;
        return RenderThreadFunction(fn, &cond, &state);
    }
    bool try_wait()
    {
        if(did_wait)
            return true;
        std::unique_lock<std::mutex> lockIt(renderThreadLock, std::try_to_lock);
        if(!lockIt.owns_lock())
            return false;
        if(state == RenderThreadFunction::Waiting)
            return false;
        did_wait = true;
        return true;
    }
    void wait()
    {
        if(did_wait)
            return;
        std::unique_lock<std::mutex> lockIt(renderThreadLock);
        while(state == RenderThreadFunction::Waiting)
            cond.wait(lockIt);
        did_wait = true;
    }
    bool status()
    {
        if(!did_wait)
            return false;
        return state == RenderThreadFunction::Done;
    }
};

static void runOnRenderThreadAsync(std::function<void()> fn)
{
    assert(fn != nullptr);
    std::unique_lock<std::mutex> lockIt(renderThreadLock);
    if(!renderThreadRunning)
    {
        if(!renderThreadRunning.exchange(true))
        {
            renderThread = thread(renderThreadRunFn);
        }
    }
    renderThreadFnQueue.push_back(RenderThreadFunction(fn, nullptr, nullptr));
}

static void runOnRenderThreadAsync(std::function<void()> fn, RunOnRenderThreadStatus &status)
{
    assert(fn != nullptr);
    std::unique_lock<std::mutex> lockIt(renderThreadLock);
    if(!renderThreadRunning)
    {
        if(!renderThreadRunning.exchange(true))
        {
            renderThread = thread(renderThreadRunFn);
        }
    }
    renderThreadFnQueue.push_back(status.makeRenderThreadFunction(fn));
}
#endif

static int globalSetup()
{
    MSVC_PRAGMA(warning(suppress : 4996))
    if(getenv("SIMULATE_TOUCH") != nullptr) // enable touch simulation
    {
        touchSimulationState = std::make_shared<TouchSimulationState>();
    }
    return platformSetup();
}

static void runPlatformSetup()
{
    static int v = globalSetup();
    ignore_unused_variable_warning(v);
}

static std::thread::id mainThreadId;
static int SDLCALL myEventFilterFunction(void *userData, SDL_Event *event);

static void startSDL()
{
    if(runningSDL.exchange(true))
        return;
    runPlatformSetup();
    if(0 != SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO))
    {
        cerr << "error : can't start SDL : " << SDL_GetError() << endl;
        exit(1);
    }
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    if(!addedAtExits.exchange(true))
    {
        atexit(endGraphics);
        atexit(endAudio);
    }
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
    SDL_SetHint(SDL_HINT_SIMULATE_INPUT_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
    processorCount = SDL_GetCPUCount();
    mainThreadId = std::this_thread::get_id();
    SDL_SetEventFilter(myEventFilterFunction, nullptr);
}

static SDL_AudioSpec globalAudioSpec;
static SDL_AudioDeviceID globalAudioDeviceID = 0;
static atomic_bool validAudio(false);

unsigned getGlobalAudioSampleRate()
{
    if(!validAudio)
        return 0;
    return globalAudioSpec.freq;
}

unsigned getGlobalAudioChannelCount()
{
    if(!validAudio)
        return 0;
    return globalAudioSpec.channels;
}

void startAudio()
{
    if(!runningAudio.exchange(true))
    {
        SDLUseCount++;
        startSDL();
        validAudio = true;
        SDL_AudioSpec desired;
        desired.callback = PlayingAudio::audioCallback;
        desired.channels = 6;
        desired.format = AUDIO_S16SYS;
        desired.freq = 96000;
        desired.samples = 4096;
        desired.userdata = nullptr;
        globalAudioDeviceID =
            SDL_OpenAudioDevice(nullptr,
                                0,
                                &desired,
                                &globalAudioSpec,
                                SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
        if(globalAudioDeviceID == 0)
        {
            cerr << "error : can't open audio device : " << SDL_GetError() << endl;
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            SDLUseCount--;
            runningAudio = false;
            exit(1);
        }
        SDL_PauseAudioDevice(globalAudioDeviceID, SDL_FALSE);
    }
}

void endAudio()
{
    if(runningAudio.exchange(false))
    {
        SDL_PauseAudioDevice(globalAudioDeviceID, SDL_TRUE);
        validAudio = false;
        SDL_CloseAudioDevice(globalAudioDeviceID);
        globalAudioDeviceID = 0;
        if(--SDLUseCount <= 0)
        {
            if(runningSDL.exchange(false))
                SDL_Quit();
        }
    }
}

bool audioRunning()
{
    return validAudio;
}

namespace
{
void setupRenderLayers();
}

static bool isFullScreen = false;

void endGraphics()
{
    if(runningGraphics.exchange(false))
    {
        pauseGraphics();
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if(--SDLUseCount <= 0)
    {
        if(runningSDL.exchange(false))
            SDL_Quit();
    }
}

static void getExtensions();

static std::atomic_uint_fast64_t currentGraphicsContextId(0);

std::uint64_t getGraphicsContextId()
{
    return currentGraphicsContextId;
}

void resumeGraphics()
{
    currentGraphicsContextId++;
    glcontext = SDL_GL_CreateContext(window);
    if(glcontext == nullptr)
    {
        cerr << "error : can't create OpenGL context : " << SDL_GetError();
        exit(1);
    }
    getExtensions();
    setupRenderLayers();
}

void pauseGraphics()
{
    if(glcontext)
        SDL_GL_DeleteContext(glcontext);
    glcontext = nullptr;
}

bool Display::paused()
{
    if(glcontext)
        return false;
    return true;
}

void startGraphics()
{
    if(runningGraphics.exchange(true))
        return;
    SDLUseCount++;
    startSDL();
    isFullScreen = false;
#if 0
    const SDL_VideoInfo * vidInfo = SDL_GetVideoInfo();
    if(vidInfo == nullptr)
    {
        xResInternal = 800;
        yResInternal = 600;
    }
    else
    {
        xResInternal = vidInfo->current_w;
        yResInternal = vidInfo->current_h;
        if(xResInternal == 32 * yResInternal / 9)
            xResInternal /= 2;
        else if(xResInternal == 32 * yResInternal / 10)
            xResInternal /= 2;
    }
#else
    if(GameVersion::DEBUG)
    {
        xResInternal = 800;
        yResInternal = 600;
    }
    else
    {
        xResInternal = 1024;
        yResInternal = 768;
    }
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    window = SDL_CreateWindow("Voxels",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              xResInternal,
                              yResInternal,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(window == nullptr)
    {
        cerr << "error : can't create window : " << SDL_GetError();
        exit(1);
    }
    resumeGraphics();
}

static volatile double lastFlipTime = 0;

static volatile double oldLastFlipTime = 0;

static recursive_mutex &flipTimeLock()
{
    static recursive_mutex retval;
    return retval;
}

static void lockFlipTime()
{
    flipTimeLock().lock();
}

static void unlockFlipTime()
{
    flipTimeLock().unlock();
}

struct FlipTimeLocker
{
    FlipTimeLocker()
    {
        lockFlipTime();
    }
    ~FlipTimeLocker()
    {
        unlockFlipTime();
    }
};

initializer initializer3([]()
                         {
                             lastFlipTime = Display::realtimeTimer();
                             const float fps = defaultFPS;
                             oldLastFlipTime = lastFlipTime - static_cast<double>(1) / fps;
                         });

static volatile float averageFPSInternal = defaultFPS;

static double instantaneousFPS()
{
    FlipTimeLocker lock;
    double delta = lastFlipTime - oldLastFlipTime;
    if(delta <= eps || delta > 0.25)
    {
        return averageFPSInternal;
    }
    return 1 / delta;
}

static double frameDeltaTime()
{
    FlipTimeLocker lock;
    double delta = lastFlipTime - oldLastFlipTime;
    if(delta <= eps || delta > 0.25)
    {
        return 1 / averageFPSInternal;
    }
    return delta;
}

const float FPSUpdateFactor = 0.1f;

static float averageFPS()
{
    FlipTimeLocker lock;
    return averageFPSInternal;
}

namespace
{
void finishDrawingRenderLayers();
}

static void flipDisplay(float fps)
{
    finishDrawingRenderLayers();
    double sleepTime = -1;
    FlipTimeLocker lock;
    double curTime = Display::realtimeTimer();
    if(fps > 0)
        sleepTime = 1 / fps - (curTime - lastFlipTime);
    if(sleepTime <= eps)
    {
        oldLastFlipTime = lastFlipTime;
        lastFlipTime = curTime;
        averageFPSInternal *= 1 - FPSUpdateFactor;
        averageFPSInternal += FPSUpdateFactor * static_cast<float>(instantaneousFPS());
    }
    if(sleepTime > eps)
    {
        this_thread::sleep_for(chrono::nanoseconds(static_cast<int64_t>(sleepTime * 1e9)));
        {
            FlipTimeLocker lock;
            oldLastFlipTime = lastFlipTime;
            lastFlipTime = Display::realtimeTimer();
            averageFPSInternal *= 1 - FPSUpdateFactor;
            averageFPSInternal += FPSUpdateFactor * static_cast<float>(instantaneousFPS());
        }
    }
    SDL_GL_SwapWindow(window);
}

static KeyboardKey translateKey(SDL_Scancode input)
{
    switch(input)
    {
    case SDL_SCANCODE_BACKSPACE:
        return KeyboardKey::Backspace;
    case SDL_SCANCODE_TAB:
        return KeyboardKey::Tab;
    case SDL_SCANCODE_CLEAR:
        return KeyboardKey::Clear;
    case SDL_SCANCODE_RETURN:
        return KeyboardKey::Return;
    case SDL_SCANCODE_PAUSE:
        return KeyboardKey::Pause;
    case SDL_SCANCODE_ESCAPE:
        return KeyboardKey::Escape;
    case SDL_SCANCODE_SPACE:
        return KeyboardKey::Space;
    case SDL_SCANCODE_APOSTROPHE:
        return KeyboardKey::SQuote;
    case SDL_SCANCODE_COMMA:
        return KeyboardKey::Comma;
    case SDL_SCANCODE_MINUS:
        return KeyboardKey::Dash;
    case SDL_SCANCODE_PERIOD:
        return KeyboardKey::Period;
    case SDL_SCANCODE_SLASH:
        return KeyboardKey::FSlash;
    case SDL_SCANCODE_0:
        return KeyboardKey::Num0;
    case SDL_SCANCODE_1:
        return KeyboardKey::Num1;
    case SDL_SCANCODE_2:
        return KeyboardKey::Num2;
    case SDL_SCANCODE_3:
        return KeyboardKey::Num3;
    case SDL_SCANCODE_4:
        return KeyboardKey::Num4;
    case SDL_SCANCODE_5:
        return KeyboardKey::Num5;
    case SDL_SCANCODE_6:
        return KeyboardKey::Num6;
    case SDL_SCANCODE_7:
        return KeyboardKey::Num7;
    case SDL_SCANCODE_8:
        return KeyboardKey::Num8;
    case SDL_SCANCODE_9:
        return KeyboardKey::Num9;
    case SDL_SCANCODE_SEMICOLON:
        return KeyboardKey::Semicolon;
    case SDL_SCANCODE_EQUALS:
        return KeyboardKey::Equals;
    case SDL_SCANCODE_LEFTBRACKET:
        return KeyboardKey::LBracket;
    case SDL_SCANCODE_BACKSLASH:
        return KeyboardKey::BSlash;
    case SDL_SCANCODE_RIGHTBRACKET:
        return KeyboardKey::RBracket;
    case SDL_SCANCODE_A:
        return KeyboardKey::A;
    case SDL_SCANCODE_B:
        return KeyboardKey::B;
    case SDL_SCANCODE_C:
        return KeyboardKey::C;
    case SDL_SCANCODE_D:
        return KeyboardKey::D;
    case SDL_SCANCODE_E:
        return KeyboardKey::E;
    case SDL_SCANCODE_F:
        return KeyboardKey::F;
    case SDL_SCANCODE_G:
        return KeyboardKey::G;
    case SDL_SCANCODE_H:
        return KeyboardKey::H;
    case SDL_SCANCODE_I:
        return KeyboardKey::I;
    case SDL_SCANCODE_J:
        return KeyboardKey::J;
    case SDL_SCANCODE_K:
        return KeyboardKey::K;
    case SDL_SCANCODE_L:
        return KeyboardKey::L;
    case SDL_SCANCODE_M:
        return KeyboardKey::M;
    case SDL_SCANCODE_N:
        return KeyboardKey::N;
    case SDL_SCANCODE_O:
        return KeyboardKey::O;
    case SDL_SCANCODE_P:
        return KeyboardKey::P;
    case SDL_SCANCODE_Q:
        return KeyboardKey::Q;
    case SDL_SCANCODE_R:
        return KeyboardKey::R;
    case SDL_SCANCODE_S:
        return KeyboardKey::S;
    case SDL_SCANCODE_T:
        return KeyboardKey::T;
    case SDL_SCANCODE_U:
        return KeyboardKey::U;
    case SDL_SCANCODE_V:
        return KeyboardKey::V;
    case SDL_SCANCODE_W:
        return KeyboardKey::W;
    case SDL_SCANCODE_X:
        return KeyboardKey::X;
    case SDL_SCANCODE_Y:
        return KeyboardKey::Y;
    case SDL_SCANCODE_Z:
        return KeyboardKey::Z;
    case SDL_SCANCODE_DELETE:
        return KeyboardKey::Delete;
    case SDL_SCANCODE_KP_0:
        return KeyboardKey::KPad0;
    case SDL_SCANCODE_KP_1:
        return KeyboardKey::KPad1;
    case SDL_SCANCODE_KP_2:
        return KeyboardKey::KPad2;
    case SDL_SCANCODE_KP_3:
        return KeyboardKey::KPad3;
    case SDL_SCANCODE_KP_4:
        return KeyboardKey::KPad4;
    case SDL_SCANCODE_KP_5:
        return KeyboardKey::KPad5;
    case SDL_SCANCODE_KP_6:
        return KeyboardKey::KPad6;
    case SDL_SCANCODE_KP_7:
        return KeyboardKey::KPad7;
    case SDL_SCANCODE_KP_8:
        return KeyboardKey::KPad8;
    case SDL_SCANCODE_KP_9:
        return KeyboardKey::KPad8;
    case SDL_SCANCODE_KP_PERIOD:
        return KeyboardKey::KPadPeriod;
    case SDL_SCANCODE_KP_DIVIDE:
        return KeyboardKey::KPadFSlash;
    case SDL_SCANCODE_KP_MULTIPLY:
        return KeyboardKey::KPadStar;
    case SDL_SCANCODE_KP_MINUS:
        return KeyboardKey::KPadDash;
    case SDL_SCANCODE_KP_PLUS:
        return KeyboardKey::KPadPlus;
    case SDL_SCANCODE_KP_ENTER:
        return KeyboardKey::KPadReturn;
    case SDL_SCANCODE_KP_EQUALS:
        return KeyboardKey::KPadEquals;
    case SDL_SCANCODE_UP:
        return KeyboardKey::Up;
    case SDL_SCANCODE_DOWN:
        return KeyboardKey::Down;
    case SDL_SCANCODE_RIGHT:
        return KeyboardKey::Right;
    case SDL_SCANCODE_LEFT:
        return KeyboardKey::Left;
    case SDL_SCANCODE_INSERT:
        return KeyboardKey::Insert;
    case SDL_SCANCODE_HOME:
        return KeyboardKey::Home;
    case SDL_SCANCODE_END:
        return KeyboardKey::End;
    case SDL_SCANCODE_PAGEUP:
        return KeyboardKey::PageUp;
    case SDL_SCANCODE_PAGEDOWN:
        return KeyboardKey::PageDown;
    case SDL_SCANCODE_F1:
        return KeyboardKey::F1;
    case SDL_SCANCODE_F2:
        return KeyboardKey::F2;
    case SDL_SCANCODE_F3:
        return KeyboardKey::F3;
    case SDL_SCANCODE_F4:
        return KeyboardKey::F4;
    case SDL_SCANCODE_F5:
        return KeyboardKey::F5;
    case SDL_SCANCODE_F6:
        return KeyboardKey::F6;
    case SDL_SCANCODE_F7:
        return KeyboardKey::F7;
    case SDL_SCANCODE_F8:
        return KeyboardKey::F8;
    case SDL_SCANCODE_F9:
        return KeyboardKey::F9;
    case SDL_SCANCODE_F10:
        return KeyboardKey::F10;
    case SDL_SCANCODE_F11:
        return KeyboardKey::F11;
    case SDL_SCANCODE_F12:
        return KeyboardKey::F12;
    case SDL_SCANCODE_F13:
    case SDL_SCANCODE_F14:
    case SDL_SCANCODE_F15:
        // TODO (jacob#): implement keys
        return KeyboardKey::Unknown;
    case SDL_SCANCODE_NUMLOCKCLEAR:
        return KeyboardKey::NumLock;
    case SDL_SCANCODE_CAPSLOCK:
        return KeyboardKey::CapsLock;
    case SDL_SCANCODE_SCROLLLOCK:
        return KeyboardKey::ScrollLock;
    case SDL_SCANCODE_RSHIFT:
        return KeyboardKey::RShift;
    case SDL_SCANCODE_LSHIFT:
        return KeyboardKey::LShift;
    case SDL_SCANCODE_RCTRL:
        return KeyboardKey::RCtrl;
    case SDL_SCANCODE_LCTRL:
        return KeyboardKey::LCtrl;
    case SDL_SCANCODE_RALT:
        return KeyboardKey::RAlt;
    case SDL_SCANCODE_LALT:
        return KeyboardKey::LAlt;
    case SDL_SCANCODE_RGUI:
        return KeyboardKey::RMeta;
    case SDL_SCANCODE_LGUI:
        return KeyboardKey::LMeta;
    case SDL_SCANCODE_MODE:
        return KeyboardKey::Mode;
    case SDL_SCANCODE_HELP:
        // TODO (jacob#): implement keys
        return KeyboardKey::Unknown;
    case SDL_SCANCODE_PRINTSCREEN:
        return KeyboardKey::PrintScreen;
    case SDL_SCANCODE_SYSREQ:
        return KeyboardKey::SysRequest;
    case SDL_SCANCODE_MENU:
        return KeyboardKey::Menu;
    case SDL_SCANCODE_POWER:
    case SDL_SCANCODE_UNDO:
        // TODO (jacob#): implement keys
        return KeyboardKey::Unknown;
    default:
        return KeyboardKey::Unknown;
    }
}

static KeyboardModifiers translateModifiers(SDL_Keymod input)
{
    int retval = KeyboardModifiers_None;
    if(input & KMOD_LSHIFT)
    {
        retval |= KeyboardModifiers_LShift;
    }
    if(input & KMOD_RSHIFT)
    {
        retval |= KeyboardModifiers_RShift;
    }
    if(input & KMOD_LALT)
    {
        retval |= KeyboardModifiers_LAlt;
    }
    if(input & KMOD_RALT)
    {
        retval |= KeyboardModifiers_RAlt;
    }
    if(input & KMOD_LCTRL)
    {
        retval |= KeyboardModifiers_LCtrl;
    }
    if(input & KMOD_RCTRL)
    {
        retval |= KeyboardModifiers_RCtrl;
    }
    if(input & KMOD_LGUI)
    {
        retval |= KeyboardModifiers_LMeta;
    }
    if(input & KMOD_RGUI)
    {
        retval |= KeyboardModifiers_RMeta;
    }
    if(input & KMOD_NUM)
    {
        retval |= KeyboardModifiers_NumLock;
    }
    if(input & KMOD_CAPS)
    {
        retval |= KeyboardModifiers_CapsLock;
    }
    if(input & KMOD_MODE)
    {
        retval |= KeyboardModifiers_Mode;
    }
    return static_cast<KeyboardModifiers>(retval);
}

static MouseButton translateButton(Uint8 button)
{
    switch(button)
    {
    case SDL_BUTTON_LEFT:
        return MouseButton_Left;
    case SDL_BUTTON_MIDDLE:
        return MouseButton_Middle;
    case SDL_BUTTON_RIGHT:
        return MouseButton_Right;
    case SDL_BUTTON_X1:
        return MouseButton_X1;
    case SDL_BUTTON_X2:
        return MouseButton_X2;
    default:
        return MouseButton_None;
    }
}

static enum_array<bool, KeyboardKey> keyState;

static MouseButton buttonState = MouseButton_None;

static std::atomic_bool needQuitEvent(false);

static int translateTouch(SDL_TouchID tid, SDL_FingerID fid, bool remove)
{
    typedef pair<SDL_TouchID, SDL_FingerID> Id;
    struct hashId
    {
        size_t operator()(Id id) const
        {
            return std::hash<SDL_TouchID>()(std::get<0>(id))
                   + std::hash<SDL_FingerID>()(std::get<1>(id));
        }
    };
    static shared_ptr<unordered_map<Id, int, hashId>> touchMap;
    static shared_ptr<unordered_set<int>> allocatedIds;
    if(touchMap == nullptr)
        touchMap = make_shared<unordered_map<Id, int, hashId>>();
    if(allocatedIds == nullptr)
        allocatedIds = make_shared<unordered_set<int>>();
    Id id(tid, fid);
    auto iter = touchMap->find(id);
    if(iter == touchMap->end())
    {
        int newId = allocatedIds->size();
        for(size_t i = 0; i < allocatedIds->size(); i++)
        {
            if(0 == allocatedIds->count(i))
            {
                newId = i;
                break;
            }
        }
        allocatedIds->insert(newId);
        iter = std::get<0>(touchMap->insert(make_pair(id, newId)));
    }
    int retval = std::get<1>(*iter);
    if(remove)
    {
        touchMap->erase(iter);
        allocatedIds->erase(retval);
    }
    return retval;
}

static std::shared_ptr<PlatformEvent> makeEvent(SDL_Event &SDLEvent)
{
    switch(SDLEvent.type)
    {
    case SDL_KEYDOWN:
    {
        KeyboardKey key = translateKey(SDLEvent.key.keysym.scancode);
        auto retval = std::make_shared<KeyDownEvent>(
            key,
            translateModifiers(static_cast<SDL_Keymod>(SDLEvent.key.keysym.mod)),
            keyState[key]);
        keyState[key] = true;
        return retval;
    }
    case SDL_KEYUP:
    {
        KeyboardKey key = translateKey(SDLEvent.key.keysym.scancode);
        auto retval = std::make_shared<KeyUpEvent>(
            key, translateModifiers(static_cast<SDL_Keymod>(SDLEvent.key.keysym.mod)));
        keyState[key] = false;
        return retval;
    }
    case SDL_MOUSEMOTION:
    {
#ifdef __ANDROID__
        break;
#else
        if(SDLEvent.motion.which == SDL_TOUCH_MOUSEID)
            break;
        if(touchSimulationState)
        {
            if(touchSimulationState->draggingId >= 0)
            {
                float newX = (float)SDLEvent.motion.x / (float)xResInternal * 2 - 1;
                float newY = (float)SDLEvent.motion.y / (float)yResInternal * 2 - 1;
                TouchSimulationTouch &touch =
                    touchSimulationState->touches.at(touchSimulationState->draggingId);
                float oldX = touch.x;
                float oldY = touch.y;
                touch.x = newX;
                touch.y = newY;
                return std::make_shared<TouchMoveEvent>(
                    newX, newY, newX - oldX, newY - oldY, touchSimulationState->draggingId, 0.5f);
            }
            break;
        }
        return std::make_shared<MouseMoveEvent>((float)SDLEvent.motion.x,
                                                (float)SDLEvent.motion.y,
                                                (float)SDLEvent.motion.xrel,
                                                (float)SDLEvent.motion.yrel);
#endif
    }
    case SDL_MOUSEWHEEL:
    {
#ifdef __ANDROID__
        break;
#else
        if(SDLEvent.wheel.which == SDL_TOUCH_MOUSEID)
            break;
        if(touchSimulationState)
            break;
#ifdef SDL_MOUSEWHEEL_FLIPPED
        if(SDLEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
        {
            return std::make_shared<MouseScrollEvent>(-SDLEvent.wheel.x, -SDLEvent.wheel.y);
        }
#endif
        return std::make_shared<MouseScrollEvent>(SDLEvent.wheel.x, SDLEvent.wheel.y);
#endif
    }
    case SDL_MOUSEBUTTONDOWN:
    {
#ifdef __ANDROID__
        break;
#else
        if(SDLEvent.button.which == SDL_TOUCH_MOUSEID)
            break;
        MouseButton button = translateButton(SDLEvent.button.button);
        buttonState = static_cast<MouseButton>(buttonState | button); // set bit
        if(touchSimulationState)
        {
            float newX = (float)SDLEvent.button.x / (float)xResInternal * 2 - 1;
            float newY = (float)SDLEvent.button.y / (float)yResInternal * 2 - 1;
            if(touchSimulationState->draggingId >= 0)
            {
                TouchSimulationTouch &touch =
                    touchSimulationState->touches.at(touchSimulationState->draggingId);
                float oldX = touch.x;
                float oldY = touch.y;
                touch.x = newX;
                touch.y = newY;
                return std::make_shared<TouchMoveEvent>(
                    newX, newY, newX - oldX, newY - oldY, touchSimulationState->draggingId, 0.5f);
            }
            else
            {
                for(auto i = touchSimulationState->touches.begin();
                    i != touchSimulationState->touches.end();
                    ++i)
                {
                    TouchSimulationTouch &touch = std::get<1>(*i);
                    float oldX = touch.x;
                    float oldY = touch.y;
                    float touchSizeX = 32.0f / (float)xResInternal;
                    float touchSizeY = 32.0f / (float)yResInternal;
                    if(std::fabs(oldX - newX) > touchSizeX || std::fabs(oldY - newY) > touchSizeY)
                        continue;
                    touchSimulationState->draggingId = touch.id;
                    touch.x = newX;
                    touch.y = newY;
                    return std::make_shared<TouchMoveEvent>(
                        newX, newY, newX - oldX, newY - oldY, touch.id, 0.5f);
                }
                int newId = touchSimulationState->touches.size();
                for(int i = 0; i < (int)touchSimulationState->touches.size(); i++)
                {
                    if(touchSimulationState->touches.count(i) == 0)
                    {
                        newId = i;
                        break;
                    }
                }
                touchSimulationState->draggingId = newId;
                touchSimulationState->touches.emplace(newId,
                                                      TouchSimulationTouch(newId, newX, newY));
                return std::make_shared<TouchDownEvent>(newX, newY, 0.0f, 0.0f, newId, 0.5f);
            }
            break;
        }
        return std::make_shared<MouseDownEvent>(
            (float)SDLEvent.button.x, (float)SDLEvent.button.y, 0.0f, 0.0f, button);
#endif
    }
    case SDL_MOUSEBUTTONUP:
    {
#ifdef __ANDROID__
        break;
#else
        if(SDLEvent.button.which == SDL_TOUCH_MOUSEID)
            break;
        MouseButton button = translateButton(SDLEvent.button.button);
        buttonState = static_cast<MouseButton>(buttonState & ~button); // clear bit
        if(touchSimulationState)
        {
            float newX = (float)SDLEvent.button.x / (float)xResInternal * 2 - 1;
            float newY = (float)SDLEvent.button.y / (float)yResInternal * 2 - 1;
            if(touchSimulationState->draggingId >= 0)
            {
                TouchSimulationTouch &touch =
                    touchSimulationState->touches.at(touchSimulationState->draggingId);
                float oldX = touch.x;
                float oldY = touch.y;
                touch.x = newX;
                touch.y = newY;
                int touchId = touchSimulationState->draggingId;
                if(button == MouseButton_Left && buttonState == MouseButton_None)
                {
                    touchSimulationState->touches.erase(touchId);
                    touchSimulationState->draggingId = -1;
                    return std::make_shared<TouchUpEvent>(
                        newX, newY, newX - oldX, newY - oldY, touchId, 0.5f);
                }
                else if(buttonState == MouseButton_None)
                {
                    touchSimulationState->draggingId = -1;
                }
                return std::make_shared<TouchMoveEvent>(
                    newX, newY, newX - oldX, newY - oldY, touchId, 0.5f);
            }
            break;
        }
        return std::make_shared<MouseUpEvent>(
            (float)SDLEvent.button.x, (float)SDLEvent.button.y, 0.0f, 0.0f, button);
#endif
    }
    case SDL_FINGERMOTION:
        if(touchSimulationState)
            break;
        return std::make_shared<TouchMoveEvent>(
            SDLEvent.tfinger.x * 2 - 1,
            SDLEvent.tfinger.y * 2 - 1,
            SDLEvent.tfinger.dx * 2,
            SDLEvent.tfinger.dy * 2,
            translateTouch(SDLEvent.tfinger.touchId, SDLEvent.tfinger.fingerId, false),
            SDLEvent.tfinger.pressure);
    case SDL_FINGERDOWN:
        if(touchSimulationState)
            break;
        return std::make_shared<TouchDownEvent>(
            SDLEvent.tfinger.x * 2 - 1,
            SDLEvent.tfinger.y * 2 - 1,
            SDLEvent.tfinger.dx * 2,
            SDLEvent.tfinger.dy * 2,
            translateTouch(SDLEvent.tfinger.touchId, SDLEvent.tfinger.fingerId, false),
            SDLEvent.tfinger.pressure);
    case SDL_FINGERUP:
        if(touchSimulationState)
            break;
        return std::make_shared<TouchUpEvent>(
            SDLEvent.tfinger.x * 2 - 1,
            SDLEvent.tfinger.y * 2 - 1,
            SDLEvent.tfinger.dx * 2,
            SDLEvent.tfinger.dy * 2,
            translateTouch(SDLEvent.tfinger.touchId, SDLEvent.tfinger.fingerId, true),
            SDLEvent.tfinger.pressure);
    case SDL_JOYAXISMOTION:
    case SDL_JOYBALLMOTION:
    case SDL_JOYHATMOTION:
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
        // TODO (jacob#): handle joysticks
        break;
    case SDL_WINDOWEVENT:
    {
        switch(SDLEvent.window.event)
        {
        case SDL_WINDOWEVENT_MINIMIZED:
        case SDL_WINDOWEVENT_HIDDEN:
            if(!Display::paused())
            {
                return std::make_shared<PauseEvent>();
            }
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
        case SDL_WINDOWEVENT_SHOWN:
            if(Display::paused())
            {
                return std::make_shared<ResumeEvent>();
            }
            break;
        }
        break;
    }
    case SDL_QUIT:
        return std::make_shared<QuitEvent>();
    case SDL_SYSWMEVENT:
        // TODO (jacob#): handle SDL_SYSWMEVENT
        break;
    case SDL_TEXTEDITING:
    {
        std::string text = SDLEvent.edit.text;
        return std::make_shared<TextEditEvent>(
            string_cast<std::wstring>(text), SDLEvent.edit.start, SDLEvent.edit.length);
    }
    case SDL_TEXTINPUT:
    {
        std::string text = SDLEvent.text.text;
        return std::make_shared<TextInputEvent>(string_cast<std::wstring>(text));
    }
    }
    return nullptr;
}

static std::mutex synchronousEventLock;
static std::condition_variable synchronousEventCond;
static SDL_Event *synchronousEvent = nullptr;

static int SDLCALL myEventFilterFunction(void *userData, SDL_Event *event)
{
    switch(event->type)
    {
    case SDL_QUIT: // needs special processing
        needQuitEvent = true;
        return 0;
    case SDL_WINDOWEVENT:
    {
        if(std::this_thread::get_id() == mainThreadId)
            return 1;
        std::unique_lock<std::mutex> lockIt(synchronousEventLock);
        assert(synchronousEvent == nullptr);
        synchronousEvent = event;
        while(synchronousEvent)
        {
            synchronousEventCond.wait(lockIt);
        }
        return 0;
    }
    default:
        return 1;
    }
}

static std::shared_ptr<PlatformEvent> getSynchronousEvent()
{
    std::unique_lock<std::mutex> lockIt(synchronousEventLock);
    if(synchronousEvent)
    {
        auto retval = makeEvent(*synchronousEvent);
        synchronousEvent = nullptr;
        return retval;
    }
    else
    {
        synchronousEventCond.notify_all();
        return nullptr;
    }
}

static std::shared_ptr<PlatformEvent> makeEvent()
{
    auto retval = getSynchronousEvent();
    if(retval)
        return retval;
    if(needQuitEvent.exchange(false))
    {
        return std::make_shared<QuitEvent>();
    }
    while(true)
    {
        SDL_Event SDLEvent;
        if(SDL_PollEvent(&SDLEvent) == 0)
        {
            if(needQuitEvent.exchange(false))
            {
                return std::make_shared<QuitEvent>();
            }
            break;
        }
        retval = makeEvent(SDLEvent);
        if(retval)
            return retval;
    }
    return nullptr;
}

namespace
{
struct DefaultEventHandler : public EventHandler
{
    virtual bool handleTouchUp(TouchUpEvent &) override
    {
        return true;
    }
    virtual bool handleTouchDown(TouchDownEvent &) override
    {
        return true;
    }
    virtual bool handleTouchMove(TouchMoveEvent &) override
    {
        return true;
    }
    virtual bool handleMouseUp(MouseUpEvent &) override
    {
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &) override
    {
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &) override
    {
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &) override
    {
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent &) override
    {
        return true;
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        if(event.key == KeyboardKey::F4 && (event.mods & KeyboardModifiers_Alt) != 0)
        {
            needQuitEvent = true;
            return true;
        }
        return true;
    }
    virtual bool handleTextInput(TextInputEvent &) override
    {
        return true;
    }
    virtual bool handleTextEdit(TextEditEvent &) override
    {
        return true;
    }
    virtual bool handlePause(PauseEvent &) override
    {
        return true;
    }
    virtual bool handleResume(ResumeEvent &) override
    {
        return true;
    }
    virtual bool handleQuit(QuitEvent &) override
    {
        exit(0);
        return true;
    }
};
shared_ptr<EventHandler> DefaultEventHandler_handler(new DefaultEventHandler);
}

static void handleEvents(shared_ptr<EventHandler> eventHandler)
{
    for(std::shared_ptr<PlatformEvent> e = makeEvent(); e != nullptr; e = makeEvent())
    {
        if(eventHandler == nullptr || !e->dispatch(eventHandler))
        {
            e->dispatch(DefaultEventHandler_handler);
        }
    }
}

void glLoadMatrix(Matrix mat)
{
    float matArray[16] = {mat.x00,
                          mat.x01,
                          mat.x02,
                          mat.x03,

                          mat.x10,
                          mat.x11,
                          mat.x12,
                          mat.x13,

                          mat.x20,
                          mat.x21,
                          mat.x22,
                          mat.x23,

                          mat.x30,
                          mat.x31,
                          mat.x32,
                          mat.x33};
    glLoadMatrixf(static_cast<const float *>(matArray));
}

wstring Display::title()
{
    return string_cast<wstring>(SDL_GetWindowTitle(window));
}

void Display::title(wstring newTitle)
{
    string s = string_cast<string>(newTitle);
    SDL_SetWindowTitle(window, s.c_str());
}

void Display::handleEvents(shared_ptr<EventHandler> eventHandler)
{
    programmerjake::voxels::handleEvents(eventHandler);
}

static double timer_;

static void updateTimer()
{
    timer_ = Display::realtimeTimer();
}

initializer initializer4([]()
                         {
                             updateTimer();
                         });

void Display::flip(float fps)
{
    flipDisplay(fps);
    updateTimer();
}

void Display::flip()
{
    flipDisplay(screenRefreshRate());
    updateTimer();
}

double Display::instantaneousFPS()
{
    return programmerjake::voxels::instantaneousFPS();
}

double Display::frameDeltaTime()
{
    return programmerjake::voxels::frameDeltaTime();
}

float Display::averageFPS()
{
    return programmerjake::voxels::averageFPS();
}

double Display::timer()
{
    return timer_;
}

static float scaleXInternal = 1.0, scaleYInternal = 1.0;

int Display::width()
{
    return xResInternal;
}

int Display::height()
{
    return yResInternal;
}

float Display::scaleX()
{
    return scaleXInternal;
}

float Display::scaleY()
{
    return scaleYInternal;
}

static bool grabMouse_ = false;

bool Display::grabMouse()
{
    return grabMouse_;
}

void Display::grabMouse(bool g)
{
    grabMouse_ = g;
    if(touchSimulationState)
        return;
    SDL_SetRelativeMouseMode(g ? SDL_TRUE : SDL_FALSE);
    SDL_SetWindowGrab(window, g ? SDL_TRUE : SDL_FALSE);
}

VectorF Display::transformMouseTo3D(float x, float y, float depth)
{
    return VectorF(depth * scaleX() * (2 * x / width() - 1),
                   depth * scaleY() * (1 - 2 * y / height()),
                   -depth);
}

VectorF Display::transformTouchTo3D(float x, float y, float depth)
{
    return VectorF(depth * scaleX() * x, depth * scaleY() * -y, -depth);
}

VectorF Display::transform3DToMouse(VectorF pos)
{
    float depth = -pos.z;
    pos.x = pos.x / depth / scaleX();
    pos.y = -pos.y / depth / scaleY();
    pos.x = (pos.x * 0.5f + 0.5f) * width();
    pos.y = (pos.y * 0.5f + 0.5f) * height();
    return pos;
}

VectorF Display::transform3DToTouch(VectorF pos)
{
    float depth = -pos.z;
    pos.x = pos.x / depth / scaleX();
    pos.y = -pos.y / depth / scaleY();
    return pos;
}

float Display::getTouchControlSize()
{
    // don't have code to implement getting display size in inches
    return 0.3f;
}

namespace
{
void setArrayBufferBinding(GLuint buffer);

void renderInternal(const Triangle *triangles, std::size_t triangleCount)
{
    if(triangleCount == 0)
        return;
    const char *array_start = reinterpret_cast<const char *>(triangles);
    glVertexPointer(3, GL_FLOAT, Triangle_vertex_stride, array_start + Triangle_position_start);
    glTexCoordPointer(
        2, GL_FLOAT, Triangle_vertex_stride, array_start + Triangle_texture_coord_start);
    glColorPointer(4, GL_FLOAT, Triangle_vertex_stride, array_start + Triangle_color_start);
    glDrawArrays(GL_TRIANGLES, 0, (GLint)triangleCount * 3);
}

#if defined(_MSC_VER) || defined(__ANDROID__)
typedef std::ptrdiff_t GLsizeiptr;
typedef std::ptrdiff_t GLintptr;
typedef void(APIENTRYP PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void(APIENTRYP PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void(APIENTRYP PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void(APIENTRYP PFNGLBUFFERDATAPROC)(GLenum target,
                                            GLsizeiptr size,
                                            const void *data,
                                            GLenum usage);
typedef void(APIENTRYP PFNGLBUFFERSUBDATAPROC)(GLenum target,
                                               GLintptr offset,
                                               GLsizeiptr size,
                                               const void *data);
typedef void *(APIENTRYP PFNGLMAPBUFFERPROC)(GLenum target, GLenum access);
typedef GLboolean(APIENTRYP PFNGLUNMAPBUFFERPROC)(GLenum target);
typedef void(APIENTRYP PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint *framebuffers);
typedef void(APIENTRYP PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint *framebuffers);
typedef void(APIENTRYP PFNGLGENRENDERBUFFERSEXTPROC)(GLsizei n, GLuint *renderbuffers);
typedef void(APIENTRYP PFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei n, const GLuint *renderbuffers);
typedef void(APIENTRYP PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
typedef void(APIENTRYP PFNGLBINDRENDERBUFFEREXTPROC)(GLenum target, GLuint renderbuffer);
typedef void(APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(
    GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void(APIENTRYP PFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum target,
                                                        GLenum internalformat,
                                                        GLsizei width,
                                                        GLsizei height);
typedef void(APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum target,
                                                            GLenum attachment,
                                                            GLenum renderbuffertarget,
                                                            GLuint renderbuffer);
typedef GLenum(APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32 0x81A7
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_WRITE_ONLY 0x88B9
#endif // _MSC_VER

PFNGLBINDBUFFERPROC fnGLBindBuffer = nullptr;
PFNGLDELETEBUFFERSPROC fnGLDeleteBuffers = nullptr;
PFNGLGENBUFFERSPROC fnGLGenBuffers = nullptr;
PFNGLBUFFERDATAPROC fnGLBufferData = nullptr;
PFNGLBUFFERSUBDATAPROC fnGLBufferSubData = nullptr;
PFNGLMAPBUFFERPROC fnGLMapBuffer = nullptr;
PFNGLUNMAPBUFFERPROC fnGLUnmapBuffer = nullptr;
bool haveOpenGLBuffersWithoutMap = false;
bool haveOpenGLBuffersWithMap = false;
PFNGLGENFRAMEBUFFERSEXTPROC fnGlGenFramebuffersEXT = nullptr;
PFNGLDELETEFRAMEBUFFERSEXTPROC fnGlDeleteFramebuffersEXT = nullptr;
PFNGLGENRENDERBUFFERSEXTPROC fnGlGenRenderbuffersEXT = nullptr;
PFNGLDELETERENDERBUFFERSEXTPROC fnGlDeleteRenderbuffersEXT = nullptr;
PFNGLBINDFRAMEBUFFEREXTPROC fnGlBindFramebufferEXT = nullptr;
PFNGLBINDRENDERBUFFEREXTPROC fnGlBindRenderbufferEXT = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC fnGlFramebufferTexture2DEXT = nullptr;
PFNGLRENDERBUFFERSTORAGEEXTPROC fnGlRenderbufferStorageEXT = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC fnGlFramebufferRenderbufferEXT = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC fnGlCheckFramebufferStatusEXT = nullptr;
bool haveOpenGLFramebuffers = false;
enum_array<GLuint, RenderLayer> renderLayerFramebuffer = {};
enum_array<GLuint, RenderLayer> renderLayerRenderbuffer = {};
enum_array<GLuint, RenderLayer> renderLayerTexture = {};
int renderLayerTextureW = 1, renderLayerTextureH = 1;
bool haveOpenGLArbitraryTextureSize = false;

bool renderLayerNeedsSeperateRenderTarget(RenderLayer rl)
{
    switch(rl)
    {
    case RenderLayer::Opaque:
        return false;
    case RenderLayer::Translucent:
        return true;
    }
    UNREACHABLE();
    return false;
}

bool usingOpenGLFramebuffers = false;

void setupRenderLayers()
{
    renderLayerTextureW = xResInternal;
    renderLayerTextureH = yResInternal;
    if(!haveOpenGLArbitraryTextureSize)
    {
        renderLayerTextureW = 1;
        while(renderLayerTextureW < xResInternal && renderLayerTextureW > 0)
            renderLayerTextureW <<= 1;
        assert(renderLayerTextureW > 0);
        renderLayerTextureH = 1;
        while(renderLayerTextureH < yResInternal && renderLayerTextureH > 0)
            renderLayerTextureH <<= 1;
        assert(renderLayerTextureH > 0);
    }
    if(renderLayerTextureW <= 0)
        renderLayerTextureW = 1;
    if(renderLayerTextureH <= 0)
        renderLayerTextureH = 1;
    usingOpenGLFramebuffers = haveOpenGLFramebuffers;
#if 1
    usingOpenGLFramebuffers = false;
#endif
    if(usingOpenGLFramebuffers)
    {
        for(auto i = enum_traits<RenderLayer>::begin(); i != enum_traits<RenderLayer>::end(); ++i)
        {
            RenderLayer rl = *i;
            renderLayerFramebuffer[rl] = 0;
            renderLayerRenderbuffer[rl] = 0;
            renderLayerTexture[rl] = 0;
            if(!renderLayerNeedsSeperateRenderTarget(rl))
                continue;
            glGenTextures(1, &renderLayerTexture[rl]);
            glBindTexture(GL_TEXTURE_2D, renderLayerTexture[rl]);
#ifdef GRAPHICS_OPENGL_ES
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#elif defined(GRAPHICS_OPENGL)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#else
#error invalid graphics
#endif
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         renderLayerTextureW,
                         renderLayerTextureH,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         nullptr);
            fnGlGenFramebuffersEXT(1, &renderLayerFramebuffer[rl]);
            fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, renderLayerFramebuffer[rl]);
            fnGlFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                        GL_COLOR_ATTACHMENT0_EXT,
                                        GL_TEXTURE_2D,
                                        renderLayerTexture[rl],
                                        0);
            fnGlGenRenderbuffersEXT(1, &renderLayerRenderbuffer[rl]);
            bool good = false;
            for(GLenum depthComponent : {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT16})
            {
                fnGlBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderLayerRenderbuffer[rl]);
                fnGlRenderbufferStorageEXT(
                    GL_RENDERBUFFER_EXT, depthComponent, renderLayerTextureW, renderLayerTextureH);
                fnGlFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                               GL_DEPTH_ATTACHMENT_EXT,
                                               GL_RENDERBUFFER_EXT,
                                               renderLayerRenderbuffer[rl]);
                if(fnGlCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
                {
                    continue;
                }
                good = true;
                break;
            }
            if(!good)
            {
                fnGlBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
                fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                fnGlDeleteFramebuffersEXT(1, &renderLayerFramebuffer[rl]);
                fnGlDeleteRenderbuffersEXT(1, &renderLayerRenderbuffer[rl]);
                glDeleteTextures(1, &renderLayerTexture[rl]);
                while(i != enum_traits<RenderLayer>::begin())
                {
                    --i;
                    rl = *i;
                    if(!renderLayerNeedsSeperateRenderTarget(rl))
                        continue;
                    fnGlDeleteFramebuffersEXT(1, &renderLayerFramebuffer[rl]);
                    fnGlDeleteRenderbuffersEXT(1, &renderLayerRenderbuffer[rl]);
                    glDeleteTextures(1, &renderLayerTexture[rl]);
                }
                usingOpenGLFramebuffers = false;
                break;
            }
        }
        if(usingOpenGLFramebuffers)
        {
            glDisable(GL_BLEND);
        }
        else
        {
            glEnable(GL_BLEND);
        }
    }
}

void teardownRenderLayers()
{
    if(usingOpenGLFramebuffers)
    {
        fnGlBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            if(!renderLayerNeedsSeperateRenderTarget(rl))
                continue;
            fnGlDeleteFramebuffersEXT(1, &renderLayerFramebuffer[rl]);
            fnGlDeleteRenderbuffersEXT(1, &renderLayerRenderbuffer[rl]);
            glDeleteTextures(1, &renderLayerTexture[rl]);
        }
        usingOpenGLFramebuffers = false;
        glEnable(GL_BLEND);
    }
}

void updateRenderLayersSize()
{
    if(usingOpenGLFramebuffers)
    {
        if(xResInternal > renderLayerTextureW || yResInternal > renderLayerTextureH)
        {
            teardownRenderLayers();
            setupRenderLayers();
        }
    }
}

void clearRenderLayers(bool depthOnly)
{
    if(usingOpenGLFramebuffers)
    {
        fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        if(depthOnly)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
        }
        else
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            if(!renderLayerNeedsSeperateRenderTarget(rl))
                continue;
            fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, renderLayerFramebuffer[rl]);
            if(depthOnly)
            {
                glClear(GL_DEPTH_BUFFER_BIT);
            }
            else
            {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
        }
    }
    else
    {
        if(depthOnly)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
        }
        else
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }
}

template <typename F>
void renderToRenderLayer(RenderLayer rl, F drawFn)
{
    switch(rl)
    {
    case RenderLayer::Opaque:
    {
        if(usingOpenGLFramebuffers)
        {
            glEnable(GL_BLEND);
            fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            drawFn();
            for(RenderLayer i : enum_traits<RenderLayer>())
            {
                if(!renderLayerNeedsSeperateRenderTarget(i))
                    continue;
                fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, renderLayerFramebuffer[i]);
                drawFn();
            }
            glDisable(GL_BLEND);
        }
        else
        {
            drawFn();
        }
        return;
    }
    case RenderLayer::Translucent:
    {
        if(usingOpenGLFramebuffers)
        {
            fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, renderLayerFramebuffer[rl]);
            drawFn();
        }
        else
        {
            glDepthMask(GL_FALSE);
            drawFn();
            glDepthMask(GL_TRUE);
        }
        return;
    }
    }
    UNREACHABLE();
}

Image makeTouchTexture()
{
    const int XSize = 32, YSize = 32;
    const ColorI pixels[XSize][YSize] = {{RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 89),
                                          RGBAI(255, 255, 255, 156),
                                          RGBAI(255, 255, 255, 209),
                                          RGBAI(255, 255, 255, 235),
                                          RGBAI(253, 253, 253, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 235),
                                          RGBAI(255, 255, 255, 209),
                                          RGBAI(255, 255, 255, 156),
                                          RGBAI(255, 255, 255, 89),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(255, 255, 255, 254),
                                          RGBAI(0, 0, 0, 254),
                                          RGBAI(6, 6, 6, 255),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(255, 255, 255, 37),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(255, 255, 255, 37),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(72, 72, 72, 255),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(72, 72, 72, 255),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(255, 255, 255, 0),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(255, 255, 255, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(255, 255, 255, 89),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(255, 255, 255, 89)},
                                         {RGBAI(255, 255, 255, 156),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(255, 255, 255, 156)},
                                         {RGBAI(255, 255, 255, 209),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(255, 255, 255, 209)},
                                         {RGBAI(255, 255, 255, 235),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(255, 255, 255, 235)},
                                         {RGBAI(253, 253, 253, 255),
                                          RGBAI(255, 255, 255, 254),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(252, 252, 252, 254),
                                          RGBAI(255, 255, 255, 254),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 254),
                                          RGBAI(253, 253, 253, 255)},
                                         {RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 254),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(255, 255, 255, 254),
                                          RGBAI(0, 0, 0, 254),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 254),
                                          RGBAI(255, 255, 255, 255)},
                                         {RGBAI(255, 255, 255, 235),
                                          RGBAI(6, 6, 6, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(6, 6, 6, 255),
                                          RGBAI(255, 255, 255, 235)},
                                         {RGBAI(255, 255, 255, 209),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(255, 255, 255, 209)},
                                         {RGBAI(255, 255, 255, 156),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(255, 255, 255, 156)},
                                         {RGBAI(255, 255, 255, 89),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(255, 255, 255, 89)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(255, 255, 255, 0),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(255, 255, 255, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(72, 72, 72, 255),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 186),
                                          RGBAI(72, 72, 72, 255),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(255, 255, 255, 37),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 32),
                                          RGBAI(0, 0, 0, 227),
                                          RGBAI(92, 92, 92, 255),
                                          RGBAI(255, 255, 255, 247),
                                          RGBAI(255, 255, 255, 37),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 20),
                                          RGBAI(0, 0, 0, 164),
                                          RGBAI(2, 2, 2, 255),
                                          RGBAI(171, 171, 171, 255),
                                          RGBAI(255, 255, 255, 228),
                                          RGBAI(255, 255, 255, 24),
                                          RGBAI(255, 255, 255, 1),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 46),
                                          RGBAI(0, 0, 0, 114),
                                          RGBAI(0, 0, 0, 194),
                                          RGBAI(3, 3, 3, 255),
                                          RGBAI(127, 127, 127, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 142),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(0, 0, 0, 255),
                                          RGBAI(255, 255, 255, 254),
                                          RGBAI(0, 0, 0, 254),
                                          RGBAI(6, 6, 6, 255),
                                          RGBAI(39, 39, 39, 255),
                                          RGBAI(105, 105, 105, 255),
                                          RGBAI(182, 182, 182, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 160),
                                          RGBAI(255, 255, 255, 21),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)},
                                         {RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 89),
                                          RGBAI(255, 255, 255, 156),
                                          RGBAI(255, 255, 255, 209),
                                          RGBAI(255, 255, 255, 235),
                                          RGBAI(253, 253, 253, 255),
                                          RGBAI(255, 255, 255, 255),
                                          RGBAI(255, 255, 255, 235),
                                          RGBAI(255, 255, 255, 209),
                                          RGBAI(255, 255, 255, 156),
                                          RGBAI(255, 255, 255, 89),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(255, 255, 255, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0),
                                          RGBAI(0, 0, 0, 0)}};
    Image retval(XSize, YSize);
    for(int y = 0; y < YSize; y++)
    {
        for(int x = 0; x < XSize; x++)
        {
            retval.setPixel(x, y, pixels[y][x]);
        }
    }
    return retval;
}

Image getTouchTexture()
{
    static Image retval = makeTouchTexture();
    return retval;
}

void finishDrawingRenderLayers()
{
    if(touchSimulationState)
    {
        Display::initOverlay();
        ColorF colorizeColor = colorizeIdentity();
        Image texture = getTouchTexture();
        float w = (float)texture.width() / (float)xResInternal;
        float h = (float)texture.height() / (float)yResInternal;
        Mesh touchMesh = Generate::quadrilateral(TextureDescriptor(texture),
                                                 VectorF(-w, -h, 0.0f),
                                                 colorizeColor,
                                                 VectorF(w, -h, 0.0f),
                                                 colorizeColor,
                                                 VectorF(w, h, 0.0f),
                                                 colorizeColor,
                                                 VectorF(-w, h, 0.0f),
                                                 colorizeColor);
        Mesh mesh;
        for(std::pair<const int, TouchSimulationTouch> &p : touchSimulationState->touches)
        {
            TouchSimulationTouch &touch = std::get<1>(p);
            mesh.append(transform(Matrix::translate(touch.x, -touch.y, -1.0f), touchMesh));
        }
        Display::render(
            mesh, Matrix::scale(Display::scaleX(), Display::scaleY(), 1.0f), RenderLayer::Opaque);
    }
    if(usingOpenGLFramebuffers)
    {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
#if defined(GRAPHICS_OPENGL_ES)
        glOrthof
#else
        glOrtho
#endif
            (-1, 1, -1, 1, -1, 1);
        fnGlBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        setArrayBufferBinding(0);
        for(RenderLayer rl : enum_traits<RenderLayer>())
        {
            if(!renderLayerNeedsSeperateRenderTarget(rl))
                continue;
            glBindTexture(GL_TEXTURE_2D, renderLayerTexture[rl]);
            ColorF colorizeColor = GrayscaleAF(1, 1);
            Mesh mesh = Generate::quadrilateral(
                TextureDescriptor(Image(),
                                  0,
                                  (float)xResInternal / renderLayerTextureW,
                                  0,
                                  (float)yResInternal / renderLayerTextureH),
                VectorF(-1, -1, 0),
                colorizeColor,
                VectorF(1, -1, 0),
                colorizeColor,
                VectorF(1, 1, 0),
                colorizeColor,
                VectorF(-1, 1, 0),
                colorizeColor);
            renderInternal(mesh.triangles.data(), mesh.size());
        }
        glPopMatrix();
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
}

void setArrayBufferBinding(GLuint buffer)
{
    static GLuint lastBuffer = 0;
    static std::uint64_t bufferGrahicsContextId = 0;
    if(haveOpenGLBuffersWithoutMap)
    {
        auto currentGrahicsContextId = getGraphicsContextId();
        if(bufferGrahicsContextId != currentGrahicsContextId)
        {
            bufferGrahicsContextId = currentGrahicsContextId;
            lastBuffer = 0;
        }
        if(buffer != lastBuffer)
        {
            fnGLBindBuffer(GL_ARRAY_BUFFER, buffer);
            lastBuffer = buffer;
        }
    }
}

void getOpenGLExtensions()
{
    std::string supportedExtensionsString =
        reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
    const auto extensionDisplayPrefix = L"Extension: ";
    bool wrotePrefix = false;
    for(char ch : supportedExtensionsString)
    {
        if(ch == ' ')
        {
            getDebugLog() << postnl;
            wrotePrefix = false;
        }
        else
        {
            if(!wrotePrefix)
            {
                getDebugLog() << extensionDisplayPrefix;
                wrotePrefix = true;
            }
            getDebugLog() << (wchar_t)ch;
        }
    }
    getDebugLog() << postnl;
#ifdef GRAPHICS_OPENGL
    haveOpenGLBuffersWithoutMap =
        SDL_GL_ExtensionSupported("GL_ARB_vertex_buffer_object") ? true : false;
    haveOpenGLBuffersWithMap = haveOpenGLBuffersWithoutMap;
    if(haveOpenGLBuffersWithMap)
    {
        fnGLBindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBufferARB");
        fnGLDeleteBuffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffersARB");
        fnGLGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffersARB");
        fnGLBufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferDataARB");
        fnGLBufferSubData = (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubDataARB");
        fnGLMapBuffer = (PFNGLMAPBUFFERPROC)SDL_GL_GetProcAddress("glMapBufferARB");
        fnGLUnmapBuffer = (PFNGLUNMAPBUFFERPROC)SDL_GL_GetProcAddress("glUnmapBufferARB");
        if(!fnGLBindBuffer || !fnGLDeleteBuffers || !fnGLGenBuffers || !fnGLBufferData
           || !fnGLBufferSubData || !fnGLMapBuffer || !fnGLUnmapBuffer)
        {
            getDebugLog() << L"couldn't load OpenGL buffers extension functions" << postnl;
            haveOpenGLBuffersWithMap = false;
            haveOpenGLBuffersWithoutMap = false;
            fnGLBindBuffer = nullptr;
            fnGLDeleteBuffers = nullptr;
            fnGLGenBuffers = nullptr;
            fnGLBufferData = nullptr;
            fnGLBufferSubData = nullptr;
            fnGLMapBuffer = nullptr;
            fnGLUnmapBuffer = nullptr;
        }
    }
    else
    {
        const char *versionString = reinterpret_cast<const char *>(glGetString(GL_VERSION));
        if(versionString)
        {
            int major = 0, minor = 0;
            const char *p = versionString;
            for(; *p >= '0' && *p <= '9'; p++)
            {
                major *= 10;
                major += *p - '0';
            }
            if(*p == '.')
            {
                for(p++; *p >= '0' && *p <= '9'; p++)
                {
                    minor *= 10;
                    minor += *p - '0';
                }
            }
            if(major > 1 || (major == 1 && minor >= 5))
            {
                fnGLBindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
                fnGLDeleteBuffers =
                    (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
                fnGLGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
                fnGLBufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
                fnGLBufferSubData =
                    (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubData");
                fnGLMapBuffer = (PFNGLMAPBUFFERPROC)SDL_GL_GetProcAddress("glMapBuffer");
                fnGLUnmapBuffer = (PFNGLUNMAPBUFFERPROC)SDL_GL_GetProcAddress("glUnmapBuffer");
                haveOpenGLBuffersWithMap = true;
                haveOpenGLBuffersWithoutMap = true;
                if(!fnGLBindBuffer || !fnGLDeleteBuffers || !fnGLGenBuffers || !fnGLBufferData
                   || !fnGLBufferSubData || !fnGLMapBuffer || !fnGLUnmapBuffer)
                {
                    getDebugLog() << L"couln't load OpenGL buffers functions" << postnl;
                    haveOpenGLBuffersWithMap = false;
                    haveOpenGLBuffersWithoutMap = false;
                    fnGLBindBuffer = nullptr;
                    fnGLDeleteBuffers = nullptr;
                    fnGLGenBuffers = nullptr;
                    fnGLBufferData = nullptr;
                    fnGLBufferSubData = nullptr;
                    fnGLMapBuffer = nullptr;
                    fnGLUnmapBuffer = nullptr;
                }
            }
        }
    }
    haveOpenGLFramebuffers = SDL_GL_ExtensionSupported("GL_EXT_framebuffer_object") ? true : false;
    if(haveOpenGLFramebuffers)
    {
        fnGlGenFramebuffersEXT =
            (PFNGLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersEXT");
        fnGlDeleteFramebuffersEXT =
            (PFNGLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
        fnGlGenRenderbuffersEXT =
            (PFNGLGENRENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
        fnGlDeleteRenderbuffersEXT =
            (PFNGLDELETERENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
        fnGlBindFramebufferEXT =
            (PFNGLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
        fnGlBindRenderbufferEXT =
            (PFNGLBINDRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindRenderbufferEXT");
        fnGlFramebufferTexture2DEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
        fnGlRenderbufferStorageEXT =
            (PFNGLRENDERBUFFERSTORAGEEXTPROC)SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
        fnGlFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress(
            "glFramebufferRenderbufferEXT");
        fnGlCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress(
            "glCheckFramebufferStatusEXT");
        if(!fnGlGenFramebuffersEXT || !fnGlDeleteFramebuffersEXT || !fnGlGenRenderbuffersEXT
           || !fnGlDeleteRenderbuffersEXT || !fnGlBindFramebufferEXT || !fnGlBindRenderbufferEXT
           || !fnGlFramebufferTexture2DEXT || !fnGlRenderbufferStorageEXT
           || !fnGlFramebufferRenderbufferEXT || !fnGlCheckFramebufferStatusEXT)
        {
            getDebugLog() << L"couln't load OpenGL framebuffers extension" << postnl;
            haveOpenGLFramebuffers = false;
            fnGlGenFramebuffersEXT = nullptr;
            fnGlDeleteFramebuffersEXT = nullptr;
            fnGlGenRenderbuffersEXT = nullptr;
            fnGlDeleteRenderbuffersEXT = nullptr;
            fnGlBindFramebufferEXT = nullptr;
            fnGlBindRenderbufferEXT = nullptr;
            fnGlFramebufferTexture2DEXT = nullptr;
            fnGlRenderbufferStorageEXT = nullptr;
            fnGlFramebufferRenderbufferEXT = nullptr;
            fnGlCheckFramebufferStatusEXT = nullptr;
        }
    }
    haveOpenGLArbitraryTextureSize =
        SDL_GL_ExtensionSupported("ARB_texture_non_power_of_two") ? true : false;
#elif defined(GRAPHICS_OPENGL_ES)
    haveOpenGLBuffersWithoutMap = true;
    fnGLBindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
    fnGLDeleteBuffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
    fnGLGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
    fnGLBufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
    fnGLBufferSubData = (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubData");
    haveOpenGLBuffersWithMap = SDL_GL_ExtensionSupported("GL_OES_mapbuffer") ? true : false;
    if(!fnGLBindBuffer || !fnGLDeleteBuffers || !fnGLGenBuffers || !fnGLBufferData
       || !fnGLBufferSubData)
    {
        getDebugLog() << L"couln't load OpenGL buffers extension functions" << postnl;
        haveOpenGLBuffersWithMap = false;
        haveOpenGLBuffersWithoutMap = false;
        fnGLBindBuffer = nullptr;
        fnGLDeleteBuffers = nullptr;
        fnGLGenBuffers = nullptr;
        fnGLBufferData = nullptr;
        fnGLBufferSubData = nullptr;
        fnGLMapBuffer = nullptr;
        fnGLUnmapBuffer = nullptr;
    }
    else if(haveOpenGLBuffersWithMap)
    {
        fnGLMapBuffer = (PFNGLMAPBUFFERPROC)SDL_GL_GetProcAddress("glMapBufferOES");
        fnGLUnmapBuffer = (PFNGLUNMAPBUFFERPROC)SDL_GL_GetProcAddress("glUnmapBufferOES");
        if(!fnGLMapBuffer || !fnGLUnmapBuffer)
        {
            getDebugLog() << L"couln't load OpenGL map buffers extension functions" << postnl;
            haveOpenGLBuffersWithMap = false;
            fnGLMapBuffer = nullptr;
            fnGLUnmapBuffer = nullptr;
        }
    }
    haveOpenGLFramebuffers = SDL_GL_ExtensionSupported("GL_OES_framebuffer_object") ? true : false;
    if(haveOpenGLFramebuffers)
    {
        fnGlGenFramebuffersEXT =
            (PFNGLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersOES");
        fnGlDeleteFramebuffersEXT =
            (PFNGLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersOES");
        fnGlGenRenderbuffersEXT =
            (PFNGLGENRENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenRenderbuffersOES");
        fnGlDeleteRenderbuffersEXT =
            (PFNGLDELETERENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffersOES");
        fnGlBindFramebufferEXT =
            (PFNGLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferOES");
        fnGlBindRenderbufferEXT =
            (PFNGLBINDRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindRenderbufferOES");
        fnGlFramebufferTexture2DEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DOES");
        fnGlRenderbufferStorageEXT =
            (PFNGLRENDERBUFFERSTORAGEEXTPROC)SDL_GL_GetProcAddress("glRenderbufferStorageOES");
        fnGlFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress(
            "glFramebufferRenderbufferOES");
        fnGlCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress(
            "glCheckFramebufferStatusOES");
        if(!fnGlGenFramebuffersEXT || !fnGlDeleteFramebuffersEXT || !fnGlGenRenderbuffersEXT
           || !fnGlDeleteRenderbuffersEXT || !fnGlBindFramebufferEXT || !fnGlBindRenderbufferEXT
           || !fnGlFramebufferTexture2DEXT || !fnGlRenderbufferStorageEXT
           || !fnGlFramebufferRenderbufferEXT || !fnGlCheckFramebufferStatusEXT)
        {
            getDebugLog() << L"couln't load OpenGL framebuffers extension" << postnl;
            haveOpenGLFramebuffers = false;
            fnGlGenFramebuffersEXT = nullptr;
            fnGlDeleteFramebuffersEXT = nullptr;
            fnGlGenRenderbuffersEXT = nullptr;
            fnGlDeleteRenderbuffersEXT = nullptr;
            fnGlBindFramebufferEXT = nullptr;
            fnGlBindRenderbufferEXT = nullptr;
            fnGlFramebufferTexture2DEXT = nullptr;
            fnGlRenderbufferStorageEXT = nullptr;
            fnGlFramebufferRenderbufferEXT = nullptr;
            fnGlCheckFramebufferStatusEXT = nullptr;
        }
    }
    haveOpenGLArbitraryTextureSize =
        SDL_GL_ExtensionSupported("GL_OES_texture_npot") ? true : false;
#else
#error invalid graphics
#endif
}

struct FreeBuffer final
{
    GLuint buffer;
    bool needUnmap;
    FreeBuffer(GLuint buffer, bool needUnmap = false) : buffer(buffer), needUnmap(needUnmap)
    {
    }
};

static vector<FreeBuffer> &freeBuffers()
{
    static vector<FreeBuffer> retval;
    return retval;
}
static mutex &freeBuffersLock()
{
    static mutex retval;
    return retval;
}
GLuint allocateBuffer()
{
    assert(haveOpenGLBuffersWithoutMap);
    freeBuffersLock().lock();
    if(freeBuffers().empty())
    {
        freeBuffersLock().unlock();
        GLuint retval;
        fnGLGenBuffers(1, &retval);
        return retval;
    }
    FreeBuffer retval = freeBuffers().back();
    freeBuffers().pop_back();
    freeBuffersLock().unlock();
    if(retval.needUnmap)
    {
        assert(haveOpenGLBuffersWithMap);
        setArrayBufferBinding(retval.buffer);
        fnGLUnmapBuffer(GL_ARRAY_BUFFER);
    }
    return retval.buffer;
}
void freeBuffer(GLuint displayList, bool unmap)
{
    lock_guard<mutex> lockGuard(freeBuffersLock());
    freeBuffers().push_back(displayList);
}
}

struct MeshBufferImp
{
    virtual void render(Matrix tform, RenderLayer rl) = 0;
    virtual ~MeshBufferImp() = default;
    virtual bool set(const Mesh &mesh, bool isFinal) = 0;
    virtual bool empty() const = 0;
    virtual std::size_t capacity() const = 0;
};

namespace
{
struct MeshBufferImpOpenGLBuffer final : public MeshBufferImp
{
    MeshBufferImpOpenGLBuffer(const MeshBufferImpOpenGLBuffer &) = delete;
    MeshBufferImpOpenGLBuffer &operator=(const MeshBufferImpOpenGLBuffer &) = delete;
    GLuint buffer;
    Image image;
    std::size_t allocatedTriangleCount, usedTriangleCount = 0;
    void *bufferMemory = nullptr;
    bool gotFinalSet = false;
    std::uint64_t bufferGraphicsContextId;
    MeshBufferImpOpenGLBuffer(std::size_t triangleCount)
        : buffer(allocateBuffer()),
          image(),
          allocatedTriangleCount(triangleCount),
          bufferGraphicsContextId(getGraphicsContextId())
    {
        assert(haveOpenGLBuffersWithMap);
        setArrayBufferBinding(buffer);
        fnGLBufferData(GL_ARRAY_BUFFER, sizeof(Triangle) * triangleCount, nullptr, GL_DYNAMIC_DRAW);
        bufferMemory = fnGLMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if(bufferMemory == nullptr)
        {
            cerr << "error : can't map opengl buffer\n" << flush;
            abort();
        }
    }
    MeshBufferImpOpenGLBuffer(const Mesh &mesh)
        : buffer(allocateBuffer()),
          image(mesh.image),
          allocatedTriangleCount(mesh.size()),
          bufferMemory(nullptr),
          gotFinalSet(true),
          bufferGraphicsContextId(getGraphicsContextId())
    {
        assert(haveOpenGLBuffersWithoutMap);
        setArrayBufferBinding(buffer);
        fnGLBufferData(GL_ARRAY_BUFFER,
                       sizeof(Triangle) * mesh.size(),
                       mesh.triangles.data(),
                       GL_DYNAMIC_DRAW);
    }
    virtual ~MeshBufferImpOpenGLBuffer()
    {
        if(bufferGraphicsContextId == getGraphicsContextId())
            freeBuffer(buffer, bufferMemory != nullptr);
    }
    virtual void render(Matrix tform, RenderLayer rl) override
    {
        if(usedTriangleCount == 0)
            return;
        image.bind();
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrix(tform);
        setArrayBufferBinding(buffer);
        if(bufferMemory != nullptr)
        {
            assert(haveOpenGLBuffersWithMap);
            fnGLUnmapBuffer(GL_ARRAY_BUFFER);
            bufferMemory = nullptr;
        }
        GLint vertexCount = usedTriangleCount * 3;
        renderToRenderLayer(
            rl,
            [vertexCount]()
            {
                const char *array_start = (const char *)0;
                glVertexPointer(
                    3, GL_FLOAT, Triangle_vertex_stride, array_start + Triangle_position_start);
                glTexCoordPointer(2,
                                  GL_FLOAT,
                                  Triangle_vertex_stride,
                                  array_start + Triangle_texture_coord_start);
                glColorPointer(
                    4, GL_FLOAT, Triangle_vertex_stride, array_start + Triangle_color_start);
                glDrawArrays(GL_TRIANGLES, 0, vertexCount);
            });
        glLoadIdentity();
        if(!gotFinalSet)
        {
            assert(haveOpenGLBuffersWithMap);
            bufferMemory = fnGLMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            if(bufferMemory == nullptr)
            {
                cerr << "error : can't map opengl buffer\n" << flush;
                abort();
            }
        }
    }
    virtual bool set(const Mesh &mesh, bool isFinal) override
    {
        if(gotFinalSet)
            return false;
        if(mesh.triangles.size() > allocatedTriangleCount)
            return false;
        assert(bufferMemory);
        image = mesh.image;
        Triangle *triangles = (Triangle *)bufferMemory;
        for(std::size_t i = 0; i < mesh.triangles.size(); i++)
        {
            triangles[i] = mesh.triangles[i];
        }
        usedTriangleCount = mesh.triangles.size();
        gotFinalSet = isFinal;
        return true;
    }
    virtual bool empty() const override
    {
        return usedTriangleCount == 0;
    }
    virtual std::size_t capacity() const override
    {
        return allocatedTriangleCount;
    }
};

struct MeshBufferImpFallback final : public MeshBufferImp
{
    std::size_t allocatedTriangleCount;
    bool gotFinalSet;
    Mesh mesh;
    MeshBufferImpFallback(std::size_t triangleCount)
        : allocatedTriangleCount(triangleCount), gotFinalSet(false), mesh()
    {
        mesh.triangles.reserve(triangleCount);
    }
    virtual ~MeshBufferImpFallback()
    {
    }
    virtual void render(Matrix tform, RenderLayer rl) override
    {
        Display::render(mesh, tform, rl);
    }
    virtual bool set(const Mesh &mesh, bool isFinal) override
    {
        if(gotFinalSet)
            return false;
        if(mesh.triangles.size() > allocatedTriangleCount)
            return false;
        this->mesh.clear();
        this->mesh.append(mesh);
        gotFinalSet = isFinal;
        return true;
    }
    virtual bool empty() const override
    {
        return mesh.size() == 0;
    }
    virtual std::size_t capacity() const override
    {
        return allocatedTriangleCount;
    }
};
}

void Display::render(const Mesh &m, Matrix tform, RenderLayer rl)
{
#if 0
    getDebugLog() << L"Display::render(<size = " << m.size() << L">, ..., ";
    switch(rl)
    {
    case RenderLayer::Opaque:
        getDebugLog() << L"RenderLayer::Opaque";
        break;
    case RenderLayer::Translucent:
        getDebugLog() << L"RenderLayer::Translucent";
        break;
    }
    getDebugLog() << L")" << postnl;
#endif
    if(m.size() == 0)
        return;
    const Triangle *triangles = m.triangles.data();
#if 1
    if(haveOpenGLBuffersWithoutMap)
    {
        const std::size_t bufferCount = 32;
        static GLuint buffers[bufferCount];
        static std::size_t bufferSizes[bufferCount];
        static bool didGenerateBuffers = false;
        static std::size_t currentBufferIndex = 0;
        static std::uint64_t bufferGraphicsContextId = 0;
        auto currentGraphicsContextId = getGraphicsContextId();
        if(bufferGraphicsContextId != currentGraphicsContextId)
        {
            bufferGraphicsContextId = currentGraphicsContextId;
            didGenerateBuffers = false;
            currentBufferIndex = 0;
        }
        if(!didGenerateBuffers)
        {
            didGenerateBuffers = true;
            fnGLGenBuffers(bufferCount, buffers);
            for(std::size_t &bufferSize : bufferSizes)
            {
                bufferSize = 0;
            }
        }
        setArrayBufferBinding(buffers[currentBufferIndex]);
        if(bufferSizes[currentBufferIndex] < m.size())
        {
            bufferSizes[currentBufferIndex] *= 2;
            if(bufferSizes[currentBufferIndex] < m.size())
                bufferSizes[currentBufferIndex] = m.size();
            fnGLBufferData(GL_ARRAY_BUFFER,
                           sizeof(Triangle) * bufferSizes[currentBufferIndex],
                           nullptr,
                           GL_DYNAMIC_DRAW);
        }
        fnGLBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Triangle) * m.size(), triangles);
        triangles = nullptr;
        currentBufferIndex++;
        currentBufferIndex %= bufferCount;
    }
    else
    {
        setArrayBufferBinding(0);
    }
#else
    FIXME_MESSAGE(disabled using buffer for all rendering)
    setArrayBufferBinding(0);
#endif
    m.image.bind();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrix(tform);
    renderToRenderLayer(rl,
                        [&m, triangles]()
                        {
                            renderInternal(triangles, m.triangles.size());
                        });
    glPopMatrix();
}

void Display::initFrame()
{
    SDL_GetWindowSize(window, &xResInternal, &yResInternal);
    if(width() > height())
    {
        scaleXInternal = static_cast<float>(width()) / height();
        scaleYInternal = 1.0;
    }
    else
    {
        scaleXInternal = 1.0;
        scaleYInternal = static_cast<float>(height()) / width();
    }
    updateRenderLayersSize();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_CULL_FACE);
    if(usingOpenGLFramebuffers)
    {
        glDisable(GL_BLEND);
    }
    else
    {
        glEnable(GL_BLEND);
    }
    glDepthFunc(GL_LEQUAL);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glAlphaFunc(GL_GREATER, 0.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glViewport(0, 0, width(), height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float minDistance = 5e-2f, maxDistance = 200.0f;
#if defined(GRAPHICS_OPENGL_ES)
    glFrustumf
#else
    glFrustum
#endif
        (-minDistance * scaleX(),
         minDistance * scaleX(),
         -minDistance * scaleY(),
         minDistance * scaleY(),
         minDistance,
         maxDistance);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
}

void Display::clear(ColorF color)
{
    initFrame();
    glClearColor(color.r, color.g, color.b, color.a);
    clearRenderLayers(false);
}

void Display::initOverlay()
{
    clearRenderLayers(true);
}

bool MeshBuffer::impIsEmpty(std::shared_ptr<MeshBufferImp> mesh)
{
    return mesh->empty();
}

std::size_t MeshBuffer::impCapacity(std::shared_ptr<MeshBufferImp> mesh)
{
    return mesh->capacity();
}

MeshBuffer::MeshBuffer(std::size_t triangleCount) : imp(), tform(Matrix::identity())
{
    if(triangleCount == 0)
        imp = nullptr;
    else if(haveOpenGLBuffersWithMap)
        imp = std::make_shared<MeshBufferImpOpenGLBuffer>(triangleCount);
    else
        imp = std::make_shared<MeshBufferImpFallback>(triangleCount);
}

bool MeshBuffer::set(const Mesh &mesh, bool isFinal)
{
    if(!imp)
        return false;
    return imp->set(mesh, isFinal);
}

float Display::screenRefreshRate()
{
    int displayIndex = SDL_GetWindowDisplayIndex(window);
    if(displayIndex == -1)
        displayIndex = 0;
    SDL_DisplayMode mode;
    if(0 != SDL_GetCurrentDisplayMode(displayIndex, &mode))
        return defaultFPS;
    if(mode.refresh_rate == 0)
        return defaultFPS;
    return (float)mode.refresh_rate;
}

void Display::render(const MeshBuffer &m, RenderLayer rl)
{
    if(!m.imp)
        return;
    m.imp->render(m.tform, rl);
}

static void getExtensions()
{
    getOpenGLExtensions();
}

static std::vector<std::uint32_t> *freeTextures = nullptr;
static std::mutex freeTexturesLock;

std::uint32_t allocateTexture()
{
    std::uint32_t retval;
    std::unique_lock<std::mutex> lockIt(freeTexturesLock);
    if(freeTextures == nullptr)
        freeTextures = new std::vector<std::uint32_t>();
    if(freeTextures->empty())
    {
        lockIt.unlock();
        glGenTextures(1, (GLuint *)&retval);
        return retval;
    }
    retval = freeTextures->back();
    freeTextures->pop_back();
    lockIt.unlock();
    return retval;
}

void freeTexture(std::uint32_t texture)
{
    if(texture == 0)
        return;
    std::unique_lock<std::mutex> lockIt(freeTexturesLock);
    if(freeTextures == nullptr)
        freeTextures = new std::vector<std::uint32_t>();
    freeTextures->push_back(texture);
}

void setThreadPriority(ThreadPriority priority)
{
    SDL_ThreadPriority sdlPriority = SDL_THREAD_PRIORITY_NORMAL;
    switch(priority)
    {
    case ThreadPriority::High:
        sdlPriority = SDL_THREAD_PRIORITY_HIGH;
        break;
    case ThreadPriority::Normal:
        sdlPriority = SDL_THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::Low:
        sdlPriority = SDL_THREAD_PRIORITY_LOW;
        break;
    }
    SDL_SetThreadPriority(sdlPriority);
}

bool Display::fullScreen()
{
    return isFullScreen;
}

void Display::fullScreen(bool fs)
{
    isFullScreen = fs;
    SDL_SetWindowFullscreen(window, isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

bool Display::needTouchControls()
{
    if(touchSimulationState)
        return true;
    if(SDL_GetNumTouchDevices() > 0)
        return true;
    return false;
}

DynamicLinkLibrary::DynamicLinkLibrary(std::wstring name)
    : handle(SDL_LoadObject(string_cast<std::string>(name).c_str()))
{
}

void DynamicLinkLibrary::destroy()
{
    SDL_UnloadObject(handle);
}

void *DynamicLinkLibrary::resolve(std::wstring name)
{
    if(handle == nullptr)
        throw std::logic_error("resolve called on empty DynamicLinkLibrary");
    void *retval = SDL_LoadFunction(handle, string_cast<std::string>(name).c_str());
    if(retval == nullptr)
        throw SymbolNotFoundException("symbol not found");
    return retval;
}

bool Display::Text::active()
{
    return SDL_IsTextInputActive() ? true : false;
}

void Display::Text::start(float minX, float maxX, float minY, float maxY)
{
    SDL_StartTextInput();
    VectorF corner1 = Display::transform3DToMouse(VectorF(minX, minY, -1.0f));
    VectorF corner2 = Display::transform3DToMouse(VectorF(maxX, maxY, -1.0f));
    VectorF minCorner, maxCorner;
    minCorner.x = std::min(corner1.x, corner2.x);
    minCorner.y = std::min(corner1.y, corner2.y);
    maxCorner.x = std::max(corner1.x, corner2.x);
    maxCorner.y = std::max(corner1.y, corner2.y);
    SDL_Rect rect;
    rect.x = (int)minCorner.x;
    rect.y = (int)minCorner.y;
    rect.w = (int)(maxCorner.x - minCorner.x);
    rect.h = (int)(maxCorner.y - minCorner.y);
    SDL_SetTextInputRect(&rect);
}

void Display::Text::stop()
{
    SDL_StopTextInput();
}

namespace
{
struct TemporaryFile final
{
    TemporaryFile(const TemporaryFile &) = delete;
    TemporaryFile &operator=(const TemporaryFile &) = delete;
    stream::FileReader *fileReader = nullptr;
    stream::FileWriter *fileWriter = nullptr;
    const std::string deleteFileName;
    TemporaryFile(FILE *readFile, FILE *writeFile, std::wstring fileName)
        : deleteFileName(string_cast<std::string>(fileName))
    {
        if(writeFile)
            fileWriter = new stream::FileWriter(writeFile);
        else
            fileWriter = new stream::FileWriter(fileName); // create writer first to truncate file
        try
        {
            if(readFile)
                fileReader = new stream::FileReader(readFile);
            else
                fileReader = new stream::FileReader(fileName);
        }
        catch(...)
        {
            delete fileWriter;
            throw;
        }
    }
    ~TemporaryFile()
    {
        delete fileReader;
        delete fileWriter;
        std::remove(deleteFileName.c_str());
    }
    static std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>> getRW(
        std::shared_ptr<TemporaryFile> tempFile)
    {
        return std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>>(
            std::shared_ptr<stream::Reader>(tempFile, tempFile->fileReader),
            std::shared_ptr<stream::Writer>(tempFile, tempFile->fileWriter));
    }
};
}

std::size_t getProcessorCount()
{
    startSDL();
    return processorCount;
}
}
}

#if _WIN64 || _WIN32
namespace programmerjake
{
namespace voxels
{
std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>> createTemporaryFile()
{
    const DWORD bufferSize = MAX_PATH + 1;
    static thread_local char pathBuffer[bufferSize];
    DWORD pathRetval = ::GetTempPathA(bufferSize, &pathBuffer[0]);
    if(pathRetval == 0 || pathRetval > bufferSize)
        throw stream::IOException("GetTempPathA failed");
    static thread_local char fileNameBuffer[bufferSize];
    if(0 == ::GetTempFileNameA(pathBuffer, "tmp", 0, &fileNameBuffer[0]))
    {
        throw stream::IOException("GetTempFileNameA failed");
    }
    std::wstring fileName = string_cast<std::wstring>(std::string(fileNameBuffer));
    std::shared_ptr<TemporaryFile> tempFile =
        std::make_shared<TemporaryFile>(nullptr, nullptr, fileName);
    return TemporaryFile::getRW(tempFile);
}
}
}
#elif __ANDROID__
namespace programmerjake
{
namespace voxels
{
std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>> createTemporaryFile()
{
    static std::atomic_size_t nextFileNumber(0);
    const char *internalStoragePath = SDL_AndroidGetInternalStoragePath();
    if(internalStoragePath == nullptr)
        throw stream::IOException(std::string("SDL_AndroidGetInternalStoragePath failed: ")
                                  + SDL_GetError());

    std::ostringstream ss;
    ss << internalStoragePath << "/tmp" << nextFileNumber++ << ".tmp";
    std::wstring fileName = string_cast<std::wstring>(ss.str());
    std::shared_ptr<stream::Writer> writer =
        std::make_shared<stream::FileWriter>(fileName); // writer first to truncate; we don't need
    // to keep file contents around because we
    // are the only one using them
    std::shared_ptr<stream::Reader> reader = std::make_shared<stream::FileReader>(fileName);
    remove(ss.str().c_str()); // removes directory entry; linux keeps file around until file isn't
    // open by anything
    return std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>>(reader,
                                                                                       writer);
}
}
}
#elif __APPLE__ || __linux || __unix || __posix
#include <fcntl.h>
namespace programmerjake
{
namespace voxels
{
std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>> createTemporaryFile()
{
    unsigned fileNumber = std::time(nullptr);
    const unsigned startFileNumber = fileNumber;
    const unsigned fileNumberMod = 100000;
    fileNumber %= fileNumberMod;
    std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>> retval;
    for(;;)
    {
        std::ostringstream ss;
        ss << "/tmp/tmp" << fileNumber << ".tmp";
        std::wstring fileName = string_cast<std::wstring>(ss.str());
        int writeFileDescriptor =
            open(ss.str().c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if(writeFileDescriptor == -1)
        {
            if(errno != EEXIST)
                stream::IOException::throwErrorFromErrno("createTemporaryFile: open");
            fileNumber++;
            fileNumber %= fileNumberMod;
            if(fileNumber == startFileNumber)
                throw stream::IOException("can't create temporary file");
            continue;
        }
        FILE *writeFile = fdopen(writeFileDescriptor, "wb");
        if(!writeFile)
        {
            close(writeFileDescriptor);
            unlink(ss.str().c_str());
            stream::IOException::throwErrorFromErrno("createTemporaryFile: fdopen");
        }
        std::shared_ptr<stream::Writer> writer;
        try
        {
            writer = std::make_shared<stream::FileWriter>(writeFile); // writer first to truncate
        }
        catch(...)
        {
            std::fclose(writeFile);
            unlink(ss.str().c_str());
            throw;
        }
        std::shared_ptr<stream::Reader> reader;
        try
        {
            reader = std::make_shared<stream::FileReader>(fileName);
        }
        catch(...)
        {
            writer = nullptr;
            unlink(ss.str().c_str());
            throw;
        }
        unlink(ss.str().c_str()); // removes directory entry; posix keeps file around until file
        // isn't open by anything
        retval = std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>>(
            reader, writer);
        break;
    }
    return retval;
}
}
}
#else
#error unknown platform in createTemporaryFile
#endif

#if _WIN64 || _WIN32
namespace programmerjake
{
namespace voxels
{
void platformPostLogMessage(std::wstring msg)
{
    while(!msg.empty())
    {
        std::size_t pos = msg.find_first_of(L"\r\n");
        std::wstring currentLine;
        if(pos == std::wstring::npos)
        {
            currentLine = msg;
            msg.clear();
        }
        else
        {
            currentLine = msg.substr(0, pos + 1);
            msg.erase(0, pos + 1);
        }
        assert(!currentLine.empty());
        if(currentLine[currentLine.size() - 1] == L'\r')
        {
            currentLine.erase(currentLine.size() - 1);
            currentLine += L"        \r";
        }
        else if(currentLine[currentLine.size() - 1] == L'\n')
        {
            currentLine.erase(currentLine.size() - 1);
            currentLine += L"        \n";
        }
        else
        {
            currentLine += L"";
        }
        std::cout << string_cast<std::string>(currentLine);
    }
    std::cout << std::flush;
}
}
}
#elif __ANDROID__
namespace programmerjake
{
namespace voxels
{
void platformPostLogMessage(std::wstring msg)
{
    startSDL();
    while(!msg.empty())
    {
        std::size_t pos = msg.find_first_of(L"\r\n");
        std::wstring currentLine;
        if(pos == std::wstring::npos)
        {
            currentLine = msg;
            msg.clear();
        }
        else
        {
            currentLine = msg.substr(0, pos + 1);
            msg.erase(0, pos + 1);
        }
        assert(!currentLine.empty());
        if(currentLine[currentLine.size() - 1] == L'\r')
        {
            currentLine.erase(currentLine.size() - 1);
        }
        else if(currentLine[currentLine.size() - 1] == L'\n')
        {
            currentLine.erase(currentLine.size() - 1);
        }
        SDL_Log("%s", string_cast<std::string>(currentLine).c_str());
    }
}
}
}
#elif __APPLE__ || __linux || __unix || __posix
#include <fcntl.h>
namespace programmerjake
{
namespace voxels
{
void platformPostLogMessage(std::wstring msg)
{
    while(!msg.empty())
    {
        std::size_t pos = msg.find_first_of(L"\r\n");
        std::wstring currentLine;
        if(pos == std::wstring::npos)
        {
            currentLine = msg;
            msg.clear();
        }
        else
        {
            currentLine = msg.substr(0, pos + 1);
            msg.erase(0, pos + 1);
        }
        assert(!currentLine.empty());
        if(currentLine[currentLine.size() - 1] == L'\r')
        {
            currentLine.erase(currentLine.size() - 1);
            currentLine += L"\n";
        }
        std::cout << string_cast<std::string>(currentLine);
    }
    std::cout << std::flush;
}
}
}
#else
#error unknown platform in platformPostLogMessage
#endif

namespace programmerjake
{
namespace voxels
{
namespace
{
int callMyMain(int argc, char **argv);
}
}
}

extern "C" int main(int argc, char *argv[])
{
    int retval;
    {
        programmerjake::voxels::runPlatformSetup();
        programmerjake::voxels::TLS tls;
        programmerjake::voxels::global_instance_maker_init_list::init_all();
        retval = programmerjake::voxels::callMyMain(argc, argv);
    }
    std::exit(retval);
    return 0;
}

#ifdef main
#undef main
#endif // main

namespace programmerjake
{
namespace voxels
{
namespace
{
int callMyMain(int argc, char **argv)
{
    std::vector<std::wstring> args;
    args.reserve(argc);
    for(int i = 0; i < argc; i++)
    {
        args.push_back(programmerjake::voxels::string_cast<std::wstring>(std::string(argv[i])));
    }
    return main(std::move(args));
}
}
}
}
