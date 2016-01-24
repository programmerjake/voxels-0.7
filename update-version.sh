#!/bin/sh
# Copyright (C) 2012-2016 Jacob R. Lifshay
# This file is part of Voxels.
#
# Voxels is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Voxels is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Voxels; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
#
#
mkdir -p obj
g++-4.8 -std=c++11 -DCOMPILE_DUMP_VERSION -Iinclude/ src/util/game_version.cpp -o obj/dump-version || exit 1
cat > src/util/game_version.cpp << EOF
// Automatically generated : don't modify
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
#include "util/game_version.h"

const std::wstring programmerjake::voxels::GameVersion::VERSION = L"`obj/dump-version --next-version-str`";
const std::uint32_t programmerjake::voxels::GameVersion::FILE_VERSION = `obj/dump-version`;

#ifdef COMPILE_DUMP_VERSION
#include <iostream>

using namespace programmerjake::voxels;
using namespace std;

int main(int argc, char ** argv)
{
    const int curVersion = `obj/dump-version --next-version`;
    if(argc > 1 && string(argv[1]) == "--next-version")
        cout << (curVersion + 1) << endl;
    else if(argc > 1 && string(argv[1]) == "--next-version-str")
        cout << "0.7.4." << (curVersion + 1) << endl;
    else if(argc > 1)
        cout << "0.7.4." << curVersion << endl;
    else
        cout << GameVersion::FILE_VERSION << endl;
    return 0;
}
#endif // COMPILE_DUMP_VERSION

EOF
