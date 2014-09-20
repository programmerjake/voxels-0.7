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
#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include "stream/stream.h"
#include <memory>

namespace stream
{

class NetworkException : public IOException
{
public:
    explicit NetworkException(string msg)
        : IOException(msg)
    {
    }
};

class NetworkConnection final : public StreamRW
{
    friend class NetworkServer;
private:
    shared_ptr<Reader> readerInternal;
    shared_ptr<Writer> writerInternal;
    NetworkConnection(int readFd, int writeFd)
        : readerInternal(new FileReader(fdopen(readFd, "r"))), writerInternal(new FileWriter(fdopen(writeFd, "w")))
    {
    }
public:
    explicit NetworkConnection(wstring url, uint16_t port);
    shared_ptr<Reader> preader() override
    {
        return readerInternal;
    }
    shared_ptr<Writer> pwriter() override
    {
        return writerInternal;
    }
};

class NetworkServer final : public StreamServer
{
    NetworkServer(const NetworkServer &) = delete;
    const NetworkServer & operator =(const NetworkServer &) = delete;
private:
    int fd;
public:
    explicit NetworkServer(uint16_t port);
    ~NetworkServer();
    shared_ptr<StreamRW> accept() override;
};

}

#endif // NETWORK_H_INCLUDED
