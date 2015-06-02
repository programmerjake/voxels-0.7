/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
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
#include "platform/stack_trace.h"
#include "util/logging.h"
#include <cstdlib>
#include <algorithm>
#include <cstring>
#if _WIN64 || _WIN32
FIXME_MESSAGE(finish stack trace code for windows)
#elif __ANDROID || __APPLE__ || __linux || __unix || __posix
#include <execinfo.h>
#include <cxxabi.h>

namespace programmerjake
{
namespace voxels
{
void StackTrace::SymbolListDeleter::operator ()(const char * const*ptr) const
{
    std::free(const_cast<const char **>(ptr));
}
StackTrace StackTrace::make()
{
    StackTrace retval;
    retval.usedAddressCount = std::max<int>(0, backtrace(&retval.addresses[0], retval.addresses.size()));
    return std::move(retval);
}
void StackTrace::addressesToSymbols()
{
    if(symbolList || empty())
        return;
    symbolList.reset(backtrace_symbols(&addresses[0], usedAddressCount));
}
void StackTrace::dump()
{
    dump(getDebugLog());
    getDebugLog() << postnl;
}
void StackTrace::dump(std::wostream &os)
{
    if(empty())
    {
        os << L"<empty stack trace>\n";
        return;
    }
    addressesToSymbols();
    const std::size_t functionNameSize = 512;
    static thread_local char mangledFunctionName[functionNameSize];
    for(std::size_t i = 0; i < usedAddressCount; i++)
    {
        std::strncpy(mangledFunctionName, symbolList.get()[i], functionNameSize);
        char *beginName = nullptr;
        char *beginOffset = nullptr;
        char *endOffset = nullptr;
        for(char *j = mangledFunctionName; *j; j++)
        {
            switch(*j)
            {
            case '(':
                beginName = j;
                break;
            case '+':
                beginOffset = j;
                break;
            case ')':
                if(beginOffset)
                    endOffset = j;
                break;
            }
        }
        if(beginName && beginOffset && endOffset && beginName < beginOffset)
        {
            *beginName++ = '\0';
            *beginOffset++ = '\0';
            *endOffset++ = '\0';
            int status;
            char *demangledFunctionName = abi::__cxa_demangle(beginName, nullptr, nullptr, &status);
            if(status == 0)
            {
                try
                {
                    os << demangledFunctionName << "+" << beginOffset;
                }
                catch(...)
                {
                    std::free(demangledFunctionName);
                    throw;
                }
                std::free(demangledFunctionName);
            }
            else
            {
                os << beginName << "()+" << beginOffset;
            }
            os << "\n";
        }
    }
}
}
}
#else
#error unknown platform for network.cpp
#endif
