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
private:
    std::function<void(std::wstring)> *const postFunction;
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
    friend void operator <<(std::wostream &os, post_t);
};

inline void operator <<(std::wostream &os, post_t)
{
    LogStream *logStream = dynamic_cast<LogStream *>(&os);
    assert(logStream != nullptr);
    std::unique_lock<std::recursive_mutex> lockIt(*logStream->theLock);
    (*logStream->postFunction)(logStream->str());
    logStream->str(L"");
}

inline LogStream &getDebugLog()
{
    static std::recursive_mutex theLock;
    static std::function<void(std::wstring)> postFunction = [](std::wstring str)
    {
        std::wcout << str << std::flush;
    };
    static thread_local std::unique_ptr<LogStream> theLogStream;
    if(theLogStream == nullptr)
        theLogStream.reset(new LogStream(&theLock, &postFunction));
    return *theLogStream;
}

constexpr post_t post = post_t{};

struct postnl_t
{
    friend void operator <<(std::wostream &os, postnl_t)
    {
        os << L"\n" << post;
    }
};

constexpr postnl_t postnl = postnl_t{};
}
}

#endif // LOGGING_H_INCLUDED
