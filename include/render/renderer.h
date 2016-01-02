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
#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "render/mesh.h"
#include "platform/platform.h"
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
struct reset_render_layer_t
{
};

static constexpr reset_render_layer_t reset_render_layer{};

struct start_overlay_t
{
};

static constexpr start_overlay_t start_overlay{};

class Renderer
{
    RenderLayer currentRenderLayer = RenderLayer::Opaque;
    void render(const Mesh &m, Matrix tform);
public:
    Renderer & operator <<(const Mesh &m)
    {
        render(m, Matrix::identity());
        return *this;
    }
    Renderer & operator <<(TransformedMesh m)
    {
        assert(m.mesh != nullptr);
        render(*m.mesh, m.tform);
        return *this;
    }
    Renderer & operator <<(ColorizedMesh m)
    {
        return *this << static_cast<Mesh>(m);
    }
    Renderer & operator <<(ColorizedTransformedMesh m)
    {
        return *this << static_cast<Mesh>(m);
    }
    Renderer & operator <<(TransformedMeshRef m)
    {
        render(m.mesh, m.tform);
        return *this;
    }
    Renderer & operator <<(ColorizedMeshRef m)
    {
        return *this << static_cast<Mesh>(m);
    }
    Renderer & operator <<(ColorizedTransformedMeshRef m)
    {
        return *this << static_cast<Mesh>(m);
    }
    Renderer & operator <<(TransformedMeshRRef &&m)
    {
        render(m.mesh, m.tform);
        return *this;
    }
    Renderer & operator <<(ColorizedMeshRRef &&m)
    {
        return *this << static_cast<Mesh>(std::move(m));
    }
    Renderer & operator <<(ColorizedTransformedMeshRRef &&m)
    {
        return *this << static_cast<Mesh>(std::move(m));
    }
    Renderer & operator <<(std::shared_ptr<Mesh> m)
    {
        assert(m != nullptr);
        operator <<(*m);
        return *this;
    }
    Renderer & operator <<(reset_render_layer_t)
    {
        currentRenderLayer = RenderLayer::Opaque;
        return *this;
    }
    Renderer & operator <<(start_overlay_t)
    {
        Display::initOverlay();
        return *this;
    }
    Renderer & operator <<(RenderLayer rl)
    {
        currentRenderLayer = rl;
        return *this;
    }
    Renderer & operator <<(const MeshBuffer &m)
    {
        Display::render(m, currentRenderLayer);
        return *this;
    }
};
}
}

#endif // RENDERER_H_INCLUDED
