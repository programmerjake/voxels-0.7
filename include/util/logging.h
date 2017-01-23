/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#include "util/tls.h"

namespace programmerjake
{
namespace voxels
{
struct post_t;
class LogStream final
{
    LogStream(const LogStream &) = delete;
    LogStream &operator=(const LogStream &) = delete;

private:
    std::function<void(std::wstring)> *const postFunction;
    bool inPostFunction = false;
    std::wostringstream ss;

public:
    std::recursive_mutex *const theLock;
    LogStream(std::recursive_mutex *theLock, std::function<void(std::wstring)> *postFunction)
        : postFunction(postFunction), ss(), theLock(theLock)
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
    template <typename T>
    friend LogStream &operator<<(LogStream &os, T &&v)
    {
        os.ss << std::forward<T>(v);
        return os;
    }
    friend void operator<<(LogStream &os, post_t);
};

struct post_t
{
    friend void operator<<(LogStream &os, post_t);
};

void defaultDebugLogPostFunction(std::wstring str);

LogStream &getDebugLog(TLS &tls = TLS::getSlow());

constexpr post_t post{};

struct postnl_t
{
    friend void operator<<(LogStream &os, postnl_t)
    {
        os << L"\n" << post;
    }
};

constexpr postnl_t postnl{};

struct postr_t
{
    friend void operator<<(LogStream &os, postr_t)
    {
        os << L"\r" << post;
    }
};

constexpr postr_t postr{};
}
}

#endif // LOGGING_H_INCLUDED
