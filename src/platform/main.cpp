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
