#!/bin/bash
# Copyright (C) 2012-2015 Jacob R. Lifshay
# This file is part of Voxels.
#
# Voxels is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Voxels is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Voxels; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
#
#

project_filename="voxels-0.7.cbp"
declare -a a
mapfile -t a < "$project_filename"
target="Release"
if [[ ! -z "$1" ]]; then
    target="$1"
fi
found_target=0
current_target=""
in_compiler=0
in_linker=0
in_extensions=0
compiler_command_line=()
linker_command_line=()
output_executable_name=""
compiler_name=""
object_output_dir=""
files=()
for((i=0;i<${#a[@]};i++)); do
    line="${a[i]}"
    if [[ "$line" =~ '</Extensions>' ]]; then
        in_extensions=0
    elif [[ "$line" =~ '<Extensions>' ]] || ((in_extensions)); then
        in_extensions=1
    elif [[ "$line" =~ '</Target>' ]]; then
        current_target=""
    elif [[ "$line" =~ '<Target title="'(.*)'">' ]]; then
        current_target="${BASH_REMATCH[1]}"
        if [ "$current_target" == "$target" ]; then
            found_target=1
        fi
    elif [ "$current_target" != "$target" -a ! -z "$current_target" ]; then
        continue
    elif [[ "$line" =~ '<Compiler>' ]]; then
        in_compiler=1
    elif [[ "$line" =~ '</Compiler>' ]]; then
        in_compiler=0
    elif [[ "$line" =~ '<Linker>' ]]; then
        in_linker=1
    elif [[ "$line" =~ '</Linker>' ]]; then
        in_linker=0
    elif [[ "$line" =~ '<Add option="'(.*)'" />' ]]; then
        if ((in_compiler)); then
            compiler_command_line=("${compiler_command_line[@]}" "${BASH_REMATCH[1]}")
        elif ((in_linker)); then
            linker_command_line=("${linker_command_line[@]}" "${BASH_REMATCH[1]}")
        fi
    elif [[ "$line" =~ '<Add directory="'(.*)'" />' ]]; then
        if ((in_compiler)); then
            compiler_command_line=("${compiler_command_line[@]}" "-I${BASH_REMATCH[1]}")
        elif ((in_linker)); then
            linker_command_line=("${linker_command_line[@]}" "-L${BASH_REMATCH[1]}")
        fi
    elif [[ "$line" =~ '<Add library="'(.*)'" />' ]]; then
        if ((in_linker)); then
            linker_command_line=("${linker_command_line[@]}" "-l${BASH_REMATCH[1]}")
        fi
    elif [[ "$line" =~ '<Option output="'([^\"]*)'"' ]]; then
        output_executable_name="${BASH_REMATCH[1]}"
    elif [[ "$line" =~ '<Option compiler="'([^\"]*)'"' ]]; then
        compiler_name="${BASH_REMATCH[1]}"
    elif [[ "$line" =~ '<Option object_output="'([^\"]*)'"' ]]; then
        object_output_dir="${BASH_REMATCH[1]}"
    elif [[ "$line" =~ '<Unit filename="'([^\"]*('.cpp'|'.c'))'" />'$ ]]; then
        files=("${files[@]}" "${BASH_REMATCH[1]}")
    elif [[ "$line" =~ '<Unit filename="'([^\"]*'.h')'" />'$ ]]; then
        :
    elif [[ "$line" =~ '<?xml'.*'?>' || "$line" =~ '<'/?'CodeBlocks_project_file>' || "$line" =~ '<'/?'Project>' || "$line" =~ '<'/?'Build>' || "$line" =~ '<FileVersion '.*' />' ]]; then
        :
    elif [[ "$line" =~ '<Option parameters="'[^\"]*'" />' || "$line" =~ '<Option title="'[^\"]*'" />' || "$line" =~ '<Option pch_mode="'[^\"]*'" />' || "$line" =~ '<Option type="'[^\"]*'" />' ]]; then
        :
    else
        echo "unhandled line : $i : $line" >&2
        exit 1
    fi
done
if ! ((found_target)); then
    echo "target '$target' not found." >&2
    exit 1
fi
declare -A objects
declare -A object_directories
for file in "${files[@]}"; do
    objects["$file"]="$object_output_dir${file%.*}.o"
done
for file in "${objects[@]}"; do
    directory="${file%/*}"
    object_directories["$directory"]="$directory"
done
exec > Makefile
echo "# generated from $project_filename"
cat <<'EOF'
# Copyright (C) 2012-2015 Jacob R. Lifshay
# This file is part of Voxels.
#
# Voxels is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Voxels is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Voxels; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
#
#
EOF
echo CXX = g++
echo CC = gcc
echo LD = g++
echo
echo ".PHONY: all build clean prebuild postbuild"
echo
echo "all: build"
echo 
echo "clean:"
echo $'\t'rm -rf "${object_output_dir%/}"
echo $'\t'rm -f "$output_executable_name"
echo
echo "build: prebuild $output_executable_name postbuild"
echo
echo "prebuild:"
printf "\tmkdir -p %s\n" "${object_directories[@]}"
if [[ "$output_executable_name" =~ ^(.+)/[^/]*$ ]]; then
    printf "\tmkdir -p %s\n" "${BASH_REMATCH[1]}"
fi
echo
echo "postbuild:"
echo
echo "$output_executable_name: ${objects[@]}"
echo $'\t''$(LD)' "${objects[@]}" -o "$output_executable_name" "${linker_command_line[@]}"
echo
index=0
max_index=${#objects[@]}
pound_line="####################################"
space_line="                                    "
for source in "${!objects[@]}"; do
    index=$((index + 1))
    printf "processing dependencies |%s%s| %i%% (%s)\x1b[K\r" "${pound_line::index * ${#pound_line} / max_index}" "${space_line:index * ${#pound_line} / max_index}" $((index * 100 / max_index)) "$source" >&2
    printf "%s: " "${objects["$source"]}"
    gcc_executable="g++"
    compiler_line_start=""
    if [ "${source%.c}" != "${source}" ]; then
        compiler_line_start=$'\t$(CC)'
        gcc_executable="gcc"
    else
        compiler_line_start=$'\t$(CXX)'
    fi
    (bash -c "$gcc_executable ${compiler_command_line[*]} $source -M" | sed '{s/^ *//; s/ \\$//g; s/ \([^ \\]\)/\n\1/g}' | sed '{s/^[^/].*[^:]$/&/p; d}' | tr $'\n' ' ' | sed '{s/^\(.*\) $/\1\n/}') 2> >(while read v || [ ! -z "$v" ]; do printf "\r%s\x1b[K\n" "$v" >&2; done) 
    echo "$compiler_line_start" -c -o "${objects["$source"]}" "${compiler_command_line[@]}" "$source"
    echo
done
echo >&2



















