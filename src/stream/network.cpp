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
#include "stream/network.h"
#include "util/util.h"
#if _WIN64 || _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cassert>
#include <sstream>

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace stream
{

namespace
{
struct NetworkInit final
{
    NetworkInit(const NetworkInit &) = delete;
    NetworkInit &operator =(const NetworkInit &) = delete;
    NetworkInit()
    {
        WSADATA data;
        int wsaStartupRetVal = WSAStartup(MAKEDWORD(2, 2), &data);
        if(wsaStartupRetVal != 0)
        {
            std::ostringstream ss;
            ss << "WSAStartup failed: " << wsaStartupRetVal;
            throw NetworkException(ss.str());
        }
    }
    ~NetworkInit()
    {
        WSACleanup();
    }
};

void initNetwork()
{
    static std::shared_ptr<NetworkInit> retval = std::make_shared<NetworkInit>();
}

std::string winsockStrError(DWORD errorCode)
{
    LPWSTR message = nullptr;
    DWORD formatMessageRetVal = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode, 0, (LPWSTR)&message, 1, nullptr);
    if(formatMessageRetVal == 0 || message == nullptr)
    {
        std::ostringstream ss;
        ss << errorCode << " (message formatting failed)";
        return ss.str();
    }
    std::wstring retval = std::wstring(message, formatMessageRetVal);
    LocalFree(message);
    return string_cast<std::string>(retval);
}

class NetworkReader final : public Reader
{
private:
    std::shared_ptr<unsigned> fd;
    bool availableData = false;
public:
    explicit NetworkReader(std::shared_ptr<unsigned> fd)
        : fd(fd)
    {
        assert(fd != nullptr);
        DWORD flag = 1;
        setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));
    }
    virtual ~NetworkReader()
    {
        shutdown(*fd, SD_RECEIVE);
    }
    virtual std::uint8_t readByte() override
    {
        std::uint8_t retval;
        readAllBytes(&retval, 1);
        return retval;
    }
    virtual bool dataAvailable() override
    {
        if(availableData)
            return true;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(*fd, &readfds);
        TIMEVAL timeout = {0, 0};
        int selectRetVal = select(*fd + 1, &readfds, nullptr, nullptr, &timeout);
        if(selectRetVal == SOCKET_ERROR)
        {
            DWORD errorValue = WSAGetLastError();
            throw NetworkException(std::string("io error : ") + winsockStrError(errorValue));
        }
        if(selectRetVal == 0)
            return false;
        availableData = true;
        return true;
    }
    virtual std::size_t readBytes(std::uint8_t *array, std::size_t maxCount) override
    {
        if(maxCount == 0)
            return 0;
        availableData = false;
        std::size_t retval = 0;
        for(;;)
        {
            int recvSize = (int)maxCount;
            if(maxCount >= 0x10000)
                recvSize = 0x10000;
            int recvRetVal = recv(*fd, (char *)array, recvSize, MSG_WAITALL);
            if(recvRetVal == SOCKET_ERROR)
            {
                DWORD errorValue = WSAGetLastError();
                throw NetworkException(std::string("io error : ") + winsockStrError(errorValue));
            }
            else if(recvRetVal == 0)
            {
                return retval;
            }
            else if((std::size_t)recvRetVal == maxCount)
            {
                return retval + maxCount;
            }
            else
            {
                array += recvRetVal;
                maxCount -= recvRetVal;
                retval += recvRetVal;
            }
        }
    }
    virtual std::size_t readAvailableBytes(std::uint8_t *array, std::size_t maxCount) override
    {
        if(maxCount == 0)
            return 0;
        std::size_t retval = 0;
        for(;;)
        {
            if(!dataAvailable())
                return retval;
            int recvSize = (int)maxCount;
            if(maxCount >= 0x10000)
                recvSize = 0x10000;
            int recvRetVal = recv(*fd, (char *)array, recvSize, 0);
            availableData = false;
            if(recvRetVal == SOCKET_ERROR)
            {
                DWORD errorValue = WSAGetLastError();
                throw NetworkException(std::string("io error : ") + winsockStrError(errorValue));
            }
            else if(recvRetVal == 0)
            {
                return retval;
            }
            else if((std::size_t)recvRetVal == maxCount)
            {
                return retval + maxCount;
            }
            else
            {
                array += recvRetVal;
                maxCount -= recvRetVal;
                retval += recvRetVal;
            }
        }
    }
};

