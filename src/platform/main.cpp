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
#if 0
#include "networking/client.h"
#include "networking/server.h"
#include "stream/stream.h"
#include "stream/network.h"
#include "util/util.h"
#include "util/game_version.h"
#include "util/string_cast.h"
#include <thread>
#include <vector>
#include <iostream>

using namespace std;

namespace
{
void serverThreadFn(shared_ptr<stream::StreamServer> server)
{
    runServer(server);
}

bool isQuiet = false;

void outputVersion()
{
    if(isQuiet)
        return;
    static bool didOutputVersion = false;
    if(didOutputVersion)
        return;
    didOutputVersion = true;
    cout << "voxels " << string_cast<string>(GameVersion::VERSION) << "\n";
}

void help()
{
    isQuiet = false;
    outputVersion();
    cout << "usage : voxels [-h | --help] [-q | --quiet] [--server] [--client <server url>]\n";
}

int error(wstring msg)
{
    outputVersion();
    cout << "error : " << string_cast<string>(msg) << "\n";
    if(!isQuiet)
        help();
    return 1;
}
}

int myMain(vector<wstring> args)
{
    args.erase(args.begin());
    while(!args.empty() && args.front() == L"")
        args.erase(args.begin());
    bool isServer = false, isClient = false;
    wstring clientAddr;
    for(auto i = args.begin(); i != args.end(); i++)
    {
        wstring arg = *i;
        if(arg == L"--help" || arg == L"-h")
        {
            help();
            return 0;
        }
        else if(arg == L"-q" || arg == L"--quiet")
        {
            isQuiet = true;
        }
        else if(arg == L"--server")
        {
            if(isServer)
                return error(L"can't specify two server flags");
            if(isClient)
                return error(L"can't specify both server and client");
            isServer = true;
        }
        else if(arg == L"--client")
        {
            if(isServer)
                return error(L"can't specify both server and client");
            if(isClient)
                return error(L"can't specify two client flags");
            isClient = true;
            i++;
            if(i == args.end())
                return error(L"--client missing server url");
            arg = *i;
            clientAddr = arg;
        }
        else
            return error(L"unrecognized argument : " + arg);
    }
    thread serverThread;
    try
    {
        if(isServer)
        {
            cout << "Voxels " << string_cast<string>(GameVersion::VERSION) << " (c) 2014 Jacob R. Lifshay" << endl;
            shared_ptr<stream::NetworkServer> server = make_shared<stream::NetworkServer>(GameVersion::port);
            cout << "Connected to port " << GameVersion::port << endl;
            runServer(server);
            return 0;
        }
        if(isClient)
        {
            shared_ptr<stream::NetworkConnection> connection = make_shared<stream::NetworkConnection>(clientAddr, GameVersion::port);
            runClient(connection);
            return 0;
        }
        stream::StreamBidirectionalPipe pipe;
        shared_ptr<stream::NetworkServer> server = nullptr;
        try
        {
            server = make_shared<stream::NetworkServer>(GameVersion::port);
            cout << "Connected to port " << GameVersion::port << endl;
        }
        catch(stream::IOException & e)
        {
            cout << e.what() << endl;
        }
        serverThread = thread(serverThreadFn, shared_ptr<stream::StreamServer>(new stream::StreamServerWrapper(list<shared_ptr<stream::StreamRW>>{pipe.pport1()}, server)));
        runClient(pipe.pport2());
    }
    catch(exception & e)
    {
        return error(string_cast<wstring>(e.what()));
    }
    serverThread.join();
    return 0;
}

int main(int argc, char ** argv)
{
    vector<wstring> args;
    args.resize(argc);
    for(int i = 0; i < argc; i++)
    {
        args[i] = string_cast<wstring>(argv[i]);
    }
    return myMain(args);
}
#else
#include "platform/audio.h"
#include "platform/platform.h"
#include "platform/event.h"
#include "render/text.h"
#include "render/renderer.h"
#include "util/util.h"
#include "world/world.h"
#include "player/player.h"
#include <memory>
#include <sstream>
#include <cwchar>
#include <string>
#include <thread>
#include <atomic>
#include "util/flag.h"
#include "block/builtin/air.h"
#include "block/builtin/stone.h"

using namespace std;

constexpr int maxBackgroundFileNumber = 17;

Audio loadAudio(int fileNumber)
{
    assert(fileNumber >= 1 && fileNumber <= 17);
    wostringstream ss;
    ss << L"background";
    if(fileNumber > 1)
        ss << fileNumber;
    ss << L".ogg";
    return Audio(ss.str(), true);
}

