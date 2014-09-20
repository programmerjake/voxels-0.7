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
#ifndef PLATFORM_H_INCLUDED
#define PLATFORM_H_INCLUDED

#include <string>
#include <memory>
#include "util/matrix.h"
#include "util/vector.h"
#include "stream/stream.h"
#include "render/mesh.h"

#ifndef EVENT_H_INCLUDED
class EventHandler;
#endif // EVENT_H_INCLUDED

const float defaultFPS = 60;

shared_ptr<stream::Reader> getResourceReader(wstring resource);

enum KeyboardKey
{
    KeyboardKey_Unknown,
    KeyboardKey_A,
    KeyboardKey_B,
    KeyboardKey_C,
    KeyboardKey_D,
    KeyboardKey_E,
    KeyboardKey_F,
    KeyboardKey_G,
    KeyboardKey_H,
    KeyboardKey_I,
    KeyboardKey_J,
    KeyboardKey_K,
    KeyboardKey_L,
    KeyboardKey_M,
    KeyboardKey_N,
    KeyboardKey_O,
    KeyboardKey_P,
    KeyboardKey_Q,
    KeyboardKey_R,
    KeyboardKey_S,
    KeyboardKey_T,
    KeyboardKey_U,
    KeyboardKey_V,
    KeyboardKey_W,
    KeyboardKey_X,
    KeyboardKey_Y,
    KeyboardKey_Z,

    KeyboardKey_Backspace,
    KeyboardKey_Tab,
    KeyboardKey_Clear,
    KeyboardKey_Return,
    KeyboardKey_Pause,
    KeyboardKey_Escape,
    KeyboardKey_Space,
    KeyboardKey_SQuote,
    KeyboardKey_Equals,
    KeyboardKey_Comma,
    KeyboardKey_Dash,
    KeyboardKey_BQuote,
    KeyboardKey_LBracket,
    KeyboardKey_RBracket,
    KeyboardKey_BSlash,
    KeyboardKey_Delete,
    KeyboardKey_Period,
    KeyboardKey_FSlash,
    KeyboardKey_Num0,
    KeyboardKey_Num1,
    KeyboardKey_Num2,
    KeyboardKey_Num3,
    KeyboardKey_Num4,
    KeyboardKey_Num5,
    KeyboardKey_Num6,
    KeyboardKey_Num7,
    KeyboardKey_Num8,
    KeyboardKey_Num9,
    KeyboardKey_Colon,
    KeyboardKey_F1,
    KeyboardKey_F2,
    KeyboardKey_F3,
    KeyboardKey_F4,
    KeyboardKey_F5,
    KeyboardKey_F6,
    KeyboardKey_F7,
    KeyboardKey_F8,
    KeyboardKey_F9,
    KeyboardKey_F10,
    KeyboardKey_F11,
    KeyboardKey_F12,
    KeyboardKey_Up,
    KeyboardKey_Down,
    KeyboardKey_Left,
    KeyboardKey_Right,
    KeyboardKey_Insert,
    KeyboardKey_Home,
    KeyboardKey_End,
    KeyboardKey_PageUp,
    KeyboardKey_PageDown,
    KeyboardKey_LShift,
    KeyboardKey_RShift,
    KeyboardKey_LCtrl,
    KeyboardKey_RCtrl,
    KeyboardKey_KPad0,
    KeyboardKey_KPad1,
    KeyboardKey_KPad2,
    KeyboardKey_KPad3,
    KeyboardKey_KPad4,
    KeyboardKey_KPad5,
    KeyboardKey_KPad6,
    KeyboardKey_KPad7,
    KeyboardKey_KPad8,
    KeyboardKey_KPad9,
    KeyboardKey_KPadFSlash,
    KeyboardKey_KPadStar,
    KeyboardKey_KPadDash,
    KeyboardKey_KPadPlus,
    KeyboardKey_KPadReturn,
    KeyboardKey_KPadPeriod,
    KeyboardKey_KPadEquals,
    KeyboardKey_NumLock,
    KeyboardKey_CapsLock,
    KeyboardKey_ScrollLock,
    KeyboardKey_LAlt,
    KeyboardKey_RAlt,
    KeyboardKey_LMeta,
    KeyboardKey_RMeta,
    KeyboardKey_LSuper,
    KeyboardKey_RSuper,
    KeyboardKey_AltGr,
    KeyboardKey_Menu,
    KeyboardKey_Mode,
    KeyboardKey_PrintScreen,

    KeyboardKey_max = KeyboardKey_PrintScreen,
    KeyboardKey_min = KeyboardKey_Unknown,