class NetworkWriter final : public Writer
{
private:
    std::vector<std::uint8_t> buffer;
    std::shared_ptr<unsigned> fd;
public:
    explicit NetworkWriter(std::shared_ptr<unsigned> fd)
        : buffer(), fd(fd)
    {
        assert(fd != nullptr);
        DWORD flag = 1;
        setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));
    }
    virtual ~NetworkWriter()
    {
        shutdown(*fd, SD_SEND);
    }
    virtual void writeByte(std::uint8_t v) override
    {
        buffer.push_back(v);
        if(buffer.size() >= 0x4000)
            flush();
    }
    virtual void flush() override
    {
        const std::uint8_t *pbuffer = buffer.data();
        std::size_t sizeLeft = buffer.size();
        while(sizeLeft > 0)
        {
            int retval = send(*fd, (const char *)pbuffer, sizeLeft, 0);
            if(retval == SOCKET_ERROR)
            {
                DWORD errorValue = WSAGetLastError();
                throw NetworkException(std::string("io error : ") + winsockStrError(errorValue));
            }
            else
            {
                sizeLeft -= retval;
                pbuffer += retval;
            }
        }
        int flag = 1;
        setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));
        buffer.clear();
    }
};
}

NetworkConnection::NetworkConnection(std::wstring url, std::uint16_t port)
    : readerInternal(),
    writerInternal()
{
    initNetwork();
    std::string url_utf8 = string_cast<std::string>(url), port_str = std::to_string((unsigned)port);
    addrinfo *addrList = nullptr;
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_family = 0;
    int retval = getaddrinfo(url_utf8.c_str(), port_str.c_str(), &hints, &addrList);

    if(0 != retval)
    {
        throw NetworkException(std::string("getaddrinfo: ") + winsockStrError(retval));
    }

    unsigned fd = INVALID_SOCKET;

    DWORD errorValue;

    for(addrinfo *addr = addrList; addr; addr = addr->ai_next)
    {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if(fd == INVALID_SOCKET)
        {
            errorValue = WSAGetLastError();
            continue;
        }
        if(SOCKET_ERROR != connect(fd, addr->ai_addr, addr->ai_addrlen))
        {
            break;
        }
        errorValue = WSAGetLastError();
        closesocket(fd);
        fd = INVALID_SOCKET;
    }

    if(fd == INVALID_SOCKET)
    {
        std::string msg = "can't connect: ";
        msg += winsockStrError(errorValue);
        freeaddrinfo(addrList);
        throw NetworkException(msg);
    }

    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));

    freeaddrinfo(addrList);
    std::shared_ptr<unsigned> pfd = std::shared_ptr<unsigned>(new unsigned(fd), [](unsigned *pfd)
    {
        closesocket(*pfd);
        delete pfd;
    });
    readerInternal = std::shared_ptr<Reader>(new NetworkReader(pfd));
    writerInternal = std::shared_ptr<Writer>(new NetworkWriter(pfd));
}

