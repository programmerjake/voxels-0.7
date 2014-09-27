/*
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
#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "render/mesh.h"
#include "platform/platform.h"

struct enable_depth_buffer_t
{
};

static constexpr enable_depth_buffer_t enable_depth_buffer;

struct disable_depth_buffer_t
{
};

static constexpr disable_depth_buffer_t disable_depth_buffer;

class Renderer
{
    bool depthBufferEnabled = true;
public:
    Renderer & operator <<(const Mesh &m);
    Renderer & operator <<(TransformedMesh m)
    {
        return *this << (Mesh)m;
    }
    Renderer & operator <<(ColorizedMesh m)
    {
        return *this << (Mesh)m;
    }
    Renderer & operator <<(ColorizedTransformedMesh m)
    {
        return *this << (Mesh)m;
    }
    Renderer & operator <<(TransformedMeshRef m)
    {
        return *this << (Mesh)m;
    }
    Renderer & operator <<(ColorizedMeshRef m)
    {
        return *this << (Mesh)m;
    }
    Renderer & operator <<(ColorizedTransformedMeshRef m)
    {
        return *this << (Mesh)m;
    }
    Renderer & operator <<(shared_ptr<Mesh> m)
    {
        assert(m != nullptr);
        operator <<(*m);
        return *this;
    }
    Renderer & operator <<(enable_depth_buffer_t)
    {
        depthBufferEnabled = true;
        return *this;
    }
    Renderer & operator <<(disable_depth_buffer_t)
    {
        depthBufferEnabled = false;
        return *this;
    }
    Renderer & operator <<(RenderLayer rl)
    {
        switch(rl)
        {
        case RenderLayer::Opaque:
            depthBufferEnabled = true;
            return *this;
        case RenderLayer::Translucent:
            depthBufferEnabled = false;
            return *this;
        }
        assert(false);
        return *this;
    }
    Renderer & operator <<(shared_ptr<CachedMesh> m)
    {
        Display::render(m, depthBufferEnabled);
        return *this;
    }
    shared_ptr<CachedMesh> cacheMesh(const Mesh & m)
    {
        return makeCachedMesh(m);
    }
};

#endif // RENDERER_H_INCLUDED
