/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "platform/platform.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include "util/matrix.h"
#include "util/vector.h"
#include "util/game_version.h"
#include "util/string_cast.h"
#include <cwchar>
#include <string>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include "platform/audio.h"

#ifndef SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK
#define SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK "SDL_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK"
#endif // SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK

using namespace std;

double Display::realtimeTimer()
{
    return static_cast<double>(chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count()) * 1e-9;
}

namespace
{
struct RWOpsException : public stream::IOException
{
    RWOpsException(string str)
        : IOException(str)
    {
    }
};

class RWOpsReader final : public stream::Reader
{
private:
    SDL_RWops * rw;
public:
    RWOpsReader(SDL_RWops * rw)
        : rw(rw)
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
            const char * str = SDL_GetError();
            if(str[0]) // non-empty string : error
                throw RWOpsException(str);
            throw stream::EOFException();
        }
        return retval;
    }
    ~RWOpsReader()
    {
        SDL_RWclose(rw);
    }
};
}

static void startSDL();

#ifdef _WIN64
#error implement getResourceReader for Win64
#elif _WIN32
#error implement getResourceReader for Win32
#elif __ANDROID
#error implement getResourceReader for Android
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#error implement getResourceReader for iPhone simulator
#elif TARGET_OS_IPHONE
#error implement getResourceReader for iPhone
#else
#error implement getResourceReader for OS X
#endif
#elif __linux
#include <unistd.h>
#include <climits>
#include <cerrno>
#include <cstring>
#include <cwchar>
static atomic_bool setResourcePrefix(false);
static wstring * pResourcePrefix = nullptr;
static wstring getExecutablePath();
static void calcResourcePrefix();

static wstring getResourceFileName(wstring resource)
{
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
    string fname = string_cast<string>(getResourceFileName(resource));
    try
    {
        return make_shared<RWOpsReader>(SDL_RWFromFile(fname.c_str(), "rb"));
    }
    catch(RWOpsException & e)
    {
        throw RWOpsException("can't open resource : " + string_cast<string>(resource));
    }
}
#elif __unix
#error implement getResourceReader for other unix
#elif __posix
#error implement getResourceReader for other posix
#else
#error unknown platform in getResourceReader
#endif

static int xResInternal, yResInternal;

static SDL_Window *window = nullptr;
static SDL_GLContext glcontext = nullptr;
static atomic_bool runningGraphics(false), runningSDL(false), runningAudio(false);
static atomic_int SDLUseCount(0);
static atomic_bool addedAtExits(false);

