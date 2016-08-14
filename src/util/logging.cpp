/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
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
#include "util/logging.h"
#include "platform/platform.h"

namespace programmerjake
{
namespace voxels
{
void defaultDebugLogPostFunction(std::wstring str)
{
    platformPostLogMessage(str);
}

void operator<<(LogStream &os, post_t)
{
    LogStream *logStream = dynamic_cast<LogStream *>(&os);
    assert(logStream != nullptr);
    std::unique_lock<std::recursive_mutex> lockIt(*logStream->theLock);
    try
    {
        logStream->inPostFunction = true;
        (*logStream->postFunction)(logStream->ss.str());
        logStream->ss.str(L"");
    }
    catch(...)
    {
        logStream->inPostFunction = false;
        throw;
    }
    logStream->inPostFunction = false;
}

LogStream &getDebugLog(TLS &tls)
{
    static std::recursive_mutex theLock;
    static std::function<void(std::wstring)> postFunction = defaultDebugLogPostFunction;
    struct TheLogStreamTag
    {
    };
    thread_local_variable<std::unique_ptr<LogStream>, TheLogStreamTag> theLogStream(tls);
    if(theLogStream.get() == nullptr)
        theLogStream.get().reset(new LogStream(&theLock, &postFunction));
    return *theLogStream.get();
}
}
}
