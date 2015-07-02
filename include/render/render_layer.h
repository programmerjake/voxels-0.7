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
#ifndef RENDER_LAYER_H_INCLUDED
#define RENDER_LAYER_H_INCLUDED

#include <cstdint>
#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{
enum class RenderLayer : std::uint8_t
{
    Opaque,
    Translucent,
    DEFINE_ENUM_LIMITS(Opaque, Translucent)
};
}
}

#endif // RENDER_LAYER_H_INCLUDED
