/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include "stream/stream.h"
#include <memory>
#include <string>

namespace programmerjake
{
namespace voxels
{
namespace stream
{

class NetworkException : public IOException
{
public:
    explicit NetworkException(std::string msg)
        : IOException(msg)
    {
    }
};

class NetworkConnection final : public StreamRW
{
    friend class NetworkServer;
private:
    std::shared_ptr<Reader> readerInternal;
    std::shared_ptr<Writer> writerInternal;
    NetworkConnection(int readFd, int writeFd);
public:
    explicit NetworkConnection(std::wstring url, uint16_t port);
    std::shared_ptr<Reader> preader() override
    {
        return readerInternal;
    }
    std::shared_ptr<Writer> pwriter() override
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
    std::shared_ptr<StreamRW> accept() override;
};

}
}
}

#endif // NETWORK_H_INCLUDED
