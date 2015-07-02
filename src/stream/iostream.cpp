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
#include "stream/iostream.h"
#include <iostream>
#include "util/util.h"
#include <cwchar>
#include <cstdlib>

namespace programmerjake
{
namespace voxels
{
namespace stream
{
#if 0
namespace
{
initializer init1([]()
{
    static const std::uint8_t a[] = "this is a test.\n";
    MemoryReader reader(a);
    ReaderIStream is(reader);
    while(is.peek() != WEOF)
    {
        std::wcout << (wchar_t)is.peek() << L":" << (int)is.peek() << L"\n";
        is.get();
    }
    std::exit(0);
});
}
#endif
}
}
}
