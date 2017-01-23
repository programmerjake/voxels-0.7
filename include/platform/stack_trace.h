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
#ifndef STACK_TRACE_H_INCLUDED
#define STACK_TRACE_H_INCLUDED

#include <cstddef>
#include <memory>
#include <utility>
#include <ostream>
#include "util/checked_array.h"

namespace programmerjake
{
namespace voxels
{
class StackTrace final
{
private:
    checked_array<void *, 16> addresses;
    std::size_t usedAddressCount = 0;
    struct SymbolListDeleter final
    {
        void operator()(const char *const *ptr) const;
    };
    std::unique_ptr<const char *const, SymbolListDeleter> symbolList;
    void addressesToSymbols();

public:
    StackTrace() : addresses{nullptr}, symbolList()
    {
    }
    static StackTrace make();
    StackTrace(StackTrace &&rt)
        : addresses(rt.addresses),
          usedAddressCount(rt.usedAddressCount),
          symbolList(std::move(rt.symbolList))
    {
    }
    StackTrace(const StackTrace &rt)
        : addresses(rt.addresses), usedAddressCount(rt.usedAddressCount), symbolList()
    {
    }
    void swap(StackTrace &rt)
    {
        using std::swap;
        swap(addresses, rt.addresses);
        swap(usedAddressCount, rt.usedAddressCount);
        swap(symbolList, rt.symbolList);
    }
    friend void swap(StackTrace &l, StackTrace &r)
    {
        l.swap(r);
    }
    StackTrace &operator=(StackTrace rt)
    {
        swap(rt);
        return *this;
    }
    bool operator==(const StackTrace &rt) const
    {
        if(usedAddressCount != rt.usedAddressCount)
            return false;
        for(std::size_t i = 0; i < usedAddressCount; i++)
        {
            if(addresses[i] != rt.addresses[i])
                return false;
        }
        return true;
    }
    bool operator!=(const StackTrace &rt) const
    {
        return !operator==(rt);
    }
    bool empty() const
    {
        return usedAddressCount == 0;
    }
    explicit operator bool() const
    {
        return !empty();
    }
    bool operator!() const
    {
        return empty();
    }
    void dump();
    void dump(std::wostream &os);
    std::size_t hash() const
    {
        if(empty())
            return 0;
        std::size_t retval = usedAddressCount;
        for(std::size_t i = 0; i < usedAddressCount; i++)
        {
            retval = retval * 3 + std::hash<void *>()(addresses[i]);
        }
        return retval;
    }
};
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::StackTrace> final
{
    std::size_t operator()(const programmerjake::voxels::StackTrace &v) const
    {
        return v.hash();
    }
};
}

#endif // STACK_TRACE_H_INCLUDED
