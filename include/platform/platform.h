/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
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
#include "platform/event.h"
#ifndef PLATFORM_H_INCLUDED
#define PLATFORM_H_INCLUDED

#include <cwchar>
#include <string>
#include <memory>
#include <vector>
#include "util/matrix.h"
#include "util/vector.h"
#include "stream/stream.h"
#include "render/mesh.h"
#include "util/enum_traits.h"
#include <type_traits>
#include <tuple>

namespace programmerjake
{
namespace voxels
{
#ifndef EVENT_H_INCLUDED
struct EventHandler;
#endif // EVENT_H_INCLUDED

const float defaultFPS = 60;

std::shared_ptr<stream::Reader> getResourceReader(std::wstring resource);

/** returns reader/writer pair
 @note Writer::flush should be called after writing before attempting to read
 */
std::pair<std::shared_ptr<stream::Reader>, std::shared_ptr<stream::Writer>> createTemporaryFile();

enum class KeyboardKey : std::uint8_t
{
    Unknown,
    A, // alphabet must be in order
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Backspace,
    Tab,
    Clear,
    Return,
    Pause,
    Escape,
    Space,
    SQuote,
    Equals,
    Comma,
    Dash,
    BQuote,
    LBracket,
    RBracket,
    BSlash,
    Delete,
    Period,
    FSlash,
    Num0, // numbers must be in order
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Colon,
    F1, // F keys must be in order
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Up,
    Down,
    Left,
    Right,
    Insert,
    Home,
    End,
    PageUp,
    PageDown,
    LShift,
    RShift,
    LCtrl,
    RCtrl,
    KPad0, // numbers must be in order
    KPad1,
    KPad2,
    KPad3,
    KPad4,
    KPad5,
    KPad6,
    KPad7,
    KPad8,
    KPad9,
    KPadFSlash,
    KPadStar,
    KPadDash,
    KPadPlus,
    KPadReturn,
    KPadPeriod,
    KPadEquals,
    NumLock,
    CapsLock,
    ScrollLock,
    LAlt,
    RAlt,
    LMeta,
    RMeta,
    LSuper,
    RSuper,
    AltGr,
    Menu,
    Mode,
    PrintScreen,

    DEFINE_ENUM_LIMITS(Unknown, PrintScreen)

    DQuote = SQuote,
    Underline = Dash,
    Tilde = BQuote,
    LBrace = LBracket,
    RBrace = RBracket,
    Pipe = BSlash,
    EMark = Num1,
    Pound = Num3,
    Dollar = Num4,
    Percent = Num5,
    Caret = Num6,
    Amp = Num7,
    Star = Num8,
    LParen = Num9,
    RParen = Num0,
    Semicolon = Colon,
    LAngle = Comma,
    Plus = Equals,
    RAngle = Period,
    QMark = FSlash,
    AtSign = Num2,
    KPadInsert = KPad0,
    KPadEnd = KPad1,
    KPadDown = KPad2,
    KPadPageDown = KPad3,
    KPadLeft = KPad4,
    KPadRight = KPad6,
    KPadHome = KPad7,
    KPadUp = KPad8,
    KPadPageUp = KPad9,
    KPadDelete = KPadPeriod,
    SysRequest = PrintScreen,
    Break = Pause,
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
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::KeyboardKey>
{
private:
    typedef std::underlying_type<programmerjake::voxels::KeyboardKey>::type int_type;
    hash<int_type> hasher;
public:
    size_t operator ()(programmerjake::voxels::KeyboardKey v) const
    {
        return hasher((int_type)v);
    }
};

template <>
struct hash<programmerjake::voxels::KeyboardModifiers>
{
private:
    typedef std::underlying_type<programmerjake::voxels::KeyboardModifiers>::type int_type;
    hash<int_type> hasher;
public:
    size_t operator ()(programmerjake::voxels::KeyboardModifiers v) const
    {
        return hasher((int_type)v);
    }
};

template <>
struct hash<programmerjake::voxels::MouseButton>
{
private:
    typedef std::underlying_type<programmerjake::voxels::MouseButton>::type int_type;
    hash<int_type> hasher;
public:
    size_t operator ()(programmerjake::voxels::MouseButton v) const
    {
        return hasher((int_type)v);
    }
};
}

namespace programmerjake
{
namespace voxels
{
struct MeshBufferImp;
class MeshBuffer;

namespace Display
{
    void render(const MeshBuffer &m, RenderLayer rl);
}

class MeshBuffer final
{
private:
    std::shared_ptr<MeshBufferImp> imp;
    Matrix tform;
    static bool impIsEmpty(std::shared_ptr<MeshBufferImp> mesh);
    static std::size_t impCapacity(std::shared_ptr<MeshBufferImp> mesh);
    MeshBuffer(std::shared_ptr<MeshBufferImp> imp, Matrix tform)
        : imp(std::move(imp)), tform(tform)
    {
    }
public:
    MeshBuffer()
        : imp(), tform(Matrix::identity())
    {
    }
    MeshBuffer(std::size_t triangleCount);
    bool set(const Mesh &mesh, bool isFinal);
    std::size_t capacity() const
    {
        if(imp == nullptr)
            return 0;
        return impCapacity(imp);
    }
    bool empty() const
    {
        if(imp == nullptr)
            return true;
        return impIsEmpty(imp);
    }
    MeshBuffer createTransformed(Matrix m) const
    {
        return MeshBuffer(imp, transform(m, tform));
    }
    friend void Display::render(const MeshBuffer &m, RenderLayer rl);
};

inline MeshBuffer transform(const Matrix &m, const MeshBuffer &meshBuffer)
{
    return meshBuffer.createTransformed(m);
}

namespace Display
{
    std::wstring title();
    void title(std::wstring newTitle);
    void handleEvents(std::shared_ptr<EventHandler> eventHandler);
    void flip(float fps);
    void flip();
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
    VectorF transformTouchTo3D(float x, float y, float depth = 1.0f);
    VectorF transform3DToMouse(VectorF pos);
    VectorF transform3DToTouch(VectorF pos);
    void render(const Mesh & m, Matrix tform, RenderLayer rl);
    void clear(ColorF color = RGBAF(0, 0, 0, 0));
    float screenRefreshRate();
    bool fullScreen();
    void fullScreen(bool fs);
    bool needTouchControls();
}

void startGraphics();
void endGraphics();
void startAudio();
void endAudio();
unsigned getGlobalAudioSampleRate();
unsigned getGlobalAudioChannelCount();
bool audioRunning();
int main(std::vector<std::wstring> args); // called by the platform's main function
std::uint32_t allocateTexture();
void freeTexture(std::uint32_t texture);

}
}

#endif // PLATFORM_H_INCLUDED