unsigned NetworkServer::startServer(std::uint16_t port)
{
    initNetwork();
    addrinfo hints;
    memset((void *)&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    addrinfo *addrList;
    std::string port_str = std::to_string((unsigned)port);
    int retval = getaddrinfo(NULL, port_str.c_str(), &hints, &addrList);

    if(0 != retval)
    {
        throw NetworkException(std::string("getaddrinfo: ") + winsockStrError(retval));
    }

    unsigned fd = INVALID_SOCKET;
    const char *errorStr = "getaddrinfo";

    DWORD errorValue;

    for(addrinfo *addr = addrList; addr != NULL; addr = addr->ai_next)
    {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        errorStr = "socket";

        if(fd == INVALID_SOCKET)
        {
            errorValue = WSAGetLastError();
            continue;
        }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

        if(::bind(fd, addr->ai_addr, addr->ai_addrlen) == 0)
        {
            break;
        }

        errorValue = WSAGetLastError();
        closesocket(fd);
        errorStr = "bind";
        fd = INVALID_SOCKET;
    }

    if(fd == INVALID_SOCKET)
    {
        std::string msg = std::string(errorStr) + ": ";
        msg += winsockStrError(errorValue);
        freeaddrinfo(addrList);
        throw NetworkException(msg);
    }

    freeaddrinfo(addrList);

    if(listen(fd, 50) == SOCKET_ERROR)
    {
        DWORD errorValue = WSAGetLastError();
        std::string msg = "listen: ";
        msg += winsockStrError(errorValue);
        closesocket(fd);
        throw NetworkException(msg);
    }
    return (unsigned)fd;
}

NetworkServer::NetworkServer(std::uint16_t port)
    : fd(new unsigned(startServer(port)), [](unsigned *pfd)
    {
        closesocket(*pfd);
        delete pfd;
    })
{
}

NetworkServer::~NetworkServer()
{
}

std::shared_ptr<StreamRW> NetworkServer::accept()
{
    unsigned fd2 = ::accept(*fd, nullptr, nullptr);

    if(fd2 == INVALID_SOCKET)
    {
        DWORD errorValue = WSAGetLastError();
        std::string msg = "accept: ";
        msg += winsockStrError(errorValue);
        throw NetworkException(msg);
    }

    int flag = 1;
    setsockopt(fd2, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));

    std::shared_ptr<unsigned> pfd2 = std::shared_ptr<unsigned>(new unsigned(fd2), [](unsigned *pfd)
    {
        closesocket(*pfd);
        delete pfd;
    });
    std::shared_ptr<Reader> reader = std::shared_ptr<Reader>(new NetworkReader(pfd2));
    std::shared_ptr<Writer> writer = std::shared_ptr<Writer>(new NetworkWriter(pfd2));
    return std::shared_ptr<StreamRW>(new StreamRWWrapper(reader, writer));
}

}
}
}
#elif __ANDROID || __APPLE__ || __linux || __unix || __posix
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <vector>

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace stream
{

namespace
{
initializer signalInit([]()
{
    signal(SIGPIPE, SIG_IGN);
});

class NetworkReader final : public Reader
{
private:
    std::shared_ptr<unsigned> fd;
    bool availableData = false;
public:
    explicit NetworkReader(std::shared_ptr<unsigned> fd)
        : fd(fd)
    {
        assert(fd != nullptr);
        int flag = 1;
        setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));
    }
    virtual ~NetworkReader()
    {
        shutdown(*fd, SHUT_RD);
    }
    virtual std::uint8_t readByte() override
    {
        std::uint8_t retval;
        readAllBytes(&retval, 1);
        return retval;
    }
    virtual bool dataAvailable() override
    {
        if(availableData)
            return true;
        pollfd fds;
        fds.fd = *fd;
        fds.events = POLLIN;
        int pollRetVal = poll(&fds, 1, 0);
        if(pollRetVal < 0)
        {
            int errorValue = errno;
            throw NetworkException(std::string("io error : ") + std::strerror(errorValue));
        }
        if(pollRetVal == 0)
            return false;
        if(fds.revents & (POLLERR | POLLHUP))
            return false;
        availableData = true;
        return true;
    }
    virtual std::size_t readBytes(std::uint8_t *array, std::size_t maxCount) override
    {
        std::size_t retval = 0;
        for(;;)
        {
            ssize_t recvRetVal = recv(*fd, (void *)array, maxCount, 0);
            if(recvRetVal == -1)
            {
                int errorValue = errno;
                if(errorValue != EINTR)
                    throw NetworkException(std::string("io error : ") + std::strerror(errorValue));
            }
            else if(recvRetVal == 0)
            {
                return retval;
            }
            else if((std::size_t)recvRetVal == maxCount)
            {
                return retval + maxCount;
            }
            else
            {
                array += recvRetVal;
                maxCount -= recvRetVal;
                retval += recvRetVal;
            }
        }
    }
    virtual std::size_t readAvailableBytes(std::uint8_t *array, std::size_t maxCount) override
    {
        std::size_t retval = 0;
        for(;;)
        {
            ssize_t recvRetVal = recv(*fd, (void *)array, maxCount, MSG_DONTWAIT);
            if(recvRetVal == -1)
            {
                int errorValue = errno;
                if(errorValue == EAGAIN || errorValue == EWOULDBLOCK)
                    return retval;
                if(errorValue != EINTR)
                    throw NetworkException(std::string("io error : ") + std::strerror(errorValue));
            }
            else if(recvRetVal == 0)
            {
                return retval;
            }
            else if((std::size_t)recvRetVal == maxCount)
            {
                return retval + maxCount;
            }
            else
            {
                array += recvRetVal;
                maxCount -= recvRetVal;
                retval += recvRetVal;
            }
        }
    }
};

class NetworkWriter final : public Writer
{
private:
    std::vector<std::uint8_t> buffer;
    std::shared_ptr<unsigned> fd;
public:
    explicit NetworkWriter(std::shared_ptr<unsigned> fd)
        : buffer(), fd(fd)
    {
        assert(fd != nullptr);
        int flag = 1;
        setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));
    }
    virtual ~NetworkWriter()
    {
        shutdown(*fd, SHUT_WR);
    }
    virtual void writeByte(std::uint8_t v) override
    {
        buffer.push_back(v);
        if(buffer.size() >= 0x4000)
            flush();
    }
    virtual void flush() override
    {
        const std::uint8_t *pbuffer = buffer.data();
        ssize_t sizeLeft = buffer.size();
        while(sizeLeft > 0)
        {
            ssize_t retval = send(*fd, (const void *)pbuffer, sizeLeft, 0);
            if(retval == -1)
            {
                int errorValue = errno;
                if(errorValue != EINTR)
                    throw NetworkException(std::string("io error : ") + std::strerror(errorValue));
            }
            else
            {
                sizeLeft -= retval;
                pbuffer += retval;
            }
        }
        int flag = 1;
        setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));
        buffer.clear();
    }
};
}

