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

test_project_file()
{
    local project_file="$1"
    local header_string="`printf "/*\n * Copyright (C) 2012-2016 Jacob R. Lifshay"`"
    local IFS="`printf "\na"`"
    IFS="${IFS%a}"
    local project_files="`cat $project_file | grep '<Unit filename="' | sed '{s/.*filename="\([^"]*\)".*/\1/}'`"
    local files="`find src include ! -type d`"
    local filename found_in_project new_project_files
    for filename in $files; do
        found_in_project=0
        new_project_files=""
        for project_filename in $project_files; do
            if [ "$filename" = "$project_filename" ]; then
                found_in_project=1
            elif [ -z "$new_project_files" ]; then
                new_project_files="$project_filename"
            else
                new_project_files="$new_project_files`echo; echo "$project_filename"`"
            fi
        done
        project_files="$new_project_files"
        if [ "$found_in_project" = 0 ]; then
            echo "$filename: not in project"
        fi
        if [ "$filename" != "src/util/game_version.cpp" ]; then
            if [ "`head -n 2 "$filename"`" != "$header_string" ]; then
                echo "$filename:"
                echo "`head -n 2 "$filename"`"
                echo
                echo vs.
                echo
                echo "$header_string"
                echo
            fi
        fi
    done

    for project_filename in $project_files; do
        echo "$project_filename: not found"
    done
}

test_project_file voxels-0.7.cbp
test_project_file voxels-0.7-win.cbp