struct MyEventHandler : public EventHandler
{
    shared_ptr<PlayingAudio> playingAudio;
    int audioIndex = 3;
    float volume = 0.1f;
    float theta = 0, phi = 0, deltaTheta = 0, deltaPhi = 0;
    PositionF position = PositionF(0.5f, 0.5f, 0.5f, Dimension::Overworld);
    World world;
    int32_t viewDistance = 48;
    enum_array<bool, KeyboardKey> currentKeyState;
    bool paused = false;
    Matrix getForwardMatrix()
    {
        return Matrix::rotateY(-theta);
    }
    Matrix getOrientMatrix()
    {
        return Matrix::rotateX(phi).concat(getForwardMatrix());
    }
    Matrix getPlayerMatrix()
    {
        return getOrientMatrix().concat(Matrix::translate((VectorF)position));
    }
    void drawWorld(Renderer &renderer)
    {
        ::drawWorld(world, renderer, getPlayerMatrix(), position.d, viewDistance);
    }
    double getCurrentPlayTime()
    {
        if(playingAudio)
            return playingAudio->PlayingAudio::currentTime();
        return 0;
    }
    void startAudio()
    {
        auto newPlayingAudio = loadAudio(audioIndex).play(volume, false);
        if(playingAudio)
            playingAudio->stop();
        playingAudio = newPlayingAudio;
    }
    void idleLoop()
    {
        deltaTheta *= 0.5f;
        deltaPhi *= 0.5f;
        theta = std::fmod(theta + deltaTheta, (float)(M_PI * 2));
        phi = limit<float>(phi + deltaPhi, -M_PI / 2, M_PI / 2);
        if(playingAudio)
            if(!playingAudio->isPlaying())
                playingAudio = nullptr;
        if(!playingAudio)
        {
            audioIndex %= maxBackgroundFileNumber;
            audioIndex++;
            startAudio();
        }
        PositionI pos = PositionI(rand() % 11 - 5, rand() % 11 - 5, rand() % 11 - 5, Dimension::Overworld);
        Block block = (rand() % 2 == 0 ? Block(Blocks::builtin::Air::descriptor()) : Block(Blocks::builtin::Stone::descriptor()));
        if(pos != (PositionI)position)
            world.setBlock(pos, block);
        checkGenerate = world.needGenerateChunks();
        world.mergeGeneratedChunk();
        VectorF leftVector = getForwardMatrix().apply(VectorF(-1, 0, 0));
        VectorF forwardVector = getForwardMatrix().apply(VectorF(0, 0, -1));
        VectorF upVector = VectorF(0, 1, 0);
        VectorF moveDirection = VectorF(0);
        if(currentKeyState[KeyboardKey::LShift] || currentKeyState[KeyboardKey::RShift])
            moveDirection -= upVector;
        if(currentKeyState[KeyboardKey::Space])
            moveDirection += upVector;
        if(currentKeyState[KeyboardKey::A])
            moveDirection += leftVector;
        if(currentKeyState[KeyboardKey::D])
            moveDirection -= leftVector;
        if(currentKeyState[KeyboardKey::W])
            moveDirection += forwardVector;
        if(currentKeyState[KeyboardKey::S])
            moveDirection -= forwardVector;
        if(currentKeyState[KeyboardKey::F])
            moveDirection *= 2.5;
        moveDirection *= 2;
        if(!paused)
            position += Display::frameDeltaTime() * moveDirection;
        world.move(Display::frameDeltaTime());
    }
    bool handleKeyDown(KeyDownEvent &event) override
    {
        currentKeyState[event.key] = true;
        if(event.key == KeyboardKey::Up)
        {
            volume = limit<float>(volume * 1.1f, 0, 1);
            if(playingAudio)
                playingAudio->volume(volume);
        }
        else if(event.key == KeyboardKey::Down)
        {
            volume = limit<float>(volume / 1.1f, 0, 1);
            if(playingAudio)
                playingAudio->volume(volume);
        }
        else if(event.key == KeyboardKey::Escape)
            done = true;
        else if(event.key == KeyboardKey::Left)
        {
            audioIndex += maxBackgroundFileNumber - 2;
            audioIndex %= maxBackgroundFileNumber;
            audioIndex++;
            startAudio();
        }
        else if(event.key == KeyboardKey::Right)
        {
            audioIndex %= maxBackgroundFileNumber;
            audioIndex++;
            startAudio();
        }
        else if(event.key == KeyboardKey::P)
        {
            if(!event.isRepetition)
            {
                paused = !paused;
                setMouseGrab();
            }
        }
        else
            return false;
        return true;
    }
    void setMouseGrab()
    {
        Display::grabMouse(!paused);
    }
    bool handleKeyPress(KeyPressEvent &event) override
    {
        return false;
    }
    bool handleKeyUp(KeyUpEvent &event) override
    {
        currentKeyState[event.key] = false;
        return false;
    }
    bool handleMouseDown(MouseDownEvent &event) override
    {
        return false;
    }
    bool handleMouseMove(MouseMoveEvent &event) override
    {
        if(!paused)
        {
            deltaTheta += event.deltaX / 300;
            deltaPhi -= event.deltaY / 300;
        }
        return true;
    }
    bool handleMouseScroll(MouseScrollEvent &event) override
    {
        return false;
    }
    bool handleMouseUp(MouseUpEvent &event) override
    {
        return false;
    }
    bool handleQuit(QuitEvent &event) override
    {
        bool retval = !done;
        done = true;
        return retval;
    }
    void generateChunk(PositionI chunkBasePosition)
    {
        for(int32_t rx = 0; rx < World::ChunkType::chunkSizeX; rx++)
        {
            for(int32_t ry = 0; ry < World::ChunkType::chunkSizeY; ry++)
            {
                for(int32_t rz = 0; rz < World::ChunkType::chunkSizeZ; rz++)
                {
                    PositionI pos = chunkBasePosition + VectorI(rx, ry, rz);
                    Block block = Block(Blocks::builtin::Air::descriptor());
                    float x = pos.x / 3.0, z = pos.z / 3.0;
                    if(pos.y < 5 * sin(x * z))
                        block = Block(Blocks::builtin::Stone::descriptor());
                    world.setBlock(pos, block);
                }
            }
        }
    }
    atomic_bool done;
    thread meshGeneratorThread;
    flag checkGenerate;
    vector<thread> chunkGeneratorThreads;
    MyEventHandler()
        : done(false), checkGenerate(true)
    {
        for(bool &v : currentKeyState)
            v = false;
        PositionI minPosition = World::ChunkType::getChunkBasePosition(PositionI(-16, -16, -16, Dimension::Overworld));
        PositionI maxPosition = World::ChunkType::getChunkBasePosition(PositionI(16, 16, 16, Dimension::Overworld));
        PositionI cpos = minPosition;
        for(cpos.x = minPosition.x; cpos.x <= maxPosition.x; cpos.x += World::ChunkType::chunkSizeX)
        {
            for(cpos.y = minPosition.y; cpos.y <= maxPosition.y; cpos.y += World::ChunkType::chunkSizeY)
            {
                for(cpos.z = minPosition.z; cpos.z <= maxPosition.z; cpos.z += World::ChunkType::chunkSizeZ)
                {
                    world.scheduleGenerateChunk(cpos);
                }
            }
        }
        chunkGeneratorThreads.resize(5);
        for(thread &t : chunkGeneratorThreads)
        {
            t = thread([this]()
            {
                while(!done)
                {
                    checkGenerate.wait();
                    if(done)
                        return;
                    if(!world.generateChunk())
                        checkGenerate = false;
                }
            });
        }
        while(world.generateChunk())
        {
            world.mergeGeneratedChunks();
        }
        world.updateMeshes((PositionI)position, viewDistance);
        meshGeneratorThread = thread([&]()
        {
            while(!done)
                world.updateMeshes((PositionI)position, viewDistance);
        });
    }
    ~MyEventHandler()
    {
        done = true;
        checkGenerate = true;
        meshGeneratorThread.join();
        for(thread &t : chunkGeneratorThreads)
            t.join();
    }
};

int main()
{
    shared_ptr<MyEventHandler> eh = make_shared<MyEventHandler>();
    startGraphics();
    eh->startAudio();
    Renderer r;
    eh->setMouseGrab();
    while(!eh->done)
    {
        Display::initFrame();
        Display::clear();
        eh->drawWorld(r);
        Display::initOverlay();
        wostringstream ss;
        ss << L"Press ESC to exit.\nPlaying file #" << eh->audioIndex << "\nVolume: " << 100 * 1e-3 * std::floor(1e3 * eh->volume + 0.5) << "%\nTime: " << 1e-2 * std::floor(eh->getCurrentPlayTime() * 1e2 + 0.5);
        wstring msg = ss.str();
        float msgHeight = Text::height(msg);
        float msgWidth = Text::width(msg);
        float scale = min<float>(Display::scaleX() * 2 / msgWidth, Display::scaleY() * 2 / msgHeight);
        r << transform(Matrix::scale(scale).concat(Matrix::translate(-msgWidth / 2 * scale, -msgHeight / 2 * scale, -1)), Text::mesh(ss.str(), RGBF(0.75, 0.75, 1)));
        Display::flip(60);
        Display::handleEvents(eh);
        eh->idleLoop();
    }
    return 0;
}
#endif