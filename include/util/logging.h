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
#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED

#include <sstream>
#include <mutex>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <cassert>
#include <functional>
#include "util/string_cast.h"

namespace programmerjake
{
namespace voxels
{
struct post_t
{
    friend void operator <<(std::wostream &os, post_t);
};

class LogStream : public std::wostringstream
{
    LogStream(const LogStream &) = delete;
    LogStream &operator =(const LogStream &) = delete;
private:
    std::function<void(std::wstring)> *const postFunction;
    bool inPostFunction = false;
public:
    std::recursive_mutex *const theLock;
    LogStream(std::recursive_mutex *theLock, std::function<void(std::wstring)> *postFunction)
        : postFunction(postFunction), theLock(theLock)
    {
    }
    void setPostFunction(std::function<void(std::wstring)> newPostFunction)
    {
        assert(newPostFunction != nullptr);
        std::unique_lock<std::recursive_mutex> lockIt(*theLock);
        *postFunction = newPostFunction;
    }
    std::function<void(std::wstring)> getPostFunction()
    {
        std::unique_lock<std::recursive_mutex> lockIt(*theLock);
        return *postFunction;
    }
    bool isInPostFunction() const
    {
        std::unique_lock<std::recursive_mutex> lockIt(*theLock);
        return inPostFunction;
    }
    friend void operator <<(std::wostream &os, post_t);
};

inline void operator <<(std::wostream &os, post_t)
{
    LogStream *logStream = dynamic_cast<LogStream *>(&os);
    assert(logStream != nullptr);
    std::unique_lock<std::recursive_mutex> lockIt(*logStream->theLock);
    try
    {
        logStream->inPostFunction = true;
        (*logStream->postFunction)(logStream->str());
        logStream->str(L"");
    }
    catch(...)
    {
        logStream->inPostFunction = false;
        throw;
    }
    logStream->inPostFunction = false;
}

inline LogStream &getDebugLog()
{
    static std::recursive_mutex theLock;
    static std::function<void(std::wstring)> postFunction = [](std::wstring str)
    {
        while(!str.empty())
        {
            std::size_t pos = str.find_first_of(L"\r\n");
            std::wstring currentLine;
            if(pos == std::wstring::npos)
            {
                currentLine = str;
                str.clear();
            }
            else
            {
                currentLine = str.substr(0, pos + 1);
                str.erase(0, pos + 1);
            }
            assert(!currentLine.empty());
            if(currentLine[currentLine.size() - 1] == L'\r')
            {
                currentLine.erase(currentLine.size() - 1);
                currentLine += L"\x1b[K\r";
            }
            else if(currentLine[currentLine.size() - 1] == L'\n')
            {
                currentLine.erase(currentLine.size() - 1);
                currentLine += L"\x1b[K\n";
            }
            else
            {
                currentLine += L"\x1b[K";
            }
            std::cout << string_cast<std::string>(currentLine);
        }
        std::cout << std::flush;
    };
    struct theLogStream_tls_tag
    {
    };
    static thread_local std::unique_ptr<LogStream> theLogStream;
    if(theLogStream == nullptr)
        theLogStream.reset(new LogStream(&theLock, &postFunction));
    return *theLogStream;
}

constexpr post_t post{};

struct postnl_t
{
    friend void operator <<(std::wostream &os, postnl_t)
    {
        os << L"\n" << post;
    }
};

constexpr postnl_t postnl{};

struct postr_t
{
    friend void operator <<(std::wostream &os, postr_t)
    {
        os << L"\r" << post;
    }
};

constexpr postr_t postr{};
}
}

#endif // LOGGING_H_INCLUDED