NetworkConnection::NetworkConnection(std::wstring url, std::uint16_t port)
    : readerInternal(),
    writerInternal()
{
    std::string url_utf8 = string_cast<std::string>(url), port_str = std::to_string((unsigned)port);
    addrinfo *addrList = nullptr;
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_family = 0;
    int retval = getaddrinfo(url_utf8.c_str(), port_str.c_str(), &hints, &addrList);

    if(0 != retval)
    {
        throw NetworkException(std::string("getaddrinfo: ") + gai_strerror(retval));
    }

    int fd = -1;

    for(addrinfo *addr = addrList; addr; addr = addr->ai_next)
    {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if(fd == -1)
        {
            continue;
        }
        if(-1 != connect(fd, addr->ai_addr, addr->ai_addrlen))
        {
            break;
        }
        int errorValue = errno;
        close(fd);
        errno = errorValue;
        fd = -1;
    }

    if(fd == -1)
    {
        int errorValue = errno;
        std::string msg = "can't connect: ";
        msg += std::strerror(errorValue);
        freeaddrinfo(addrList);
        throw NetworkException(msg);
    }

    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));

    freeaddrinfo(addrList);
    std::shared_ptr<unsigned> pfd = std::shared_ptr<unsigned>(new unsigned(fd), [](unsigned *pfd)
    {
        close(*pfd);
        delete pfd;
    });
    readerInternal = std::shared_ptr<Reader>(new NetworkReader(pfd));
    writerInternal = std::shared_ptr<Writer>(new NetworkWriter(pfd));
}

unsigned NetworkServer::startServer(std::uint16_t port)
{
    addrinfo hints;
    memset((void *)&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    addrinfo *addrList;
    std::string port_str = std::to_string((unsigned)port);
    int retval = getaddrinfo(NULL, port_str.c_str(), &hints, &addrList);

    if(0 != retval)
    {
        throw NetworkException(std::string("getaddrinfo: ") + gai_strerror(retval));
    }

    int fd = -1;
    const char *errorStr = "getaddrinfo";

    for(addrinfo *addr = addrList; addr != NULL; addr = addr->ai_next)
    {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        errorStr = "socket";

        if(fd < 0)
        {
            continue;
        }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

        if(::bind(fd, addr->ai_addr, addr->ai_addrlen) == 0)
        {
            break;
        }

        int temp = errno;
        close(fd);
        errno = temp;
        errorStr = "bind";
        fd = -1;
    }

    if(fd < 0)
    {
        int errorValue = errno;
        std::string msg = std::string(errorStr) + ": ";
        msg += std::strerror(errorValue);
        freeaddrinfo(addrList);
        throw NetworkException(msg);
    }

    freeaddrinfo(addrList);

    if(listen(fd, 50) == -1)
    {
        int errorValue = errno;
        std::string msg = "listen: ";
        msg += std::strerror(errorValue);
        close(fd);
        throw NetworkException(msg);
    }
    return (unsigned)fd;
}

NetworkServer::NetworkServer(std::uint16_t port)
    : fd(new unsigned(startServer(port)), [](unsigned *pfd)
    {
        close(*pfd);
        delete pfd;
    })
{
}

NetworkServer::~NetworkServer()
{
}

std::shared_ptr<StreamRW> NetworkServer::accept()
{
    int fd2 = ::accept(*fd, nullptr, nullptr);

    if(fd2 < 0)
    {
        int errorValue = errno;
        std::string msg = "accept: ";
        msg += std::strerror(errorValue);
        throw NetworkException(msg);
    }

    int flag = 1;
    setsockopt(fd2, IPPROTO_TCP, TCP_NODELAY, (const void *)&flag, sizeof(flag));

    std::shared_ptr<unsigned> pfd2 = std::shared_ptr<unsigned>(new unsigned(fd2), [](unsigned *pfd)
    {
        close(*pfd);
        delete pfd;
    });
    std::shared_ptr<Reader> reader = std::shared_ptr<Reader>(new NetworkReader(pfd2));
    std::shared_ptr<Writer> writer = std::shared_ptr<Writer>(new NetworkWriter(pfd2));
    return std::shared_ptr<StreamRW>(new StreamRWWrapper(reader, writer));
}

}
}
}
#else
#error unknown platform for network.cpp
#endif