static void startSDL()
{
    if(runningSDL.exchange(true))
        return;
    if(0 != SDL_Init(SDL_INIT_TIMER))
    {
        cerr << "error : can't start SDL : " << SDL_GetError() << endl;
        exit(1);
    }
    if(!addedAtExits.exchange(true))
    {
        atexit(endGraphics);
        atexit(endAudio);
    }
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
        if(0 != SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            cerr << "error : can't start SDL audio subsystem : " << SDL_GetError() << endl;
            SDLUseCount--;
            runningAudio = false;
            exit(1);
        }
        validAudio = true;
        SDL_AudioSpec desired;
        desired.callback = PlayingAudio::audioCallback;
        desired.channels = 6;
        desired.format = AUDIO_S16SYS;
        desired.freq = 96000;
        desired.samples = 4096;
        desired.userdata = nullptr;
        globalAudioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desired, &globalAudioSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
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
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
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

void endGraphics()
{
    if(runningGraphics.exchange(false))
    {
        SDL_GL_DeleteContext(glcontext);
        glcontext = nullptr;
        SDL_DestroyWindow(window);
        window = nullptr;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    if(--SDLUseCount <= 0)
    {
        if(runningSDL.exchange(false))
            SDL_Quit();
    }
}

static void getExtensions();

void startGraphics()
{
    if(runningGraphics.exchange(true))
        return;
    SDLUseCount++;
    startSDL();
    if(0 != SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        cerr << "error : can't start SDL video subsystem : " << SDL_GetError() << endl;
        SDLUseCount--;
        runningGraphics = false;
        exit(1);
    }
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
    window = SDL_CreateWindow("Voxels", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, xResInternal, yResInternal, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(window == nullptr)
    {
        cerr << "error : can't create window : " << SDL_GetError();
        exit(1);
    }
    glcontext = SDL_GL_CreateContext(window);
    if(glcontext == nullptr)
    {
        cerr << "error : can't create OpenGL context : " << SDL_GetError();
        exit(1);
    }
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
    getExtensions();
}

static volatile double lastFlipTime = 0;

static volatile double oldLastFlipTime = 0;

static recursive_mutex flipTimeLock;

static void lockFlipTime()
{
    flipTimeLock.lock();
}

static void unlockFlipTime()
{
    flipTimeLock.unlock();
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

static void flipDisplay(float fps = defaultFPS)
{
    double sleepTime;
    {
        FlipTimeLocker lock;
        double curTime = Display::realtimeTimer();
        sleepTime = 1 / fps - (curTime - lastFlipTime);
        if(sleepTime <= eps)
        {
            oldLastFlipTime = lastFlipTime;
            lastFlipTime = curTime;
            averageFPSInternal *= 1 - FPSUpdateFactor;
            averageFPSInternal += FPSUpdateFactor * instantaneousFPS();
        }
    }
    if(sleepTime > eps)
    {
        this_thread::sleep_for(chrono::nanoseconds(static_cast<int64_t>(sleepTime * 1e9)));
        {
            FlipTimeLocker lock;
            oldLastFlipTime = lastFlipTime;
            lastFlipTime = Display::realtimeTimer();
            averageFPSInternal *= 1 - FPSUpdateFactor;
            averageFPSInternal += FPSUpdateFactor * instantaneousFPS();
        }
    }
    SDL_GL_SwapWindow(window);
}

static KeyboardKey translateKey(SDL_Scancode input)
{
    switch(input)
    {
    case SDL_SCANCODE_BACKSPACE:
        return KeyboardKey_Backspace;
    case SDL_SCANCODE_TAB:
        return KeyboardKey_Tab;
    case SDL_SCANCODE_CLEAR:
        return KeyboardKey_Clear;
    case SDL_SCANCODE_RETURN:
        return KeyboardKey_Return;
    case SDL_SCANCODE_PAUSE:
        return KeyboardKey_Pause;
    case SDL_SCANCODE_ESCAPE:
        return KeyboardKey_Escape;
    case SDL_SCANCODE_SPACE:
        return KeyboardKey_Space;
    case SDL_SCANCODE_APOSTROPHE:
        return KeyboardKey_SQuote;
    case SDL_SCANCODE_COMMA:
        return KeyboardKey_Comma;
    case SDL_SCANCODE_MINUS:
        return KeyboardKey_Dash;
    case SDL_SCANCODE_PERIOD:
        return KeyboardKey_Period;
    case SDL_SCANCODE_SLASH:
        return KeyboardKey_FSlash;
    case SDL_SCANCODE_0:
        return KeyboardKey_Num0;
    case SDL_SCANCODE_1:
        return KeyboardKey_Num1;
    case SDL_SCANCODE_2:
        return KeyboardKey_Num2;
    case SDL_SCANCODE_3:
        return KeyboardKey_Num3;
    case SDL_SCANCODE_4:
        return KeyboardKey_Num4;
    case SDL_SCANCODE_5:
        return KeyboardKey_Num5;
    case SDL_SCANCODE_6:
        return KeyboardKey_Num6;
    case SDL_SCANCODE_7:
        return KeyboardKey_Num7;
    case SDL_SCANCODE_8:
        return KeyboardKey_Num8;
    case SDL_SCANCODE_9:
        return KeyboardKey_Num9;
    case SDL_SCANCODE_SEMICOLON:
        return KeyboardKey_Semicolon;
    case SDL_SCANCODE_EQUALS:
        return KeyboardKey_Equals;
    case SDL_SCANCODE_LEFTBRACKET:
        return KeyboardKey_LBracket;
    case SDL_SCANCODE_BACKSLASH:
        return KeyboardKey_BSlash;
    case SDL_SCANCODE_RIGHTBRACKET:
        return KeyboardKey_RBracket;
    case SDL_SCANCODE_A:
        return KeyboardKey_A;
    case SDL_SCANCODE_B:
        return KeyboardKey_B;
    case SDL_SCANCODE_C:
        return KeyboardKey_C;
    case SDL_SCANCODE_D:
        return KeyboardKey_D;
    case SDL_SCANCODE_E:
        return KeyboardKey_E;
    case SDL_SCANCODE_F:
        return KeyboardKey_F;
    case SDL_SCANCODE_G:
        return KeyboardKey_G;
    case SDL_SCANCODE_H:
        return KeyboardKey_H;
    case SDL_SCANCODE_I:
        return KeyboardKey_I;
    case SDL_SCANCODE_J:
        return KeyboardKey_J;
    case SDL_SCANCODE_K:
        return KeyboardKey_K;
    case SDL_SCANCODE_L:
        return KeyboardKey_L;
    case SDL_SCANCODE_M:
        return KeyboardKey_M;
    case SDL_SCANCODE_N:
        return KeyboardKey_N;
    case SDL_SCANCODE_O:
        return KeyboardKey_O;
    case SDL_SCANCODE_P:
        return KeyboardKey_P;
    case SDL_SCANCODE_Q:
        return KeyboardKey_Q;
    case SDL_SCANCODE_R:
        return KeyboardKey_R;
    case SDL_SCANCODE_S:
        return KeyboardKey_S;
    case SDL_SCANCODE_T:
        return KeyboardKey_T;
    case SDL_SCANCODE_U:
        return KeyboardKey_U;
    case SDL_SCANCODE_V:
        return KeyboardKey_V;
    case SDL_SCANCODE_W:
        return KeyboardKey_W;
    case SDL_SCANCODE_X:
        return KeyboardKey_X;
    case SDL_SCANCODE_Y:
        return KeyboardKey_Y;
    case SDL_SCANCODE_Z:
        return KeyboardKey_Z;
    case SDL_SCANCODE_DELETE:
        return KeyboardKey_Delete;
    case SDL_SCANCODE_KP_0:
        return KeyboardKey_KPad0;
    case SDL_SCANCODE_KP_1:
        return KeyboardKey_KPad1;
    case SDL_SCANCODE_KP_2:
        return KeyboardKey_KPad2;
    case SDL_SCANCODE_KP_3:
        return KeyboardKey_KPad3;
    case SDL_SCANCODE_KP_4:
        return KeyboardKey_KPad4;
    case SDL_SCANCODE_KP_5:
        return KeyboardKey_KPad5;
    case SDL_SCANCODE_KP_6:
        return KeyboardKey_KPad6;
    case SDL_SCANCODE_KP_7:
        return KeyboardKey_KPad7;
    case SDL_SCANCODE_KP_8:
        return KeyboardKey_KPad8;
    case SDL_SCANCODE_KP_9:
        return KeyboardKey_KPad8;
    case SDL_SCANCODE_KP_PERIOD:
        return KeyboardKey_KPadPeriod;
    case SDL_SCANCODE_KP_DIVIDE:
        return KeyboardKey_KPadFSlash;
    case SDL_SCANCODE_KP_MULTIPLY:
        return KeyboardKey_KPadStar;
    case SDL_SCANCODE_KP_MINUS:
        return KeyboardKey_KPadDash;
    case SDL_SCANCODE_KP_PLUS:
        return KeyboardKey_KPadPlus;
    case SDL_SCANCODE_KP_ENTER:
        return KeyboardKey_KPadReturn;
    case SDL_SCANCODE_KP_EQUALS:
        return KeyboardKey_KPadEquals;
    case SDL_SCANCODE_UP:
        return KeyboardKey_Up;
    case SDL_SCANCODE_DOWN:
        return KeyboardKey_Down;
    case SDL_SCANCODE_RIGHT:
        return KeyboardKey_Right;
    case SDL_SCANCODE_LEFT:
        return KeyboardKey_Left;
    case SDL_SCANCODE_INSERT:
        return KeyboardKey_Insert;
    case SDL_SCANCODE_HOME:
        return KeyboardKey_Home;
    case SDL_SCANCODE_END:
        return KeyboardKey_End;
    case SDL_SCANCODE_PAGEUP:
        return KeyboardKey_PageUp;
    case SDL_SCANCODE_PAGEDOWN:
        return KeyboardKey_PageDown;
    case SDL_SCANCODE_F1:
        return KeyboardKey_F1;
    case SDL_SCANCODE_F2:
        return KeyboardKey_F2;
    case SDL_SCANCODE_F3:
        return KeyboardKey_F3;
    case SDL_SCANCODE_F4:
        return KeyboardKey_F4;
    case SDL_SCANCODE_F5:
        return KeyboardKey_F5;
    case SDL_SCANCODE_F6:
        return KeyboardKey_F6;
    case SDL_SCANCODE_F7:
        return KeyboardKey_F7;
    case SDL_SCANCODE_F8:
        return KeyboardKey_F8;
    case SDL_SCANCODE_F9:
        return KeyboardKey_F9;
    case SDL_SCANCODE_F10:
        return KeyboardKey_F10;
    case SDL_SCANCODE_F11:
        return KeyboardKey_F11;
    case SDL_SCANCODE_F12:
        return KeyboardKey_F12;
    case SDL_SCANCODE_F13:
    case SDL_SCANCODE_F14:
    case SDL_SCANCODE_F15:
        // TODO (jacob#): implement keys
        return KeyboardKey_Unknown;
    case SDL_SCANCODE_NUMLOCKCLEAR:
        return KeyboardKey_NumLock;
    case SDL_SCANCODE_CAPSLOCK:
        return KeyboardKey_CapsLock;
    case SDL_SCANCODE_SCROLLLOCK:
        return KeyboardKey_ScrollLock;
    case SDL_SCANCODE_RSHIFT:
        return KeyboardKey_RShift;
    case SDL_SCANCODE_LSHIFT:
        return KeyboardKey_LShift;
    case SDL_SCANCODE_RCTRL:
        return KeyboardKey_RCtrl;
    case SDL_SCANCODE_LCTRL:
        return KeyboardKey_LCtrl;
    case SDL_SCANCODE_RALT:
        return KeyboardKey_RAlt;
    case SDL_SCANCODE_LALT:
        return KeyboardKey_LAlt;
    case SDL_SCANCODE_RGUI:
        return KeyboardKey_RMeta;
    case SDL_SCANCODE_LGUI:
        return KeyboardKey_LMeta;
    case SDL_SCANCODE_MODE:
        return KeyboardKey_Mode;
    case SDL_SCANCODE_HELP:
        // TODO (jacob#): implement keys
        return KeyboardKey_Unknown;
    case SDL_SCANCODE_PRINTSCREEN:
        return KeyboardKey_PrintScreen;
    case SDL_SCANCODE_SYSREQ:
        return KeyboardKey_SysRequest;
    case SDL_SCANCODE_MENU:
        return KeyboardKey_Menu;
    case SDL_SCANCODE_POWER:
    case SDL_SCANCODE_UNDO:
        // TODO (jacob#): implement keys
        return KeyboardKey_Unknown;
    default:
        return KeyboardKey_Unknown;
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

static bool &keyState(KeyboardKey key)
{
    static bool state[KeyboardKey_max + 1 - KeyboardKey_min];
    return state[static_cast<int>(key) + KeyboardKey_min];
}

static MouseButton buttonState = MouseButton_None;

static bool needQuitEvent = false;

static Event *makeEvent()
{
    static bool needCharEvent = false;
    static wstring characters;
    if(needCharEvent)
    {
        needCharEvent = false;
        wchar_t character = characters[0];
        characters.erase(0, 1);
        return new KeyPressEvent(character);
    }
    if(needQuitEvent)
    {
        needQuitEvent = false;
        return new QuitEvent();
    }
    while(true)
    {
        SDL_Event SDLEvent;
        if(SDL_PollEvent(&SDLEvent) == 0)
        {
            return nullptr;
        }
        switch(SDLEvent.type)
        {
        case SDL_KEYDOWN:
        {
            KeyboardKey key = translateKey(SDLEvent.key.keysym.scancode);
            Event *retval = new KeyDownEvent(key, translateModifiers((SDL_Keymod)SDLEvent.key.keysym.mod), keyState(key));
            keyState(key) = true;
            return retval;
        }
        case SDL_KEYUP:
        {
            KeyboardKey key = translateKey(SDLEvent.key.keysym.scancode);
            Event *retval = new KeyUpEvent(key, translateModifiers((SDL_Keymod)SDLEvent.key.keysym.mod));
            keyState(key) = false;
            return retval;
        }
        case SDL_MOUSEMOTION:
            return new MouseMoveEvent(SDLEvent.motion.x, SDLEvent.motion.y, SDLEvent.motion.xrel, SDLEvent.motion.yrel);
        case SDL_MOUSEBUTTONDOWN:
        {
            MouseButton button = translateButton(SDLEvent.button.button);
            buttonState = static_cast<MouseButton>(buttonState | button); // set bit
            return new MouseDownEvent(SDLEvent.button.x, SDLEvent.button.y, 0, 0, button);
        }
        case SDL_MOUSEBUTTONUP:
        {
            MouseButton button = translateButton(SDLEvent.button.button);
            buttonState = static_cast<MouseButton>(buttonState & ~button); // clear bit
            return new MouseUpEvent(SDLEvent.button.x, SDLEvent.button.y, 0, 0, button);
        }
        case SDL_JOYAXISMOTION:
        case SDL_JOYBALLMOTION:
        case SDL_JOYHATMOTION:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            //TODO (jacob#): handle joysticks
            break;
        case SDL_WINDOWEVENT_RESIZED:
            break;
        case SDL_QUIT:
            return new QuitEvent();
        case SDL_SYSWMEVENT:
            //TODO (jacob#): handle SDL_SYSWMEVENT
            break;
        }
    }
}

namespace
{
struct DefaultEventHandler : public EventHandler
{
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
        if(event.key == KeyboardKey_F4 && (event.mods & KeyboardModifiers_Alt) != 0)
        {
            needQuitEvent = true;
            return true;
        }
        return true;
    }
    virtual bool handleKeyPress(KeyPressEvent &) override
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
    for(Event *e = makeEvent(); e != nullptr; e = makeEvent())
    {
        if(eventHandler == nullptr || !e->dispatch(eventHandler))
        {
            e->dispatch(DefaultEventHandler_handler);
        }
    }
}

void glLoadMatrix(Matrix mat)
{
    float matArray[16] =
    {
        mat.x00,
        mat.x01,
        mat.x02,
        0,

        mat.x10,
        mat.x11,
        mat.x12,
        0,

        mat.x20,
        mat.x21,
        mat.x22,
        0,

        mat.x30,
        mat.x31,
        mat.x32,
        1,
    };
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
    ::handleEvents(eventHandler);
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

double Display::instantaneousFPS()
{
    return ::instantaneousFPS();
}

double Display::frameDeltaTime()
{
    return ::frameDeltaTime();
}

float Display::averageFPS()
{
    return ::averageFPS();
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
    //glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
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
    glFrustum(-minDistance * scaleX(), minDistance * scaleX(), -minDistance * scaleY(), minDistance * scaleY(), minDistance, maxDistance);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
}

void Display::initOverlay()
{
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
}

static bool grabMouse_ = false;

bool Display::grabMouse()
{
    return grabMouse_;
}

void Display::grabMouse(bool g)
{
    grabMouse_ = g;
    SDL_SetRelativeMouseMode(g ? SDL_TRUE : SDL_FALSE);
    SDL_SetWindowGrab(window, g ? SDL_TRUE : SDL_FALSE);
}

VectorF Display::transformMouseTo3D(float x, float y, float depth)
{
    return VectorF(depth * scaleX() * (2 * x / width() - 1), depth * scaleY() * (1 - 2 * y / height()), -depth);
}

namespace
{
void renderInternal(const Mesh & m)
{
    static vector<float> vertexArray, textureCoordArray, colorArray;
    vertexArray.resize(m.triangles.size() * 3 * 3);
    textureCoordArray.resize(m.triangles.size() * 3 * 2);
    colorArray.resize(m.triangles.size() * 3 * 4);
    for(size_t i = 0; i < m.triangles.size(); i++)
    {
        Triangle tri = m.triangles[i];
        vertexArray[i * 3 * 3 + 0 * 3 + 0] = tri.p1.x;
        vertexArray[i * 3 * 3 + 0 * 3 + 1] = tri.p1.y;
        vertexArray[i * 3 * 3 + 0 * 3 + 2] = tri.p1.z;
        vertexArray[i * 3 * 3 + 1 * 3 + 0] = tri.p2.x;
        vertexArray[i * 3 * 3 + 1 * 3 + 1] = tri.p2.y;
        vertexArray[i * 3 * 3 + 1 * 3 + 2] = tri.p2.z;
        vertexArray[i * 3 * 3 + 2 * 3 + 0] = tri.p3.x;
        vertexArray[i * 3 * 3 + 2 * 3 + 1] = tri.p3.y;
        vertexArray[i * 3 * 3 + 2 * 3 + 2] = tri.p3.z;
        textureCoordArray[i * 3 * 2 + 0 * 2 + 0] = tri.t1.u;
        textureCoordArray[i * 3 * 2 + 0 * 2 + 1] = tri.t1.v;
        textureCoordArray[i * 3 * 2 + 1 * 2 + 0] = tri.t2.u;
        textureCoordArray[i * 3 * 2 + 1 * 2 + 1] = tri.t2.v;
        textureCoordArray[i * 3 * 2 + 2 * 2 + 0] = tri.t3.u;
        textureCoordArray[i * 3 * 2 + 2 * 2 + 1] = tri.t3.v;
        colorArray[i * 3 * 4 + 0 * 4 + 0] = tri.c1.r;
        colorArray[i * 3 * 4 + 0 * 4 + 1] = tri.c1.g;
        colorArray[i * 3 * 4 + 0 * 4 + 2] = tri.c1.b;
        colorArray[i * 3 * 4 + 0 * 4 + 3] = tri.c1.a;
        colorArray[i * 3 * 4 + 1 * 4 + 0] = tri.c2.r;
        colorArray[i * 3 * 4 + 1 * 4 + 1] = tri.c2.g;
        colorArray[i * 3 * 4 + 1 * 4 + 2] = tri.c2.b;
        colorArray[i * 3 * 4 + 1 * 4 + 3] = tri.c2.a;
        colorArray[i * 3 * 4 + 2 * 4 + 0] = tri.c3.r;
        colorArray[i * 3 * 4 + 2 * 4 + 1] = tri.c3.g;
        colorArray[i * 3 * 4 + 2 * 4 + 2] = tri.c3.b;
        colorArray[i * 3 * 4 + 2 * 4 + 3] = tri.c3.a;
    }
    glVertexPointer(3, GL_FLOAT, 0, (const void *)&vertexArray[0]);
    glTexCoordPointer(2, GL_FLOAT, 0, (const void *)&textureCoordArray[0]);
    glColorPointer(4, GL_FLOAT, 0, (const void *)&colorArray[0]);
    glDrawArrays(GL_TRIANGLES, 0, (GLint)m.triangles.size() * 3);
}
}

void Display::render(const Mesh & m, bool enableDepthBuffer)
{
    m.image.bind();
    glDepthMask(enableDepthBuffer ? GL_TRUE : GL_FALSE);
    renderInternal(m);
}

void Display::clear(ColorF color)
{
    glDepthMask(GL_TRUE);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    initFrame();
}

namespace
{

struct CachedMeshData
{
    Image image;
    CachedMeshData(const Image & image)
        : image(image)
    {
    }
    virtual ~CachedMeshData()
    {
    }
    virtual void render(Matrix tform, bool enableDepthBuffer) = 0;
};

vector<GLuint> freeDisplayLists;
mutex freeDisplayListsLock;
GLuint allocateDisplayList()
{
    freeDisplayListsLock.lock();
    if(freeDisplayLists.empty())
    {
        freeDisplayListsLock.unlock();
        return glGenLists(1);
    }
    GLuint retval = freeDisplayLists.back();
    freeDisplayLists.pop_back();
    freeDisplayListsLock.unlock();
    return retval;
}
void freeDisplayList(GLuint displayList)
{
    lock_guard<mutex> lockGuard(freeDisplayListsLock);
    freeDisplayLists.push_back(displayList);
}
struct CachedMeshDataDisplayList : public CachedMeshData
{
    GLuint displayList;
    CachedMeshDataDisplayList(const Mesh & mesh)
        : CachedMeshData(mesh.image), displayList(allocateDisplayList())
    {
        glNewList(displayList, GL_COMPILE);
        renderInternal(mesh);
        glEndList();
    }
    virtual ~CachedMeshDataDisplayList()
    {
        freeDisplayList(displayList);
    }
    virtual void render(Matrix tform, bool enableDepthBuffer) override
    {
        image.bind();
        glDepthMask(enableDepthBuffer ? GL_TRUE : GL_FALSE);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrix(tform);
        glCallList(displayList);
        glLoadIdentity();
    }
};

PFNGLBINDBUFFERARBPROC fnGLBindBufferARB = nullptr;
PFNGLDELETEBUFFERSARBPROC fnGLDeleteBuffersARB = nullptr;
PFNGLGENBUFFERSARBPROC fnGLGenBuffersARB = nullptr;
PFNGLBUFFERDATAARBPROC fnGLBufferDataARB = nullptr;
PFNGLMAPBUFFERARBPROC fnGLMapBufferARB = nullptr;
PFNGLUNMAPBUFFERARBPROC fnGLUnmapBufferARB = nullptr;
bool haveOpenGLBuffers = false;

void getOpenGLBuffersExtension()
{
    haveOpenGLBuffers = SDL_GL_ExtensionSupported("GL_ARB_vertex_buffer_object");
    if(haveOpenGLBuffers)
    {
        fnGLBindBufferARB = (PFNGLBINDBUFFERARBPROC)SDL_GL_GetProcAddress("glBindBufferARB");
        fnGLDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)SDL_GL_GetProcAddress("glDeleteBuffersARB");
        fnGLGenBuffersARB = (PFNGLGENBUFFERSARBPROC)SDL_GL_GetProcAddress("glGenBuffersARB");
        fnGLBufferDataARB = (PFNGLBUFFERDATAARBPROC)SDL_GL_GetProcAddress("glBufferDataARB");
        fnGLMapBufferARB = (PFNGLMAPBUFFERARBPROC)SDL_GL_GetProcAddress("glMapBufferARB");
        fnGLUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)SDL_GL_GetProcAddress("glUnmapBufferARB");
    }
}

vector<GLuint> freeBuffers;
mutex freeBuffersLock;
GLuint allocateBuffer()
{
    assert(haveOpenGLBuffers);
    freeBuffersLock.lock();
    if(freeBuffers.empty())
    {
        freeBuffersLock.unlock();
        GLuint retval;
        fnGLGenBuffersARB(1, &retval);
        return retval;
    }
    GLuint retval = freeBuffers.back();
    freeBuffers.pop_back();
    freeBuffersLock.unlock();
    return retval;
}
void freeBuffer(GLuint displayList)
{
    lock_guard<mutex> lockGuard(freeBuffersLock);
    freeBuffers.push_back(displayList);
}
struct CachedMeshDataOpenGLBuffer : public CachedMeshData
{
    GLuint buffer;
    GLsizeiptrARB vertexSize, colorSize, textureCoordSize;
    GLsizei vertexCount;
    CachedMeshDataOpenGLBuffer(const Mesh & mesh)
        : CachedMeshData(mesh.image), buffer(allocateBuffer()), vertexCount(3 * mesh.triangles.size())
    {
        fnGLBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer);
        vertexSize = mesh.triangles.size() * 3 * 3 * sizeof(float);
        colorSize = mesh.triangles.size() * 4 * 3 * sizeof(float);
        textureCoordSize = mesh.triangles.size() * 2 * 3 * sizeof(float);
        fnGLBufferDataARB(GL_ARRAY_BUFFER_ARB, vertexSize + colorSize + textureCoordSize, nullptr, GL_STATIC_DRAW_ARB);
        void *bufferPtr = fnGLMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
        if(bufferPtr == nullptr)
        {
            cerr << "error : can't map opengl buffer\n" << flush;
            abort();
        }
        float *vertexArray = (float *)(char *)bufferPtr;
        float *colorArray = (float *)((char *)bufferPtr + vertexSize);
        float *textureCoordArray = (float *)((char *)bufferPtr + vertexSize + colorSize);
        for(size_t i = 0; i < mesh.triangles.size(); i++)
        {
            Triangle tri = mesh.triangles[i];
            vertexArray[i * 3 * 3 + 0 * 3 + 0] = tri.p1.x;
            vertexArray[i * 3 * 3 + 0 * 3 + 1] = tri.p1.y;
            vertexArray[i * 3 * 3 + 0 * 3 + 2] = tri.p1.z;
            vertexArray[i * 3 * 3 + 1 * 3 + 0] = tri.p2.x;
            vertexArray[i * 3 * 3 + 1 * 3 + 1] = tri.p2.y;
            vertexArray[i * 3 * 3 + 1 * 3 + 2] = tri.p2.z;
            vertexArray[i * 3 * 3 + 2 * 3 + 0] = tri.p3.x;
            vertexArray[i * 3 * 3 + 2 * 3 + 1] = tri.p3.y;
            vertexArray[i * 3 * 3 + 2 * 3 + 2] = tri.p3.z;
            textureCoordArray[i * 3 * 2 + 0 * 2 + 0] = tri.t1.u;
            textureCoordArray[i * 3 * 2 + 0 * 2 + 1] = tri.t1.v;
            textureCoordArray[i * 3 * 2 + 1 * 2 + 0] = tri.t2.u;
            textureCoordArray[i * 3 * 2 + 1 * 2 + 1] = tri.t2.v;
            textureCoordArray[i * 3 * 2 + 2 * 2 + 0] = tri.t3.u;
            textureCoordArray[i * 3 * 2 + 2 * 2 + 1] = tri.t3.v;
            colorArray[i * 3 * 4 + 0 * 4 + 0] = tri.c1.r;
            colorArray[i * 3 * 4 + 0 * 4 + 1] = tri.c1.g;
            colorArray[i * 3 * 4 + 0 * 4 + 2] = tri.c1.b;
            colorArray[i * 3 * 4 + 0 * 4 + 3] = tri.c1.a;
            colorArray[i * 3 * 4 + 1 * 4 + 0] = tri.c2.r;
            colorArray[i * 3 * 4 + 1 * 4 + 1] = tri.c2.g;
            colorArray[i * 3 * 4 + 1 * 4 + 2] = tri.c2.b;
            colorArray[i * 3 * 4 + 1 * 4 + 3] = tri.c2.a;
            colorArray[i * 3 * 4 + 2 * 4 + 0] = tri.c3.r;
            colorArray[i * 3 * 4 + 2 * 4 + 1] = tri.c3.g;
            colorArray[i * 3 * 4 + 2 * 4 + 2] = tri.c3.b;
            colorArray[i * 3 * 4 + 2 * 4 + 3] = tri.c3.a;
        }
        fnGLUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
        fnGLBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
    virtual ~CachedMeshDataOpenGLBuffer()
    {
        freeBuffer(buffer);
    }
    virtual void render(Matrix tform, bool enableDepthBuffer) override
    {
        image.bind();
        glDepthMask(enableDepthBuffer ? GL_TRUE : GL_FALSE);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrix(tform);
        fnGLBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer);
        glVertexPointer(3, GL_FLOAT, 0, (const void *)(GLintptrARB)0);
        glTexCoordPointer(2, GL_FLOAT, 0, (const void *)(GLintptrARB)(vertexSize + colorSize));
        glColorPointer(4, GL_FLOAT, 0, (const void *)(GLintptrARB)vertexSize);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glLoadIdentity();
        fnGLBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
};

shared_ptr<CachedMeshData> makeCachedMeshData(const Mesh &mesh)
{
    if(haveOpenGLBuffers)
        return make_shared<CachedMeshDataOpenGLBuffer>(mesh);
    return make_shared<CachedMeshDataDisplayList>(mesh);
}
}

struct CachedMesh
{
    Matrix tform;
    shared_ptr<CachedMeshData> data;
    CachedMesh(Matrix tform, shared_ptr<CachedMeshData> data)
        : tform(tform), data(data)
    {
    }
};

shared_ptr<CachedMesh> makeCachedMesh(const Mesh & mesh)
{
    if(mesh.triangles.empty())
        return make_shared<CachedMesh>(Matrix::identity(), nullptr);
    return make_shared<CachedMesh>(Matrix::identity(), makeCachedMeshData(mesh));
}

shared_ptr<CachedMesh> transform(const Matrix & m, shared_ptr<CachedMesh> mesh)
{
    return make_shared<CachedMesh>(transform(m, mesh->tform), mesh->data);
}

void Display::render(shared_ptr<CachedMesh> m, bool enableDepthBuffer)
{
    if(m->data)
        m->data->render(m->tform, enableDepthBuffer);
}

static void getExtensions()
{
    getOpenGLBuffersExtension();
}
