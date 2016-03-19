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

not_running_bash=true
if [[ "123" =~ "123" ]]; then
    if [[ "${BASH_REMATCH[0]}" == "123" ]]; then
        not_running_bash=false
    fi
fi

if "$not_running_bash"; then
    echo "this program needs to be run in bash" >&2
    exit 1
fi

zeros="0000000"

function parse_float()
{
    local v="$1" retval=0 decimals
    if [[ "$v" =~ ^('-'?[0-9][0-9]*)('.'([0-9][0-9]*)'f'?)?$ ]]; then
        decimals="${BASH_REMATCH[3]}$zeros"
        v="${BASH_REMATCH[1]}${decimals:0:${#zeros}}"
        echo $((v))
    else
        return 1
    fi
    return 0
}

function output_float()
{
    local v decimals
    [[ "$1" =~ ^'-'?[0-9]+$ ]] || return 1
    ((v = $1))
    if (( v < 0 )); then
        (( v = -v ))
        echo -n '-'
    fi
    decimals="$((v % 1$zeros))"
    if (( decimals )); then
        decimals="${zeros:${#decimals}}$decimals"
        [[ "$decimals" =~ ^([0-9]*[1-9])?'0'*$ ]]
        decimals="${BASH_REMATCH[1]}"
        echo "$((v / 1$zeros)).${decimals}f"
    else
        echo "$((v / 1$zeros))"
    fi
    return 0
}

function parse_line()
{
    local line="$1" x y kind
    if [[ "$line" =~ ^[[:space:]]*'{'[[:space:]]*('-'?[0-9][0-9.f]*)[[:space:]]*','[[:space:]]*('-'?[0-9][0-9.f]*)[[:space:]]*','[[:space:]]*'VectorFontVertex'[[:space:]]*'::'[[:space:]]*('StartLoop'|'Vertex'|'EndLoop'|'EndGlyph')[[:space:]]*'}'[[:space:]]*(','[[:space:]]*)?$ ]]; then
        x="${BASH_REMATCH[1]}"
        y="${BASH_REMATCH[2]}"
        kind="${BASH_REMATCH[3]}"
        #echo "x='$x' y='$y' kind='$kind'" >&2
        case "$kind" in
        StartLoop) 
            line="S `parse_float "$x"`" || return 1
            line="$line `parse_float "$y"`" || return 1
            ;;
        Vertex) 
            line="V `parse_float "$x"`" || return 1
            line="$line `parse_float "$y"`" || return 1
            ;;
        EndLoop) 
            line="E `parse_float "$x"`" || return 1
            line="$line `parse_float "$y"`" || return 1
            ;;
        EndGlyph) 
            line=";"
            ;;
        *)
            return 1
            ;;
        esac
        echo "$line"
    else
        return 1
    fi
    return 0
}

function output_line()
{
    local indent="    "
    [[ "$1" =~ ^(.)(' '('-'?[0-9]+)' '('-'?[0-9]+))?$ ]] || return 1
    local kind="${BASH_REMATCH[1]}" x="${BASH_REMATCH[3]}" y="${BASH_REMATCH[4]}"
    case "$kind" in
    S)
        x="`output_float "$x"`" || return 1
        y="`output_float "$y"`" || return 1
        echo "$indent{$x, $y, VectorFontVertex::StartLoop},"
        ;;
    V)
        x="`output_float "$x"`" || return 1
        y="`output_float "$y"`" || return 1
        echo "$indent{$x, $y, VectorFontVertex::Vertex},"
        ;;
    E)
        x="`output_float "$x"`" || return 1
        y="`output_float "$y"`" || return 1
        echo "$indent{$x, $y, VectorFontVertex::EndLoop},"
        ;;
    \;)
        echo "$indent{0, 0, VectorFontVertex::EndGlyph},"
        ;;
    *)
        return 1
        ;;
    esac
    return 0
}

function set_line_kind()
{
    [[ "$1" =~ ^(.)(' '('-'?[0-9]+)' '('-'?[0-9]+))?$ ]] || return 1
    local kind="$2" x="${BASH_REMATCH[3]}" y="${BASH_REMATCH[4]}"
    echo "$kind $x $y"
    return 0
}