    KeyboardKey_DQuote = KeyboardKey_SQuote,
    KeyboardKey_Underline = KeyboardKey_Dash,
    KeyboardKey_Tilde = KeyboardKey_BQuote,
    KeyboardKey_LBrace = KeyboardKey_LBracket,
    KeyboardKey_RBrace = KeyboardKey_RBracket,
    KeyboardKey_Pipe = KeyboardKey_BSlash,
    KeyboardKey_EMark = KeyboardKey_Num1,
    KeyboardKey_Pound = KeyboardKey_Num3,
    KeyboardKey_Dollar = KeyboardKey_Num4,
    KeyboardKey_Percent = KeyboardKey_Num5,
    KeyboardKey_Caret = KeyboardKey_Num6,
    KeyboardKey_Amp = KeyboardKey_Num7,
    KeyboardKey_Star = KeyboardKey_Num8,
    KeyboardKey_LParen = KeyboardKey_Num9,
    KeyboardKey_RParen = KeyboardKey_Num0,
    KeyboardKey_Semicolon = KeyboardKey_Colon,
    KeyboardKey_LAngle = KeyboardKey_Comma,
    KeyboardKey_Plus = KeyboardKey_Equals,
    KeyboardKey_RAngle = KeyboardKey_Period,
    KeyboardKey_QMark = KeyboardKey_FSlash,
    KeyboardKey_AtSign = KeyboardKey_Num2,
    KeyboardKey_KPadInsert = KeyboardKey_KPad0,
    KeyboardKey_KPadEnd = KeyboardKey_KPad1,
    KeyboardKey_KPadDown = KeyboardKey_KPad2,
    KeyboardKey_KPadPageDown = KeyboardKey_KPad3,
    KeyboardKey_KPadLeft = KeyboardKey_KPad4,
    KeyboardKey_KPadRight = KeyboardKey_KPad6,
    KeyboardKey_KPadHome = KeyboardKey_KPad7,
    KeyboardKey_KPadUp = KeyboardKey_KPad8,
    KeyboardKey_KPadPageUp = KeyboardKey_KPad9,
    KeyboardKey_KPadDelete = KeyboardKey_KPadPeriod,
    KeyboardKey_SysRequest = KeyboardKey_PrintScreen,
    KeyboardKey_Break = KeyboardKey_Pause,
};

enum KeyboardModifiers
{
    KeyboardModifiers_None = 0,
    KeyboardModifiers_LShift = 0x1,
    KeyboardModifiers_RShift = 0x2,
    KeyboardModifiers_LCtrl = 0x4,
    KeyboardModifiers_RCtrl = 0x8,
    KeyboardModifiers_LAlt = 0x10,
    KeyboardModifiers_RAlt = 0x20,
    KeyboardModifiers_LMeta = 0x40,
    KeyboardModifiers_RMeta = 0x80,
    KeyboardModifiers_NumLock = 0x100,
    KeyboardModifiers_CapsLock = 0x200,
    KeyboardModifiers_Mode = 0x400,
    KeyboardModifiers_Ctrl = KeyboardModifiers_LCtrl | KeyboardModifiers_RCtrl,
    KeyboardModifiers_Shift = KeyboardModifiers_LShift | KeyboardModifiers_RShift,
    KeyboardModifiers_Alt = KeyboardModifiers_LAlt | KeyboardModifiers_RAlt,
    KeyboardModifiers_Meta = KeyboardModifiers_LMeta | KeyboardModifiers_RMeta,
};

enum MouseButton
{
    MouseButton_None = 0,
    MouseButton_Left = 0x1,
    MouseButton_Right = 0x2,
    MouseButton_Middle = 0x4,
    MouseButton_X1 = 0x8,
    MouseButton_X2 = 0x10
};

struct CachedMesh;

shared_ptr<CachedMesh> makeCachedMesh(const Mesh & mesh);
shared_ptr<CachedMesh> transform(const Matrix & m, shared_ptr<CachedMesh> mesh);

namespace Display
{
    wstring title();
    void title(wstring newTitle);
    void handleEvents(shared_ptr<EventHandler> eventHandler);
    void flip(float fps = defaultFPS);
    double instantaneousFPS();
    double frameDeltaTime();
    float averageFPS();
    double timer();
    double realtimeTimer();
    int width();
    int height();
    float scaleX();
    float scaleY();
    void initFrame();
    void initOverlay();
    bool grabMouse();
    void grabMouse(bool g);
    VectorF transformMouseTo3D(float x, float y, float depth = 1.0f);
    void render(const Mesh & m, bool enableDepthBuffer);
    void render(shared_ptr<CachedMesh> m, bool enableDepthBuffer);
    void clear(ColorF color = RGBAF(0, 0, 0, 0));
}

void startGraphics();
void endGraphics();
void startAudio();
void endAudio();
unsigned getGlobalAudioSampleRate();
unsigned getGlobalAudioChannelCount();
bool audioRunning();

#endif // PLATFORM_H_INCLUDED
#include "platform/event.h"
