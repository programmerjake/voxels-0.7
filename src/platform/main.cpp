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
#include <memory>
#include <sstream>
#include <cwchar>
#include <string>

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
    bool done = false;
    shared_ptr<PlayingAudio> playingAudio;
    int audioIndex = 1;
    float volume = 0.5f;
    double getCurrentPlayTime()
    {
        if(playingAudio)
            return playingAudio->PlayingAudio::currentTime();
        return 0;
    }
    bool handleKeyDown(KeyDownEvent &event) override
    {
        if(event.key == KeyboardKey_Up)
        {
            if(playingAudio)
                playingAudio->volume(volume = limit<float>(volume * 1.1f, 0, 1));
        }
        else if(event.key == KeyboardKey_Down)
        {
            if(playingAudio)
                playingAudio->volume(volume = limit<float>(volume / 1.1f, 0, 1));
        }
        else if(event.key == KeyboardKey_Escape)
            done = true;
        else if(event.key == KeyboardKey_Left)
        {
            audioIndex += maxBackgroundFileNumber - 2;
            audioIndex %= maxBackgroundFileNumber;
            audioIndex++;
            auto newPlayingAudio = loadAudio(audioIndex).play(volume, true);
            if(playingAudio)
                playingAudio->stop();
            playingAudio = newPlayingAudio;
        }
        else if(event.key == KeyboardKey_Right)
        {
            audioIndex %= maxBackgroundFileNumber;
            audioIndex++;
            auto newPlayingAudio = loadAudio(audioIndex).play(volume, true);
            if(playingAudio)
                playingAudio->stop();
            playingAudio = newPlayingAudio;
        }
        else
            return false;
        return true;
    }
    bool handleKeyPress(KeyPressEvent &event) override
    {
        return false;
    }
    bool handleKeyUp(KeyUpEvent &event) override
    {
        return false;
    }
    bool handleMouseDown(MouseDownEvent &event) override
    {
        return false;
    }
    bool handleMouseMove(MouseMoveEvent &event) override
    {
        return false;
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
};

int main()
{
    startGraphics();
    shared_ptr<MyEventHandler> eh = make_shared<MyEventHandler>();
    eh->playingAudio = loadAudio(eh->audioIndex).play(eh->volume, true);
    Renderer r;
    while(!eh->done)
    {
        Display::initFrame();
        Display::clear();
        wostringstream ss;
        ss << L"Press ESC to exit.\nPlaying file #" << eh->audioIndex << "\nVolume: " << 100 * 1e-3 * std::floor(1e3 * eh->volume + 0.5) << "%\nTime: " << 1e-2 * std::floor(eh->getCurrentPlayTime() * 1e2 + 0.5);
        wstring msg = ss.str();
        float msgHeight = Text::height(msg);
        float msgWidth = Text::width(msg);
        float scale = min<float>(Display::scaleX() * 2 / msgWidth, Display::scaleY() * 2 / msgHeight);
        r << transform(Matrix::scale(scale).concat(Matrix::translate(-msgWidth / 2 * scale, -msgHeight / 2 * scale, -1)), Text::mesh(ss.str(), RGBF(0.75, 0.75, 0.75)));
        Display::flip(60);
        Display::handleEvents(eh);
    }
    return 0;
}
#endif