function transform_line()
{
    if ! [[ "$1" =~ ^(.)(' '('-'?[0-9]+)' '('-'?[0-9]+))?$ ]]; then
        echo "parse failed">&2
        echo -n "transform_line" >&2
        printf " '%s'" "$@">&2
        echo >&2
        return 1
    fi
    local kind="${BASH_REMATCH[1]}" x="${BASH_REMATCH[3]}" y="${BASH_REMATCH[4]}" tform="$2" t
    case "$kind" in
    S|V|E)
        case "$tform" in
        mirror_x)
            (( $# == 3 )) || { echo "invalid arguments to $tform" >&2; return 1; }
            (( x = 2 * $3 - x ))
            ;;
        mirror_y)
            (( $# == 3 )) || { echo "invalid arguments to $tform" >&2; return 1; }
            (( y = 2 * $3 - y ))
            ;;
        rotate_cw)
            (( $# == 4 )) || { echo "invalid arguments to $tform" >&2; return 1; }
            (( t = x - $3 + $4 ))
            (( x = $4 - y + $3 ))
            (( y = t ))
            ;;
        rotate_ccw)
            (( $# == 4 )) || { echo "invalid arguments to $tform" >&2; return 1; }
            (( t = y - $4 + $3 ))
            (( y = $3 - x + $4 ))
            (( x = t ))
            ;;
        rotate_180)
            (( $# == 4 )) || { echo "invalid arguments to $tform" >&2; return 1; }
            (( x = 2 * $3 - x ))
            (( y = 2 * $4 - y ))
            ;;
        translate)
            (( $# == 4 )) || { echo "invalid arguments to $tform" >&2; return 1; }
            (( x += $3 ))
            (( y += $4 ))
            ;;
        reverse)
            (( $# == 2 )) || { echo "invalid arguments to $tform" >&2; return 1; }
            ;;
        *)
            echo "transform $tform not implemented" >&2
            return 1
        esac
        echo "$kind $x $y"
        ;;
    \;)
        echo ";"
        ;;
    *)
        echo "invalid line" >&2
        return 1
        ;;
    esac
    return 0
}

function transform_lines()
{
    local lines
    mapfile -t lines
    local i
    for((i=0;i<${#lines[@]};i++)); do
        lines[i]="`transform_line "${lines[i]}" "$@"`" || return 1
    done
    local loopStart="-1" kind j k temp kKind
    if [[ "$1" == "reverse" ]]; then
        for((i=0;i<${#lines[@]};i++)); do
            [[ "${lines[i]}" =~ ^(.)(' '('-'?[0-9]+)' '('-'?[0-9]+))?$ ]] || return 1
            kind="${BASH_REMATCH[1]}"
            case "$kind" in
            S)
                if [[ "$loopStart" != "-1" ]]; then
                    echo "unexpected start loop" >&2
                    return 1
                fi
                loopStart="$i"
                ;;
            V)
                if [[ "$loopStart" == "-1" ]]; then
                    echo "unexpected vertex" >&2
                    return 1
                fi
                ;;
            E)
                if [[ "$loopStart" == "-1" ]]; then
                    echo "unexpected end loop" >&2
                    return 1
                fi
                for((j=loopStart+1,k=i;j<k;j++,k--)); do
                    temp="`set_line_kind "${lines[j]}" "V"`" || return 1
                    lines[j]="`set_line_kind "${lines[k]}" "V"`" || return 1
                    lines[k]="$temp"
                done
                lines[i]="`set_line_kind "${lines[i]}" "E"`" || return 1
                loopStart="-1"
                ;;
            \;)
                if [[ "$loopStart" != "-1" ]]; then
                    echo "unexpected end glyph" >&2
                    return 1
                fi
                ;;
            *)
                return 1
                ;;
            esac
        done
        if [[ "$loopStart" != "-1" ]]; then
            echo "unexpected eof" >&2
            return 1
        fi
    fi
    printf "%s\n" "${lines[@]}"
    return 0
}

function parse_lines()
{
    local lines line
    mapfile -t lines
    for line in "${lines[@]}"; do
        parse_line "$line" || return 1
    done
    return 0
}

function output_lines()
{
    local lines line
    mapfile -t lines
    for line in "${lines[@]}"; do
        output_line "$line" || return 1
    done
    return 0
}

function handle_failure()
{
    echo "failure" "$@" >&2
    exit 1
}

commands=()
valid_commands=()
valid_commands+=("mirror_x <x>          mirror across the line x=<x>")
valid_commands+=("mirror_y <y>          mirror across the line y=<y>")
valid_commands+=("rotate_cw <x> <y>     rotate 90 degrees counterclockwise around the point (<x>, <y>)")
valid_commands+=("rotate_ccw <x> <y>    rotate 90 degrees clockwise around the point (<x>, <y>)")
valid_commands+=("rotate_180 <x> <y>    rotate 180 degrees around the point (<x>, <y>)")
valid_commands+=("translate <x> <y>     translate by (<x>, <y>)")
valid_commands+=("reverse               reverse the order of vertices in the polygons")

for arg in "$@"; do
    if [[ "$arg" =~ ^'-'?[0-9]+('.'[0-9]*'f'?)?$ ]]; then
        if (( ${#commands[@]} == 0)); then
            echo "number before first command">&2
            exit 1
        fi
        arg="`parse_float "$arg"`" || handle_failure parse_float
        commands[${#commands[@]} - 1]="${commands[${#commands[@]} - 1]} $arg"
    elif [[ "$arg" =~ ^[a-zA-Z_][a-zA-Z_0-9]*$ ]]; then
        commands+=("$arg")
    elif [[ "$arg" =~ ^('-h'|'--help') ]]; then
        echo "usage: $0 [<command> [<command> [...]]]"
        echo "valid commands:"
        for i in "${valid_commands[@]}"; do
            echo "$i"
        done
        exit 0
    else
        echo "invalid argument: '$arg'" >&2
        exit 1
    fi
done

lines=("`parse_lines`") || handle_failure parse_lines
for tform in "${commands[@]}" ; do
    lines="`echo "$lines" | transform_lines $tform`" || handle_failure transform_lines "$tform"
done
echo "$lines" | output_lines

