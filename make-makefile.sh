#!/bin/bash
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

declare -a a
target="Release"
arch="linux"
valid_architectures=("linux" "win32" "win64")
OPTIND=1
while getopts "ht:a:" option_name; do
    case "$option_name" in
    h)
        cat <<'EOF'
usage: ./make-makefile.sh <options>
Generate Makefile from the project file.
Options:
-h              show this help.
-t <Target>     set the generated target.
-a <Arch>       set the output architecture.
EOF
        exit 0
        ;;
    t)
        target="$OPTARG"
        ;;
    a)
        arch="$OPTARG"
        found_arch=0
        for i in "${valid_architectures[@]}"; do
            if [[ "$arch" == "$i" ]]; then
                found_arch=1
                break
            fi
        done
        if ! ((found_arch)); then
            echo "$0: invalid architecture" >&2
            exit 1
        fi
        ;;
    *)
        exit 1
        ;;
    esac
done
if ((OPTIND <= $#)); then
    echo "$0: too many options" >&2
    exit 1
fi
case "$arch" in
linux)
    project_filename="voxels-0.7.cbp"
    ;;
win64)
    project_filename="voxels-0.7-win.cbp"
    ;;
win32)
    project_filename="voxels-0.7-win.cbp"
    ;;
esac
mapfile -t a < <(sed -z 's/\r\n/\n/g' "$project_filename")
found_target=0
current_target=""
in_compiler=0
in_linker=0
ignore_nest_count=0
compiler_command_line=()
linker_command_line=()
output_executable_name=""
compiler_name=""
object_output_dir=""
files=()
for((i=0;i<${#a[@]};i++)); do
    line="${a[i]}"
    if [[ "$line" =~ '</Extensions>' || "$line" =~ '</VirtualTargets>' ]]; then
        ignore_nest_count=$((ignore_nest_count - 1))
    elif [[ "$line" =~ '<Extensions>' || "$line" =~ '<VirtualTargets>' ]]; then
        ignore_nest_count=$((ignore_nest_count + 1))
    elif ((ignore_nest_count)); then
        :
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
    elif [[ "$line" =~ '<Option platforms="'[^\"]*'" />' ]]; then
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
march_override=""
case "$arch" in
linux)
    output_file_extension=""
    build_dir_suffix=""
    gxx_name="g++"
    gcc_name="gcc"
    ;;
win64)
    output_file_extension="-w64.exe"
    build_dir_suffix="-Win64"
    gxx_name="x86_64-w64-mingw32-g++"
    gcc_name="x86_64-w64-mingw32-gcc"
    linker_command_line=(-mconsole -lmingw32 -lSDL2main -lSDL2 -Wl,--no-undefined -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -static-libstdc++ -lvorbisfile -lpng.dll -lz -lopengl32 -lvorbis -logg)
    ;;
win32)
    output_file_extension="-w32.exe"
    build_dir_suffix="-Win32"
    gxx_name="i686-w64-mingw32-g++"
    gcc_name="i686-w64-mingw32-gcc"
    march_override="-march=i686"
#    linker_command_line=(-mconsole -lmingw32 -lSDL2main -lSDL2 -Wl,--no-undefined -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -static-libstdc++ -lvorbisfile -lpng.dll -lz -lopengl32 -lvorbis -logg)
    ;;
esac
object_output_dir="${object_output_dir%/}$build_dir_suffix/"
if [[ ! -z "$march_override" ]]; then
    for((i=0;i<${#compiler_command_line[@]};i++)); do
        if [[ "${compiler_command_line[i]:0:7}" == "-march=" ]]; then
            compiler_command_line[i]="$march_override"
            march_override=""
            break
        fi
    done
    if [[ ! -z "$march_override" ]]; then
        compiler_command_line=("${compiler_command_line[@]}" "$march_override")
    fi
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
output_executable_name="$output_executable_name$output_file_extension"
exec 2>&1 > Makefile
echo "# generated from $project_filename"
cat <<'EOF'
# Copyright (C) 2012-2016 Jacob R. Lifshay
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
echo CXX = "$gxx_name"
echo CC = "$gcc_name"
echo LD = "$gxx_name"
echo
echo ".PHONY: all build clean prebuild"
echo
echo "all: build"
echo
echo "clean:"
echo $'\t'rm -rf "${object_output_dir%/}"
echo $'\t'rm -f "$output_executable_name"
echo
echo "build: prebuild $output_executable_name"
echo
echo "prebuild:"
printf "\t@mkdir -p %s\n" "${object_directories[@]}"
if [[ "$output_executable_name" =~ ^(.+)/[^/]*$ ]]; then
    printf "\t@mkdir -p %s\n" "${BASH_REMATCH[1]}"
fi
echo
echo "$output_executable_name: ${objects[@]} | prebuild"
echo $'\t'"@printf '\x1b[0;1;34mLinking\x1b[m\n'"
echo $'\t''@$(LD)' "${objects[@]}" -o "$output_executable_name" "${linker_command_line[@]}"
echo
index=0
max_index=${#objects[@]}
pound_line="##############################"
space_line="                              "
printf "processing dependencies...\n" >&2
for source in "${!objects[@]}"; do
    index=$((index + 1))
    printf "|%s%s| %i%% (%s)\x1b[K\r" "${pound_line::index * ${#pound_line} / max_index}" "${space_line:index * ${#pound_line} / max_index}" $((index * 100 / max_index)) "$source" >&2
    printf "%s: " "${objects["$source"]}"
    gcc_executable="$gxx_name"
    compiler_line_start=""
    if [ "${source%.c}" != "${source}" ]; then
        compiler_line_start=$'\t@$(CC)'
        gcc_executable="$gcc_name"
    else
        compiler_line_start=$'\t@$(CXX)'
    fi
    (bash -c "$gcc_executable ${compiler_command_line[*]} $source -M" | sed '{s/^ *//; s/ \\$//g; s/ \([^ \\]\)/\n\1/g}' | sed '{s/^[^/].*[^:]$/&/p; d}' | tr $'\n' ' ' | sed '{s/^\(.*[^ ]\) *$/\1\n/}' | sed '{s/.*/& | prebuild/}') 2> >(while read v || [ ! -z "$v" ]; do printf "\r%s\x1b[K\n" "$v" >&2; done)
    echo $'\t'"@printf '\x1b[0;1;34mCompiling ${source}\x1b[m\n'"
    echo "$compiler_line_start" -c -o "${objects["$source"]}" "${compiler_command_line[@]}" "$source"
    echo
done
echo >&2
