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
#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "render/triangle.h"
#include "util/matrix.h"
#include "texture/image.h"
#include "stream/stream.h"
#include "render/render_layer.h"
#include <vector>
#include <cassert>
#include <utility>
#include <memory>

namespace programmerjake
{
namespace voxels
{
struct Mesh;

struct TransformedMesh
{
    Matrix tform;
    std::shared_ptr<Mesh> mesh;
    TransformedMesh(Matrix tform, std::shared_ptr<Mesh> mesh)
        : tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedMesh
{
    ColorF color;
    std::shared_ptr<Mesh> mesh;
    ColorizedMesh(ColorF color, std::shared_ptr<Mesh> mesh)
        : color(color), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedTransformedMesh
{
    ColorF color;
    Matrix tform;
    std::shared_ptr<Mesh> mesh;
    ColorizedTransformedMesh(ColorF color, Matrix tform, std::shared_ptr<Mesh> mesh)
        : color(color), tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator std::shared_ptr<Mesh>() const;
};

struct TransformedMeshRef
{
    Matrix tform;
    const Mesh &mesh;
    TransformedMeshRef(Matrix tform, const Mesh &mesh)
        : tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedMeshRef
{
    ColorF color;
    const Mesh &mesh;
    ColorizedMeshRef(ColorF color, const Mesh &mesh)
        : color(color), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedTransformedMeshRef
{
    ColorF color;
    Matrix tform;
    const Mesh &mesh;
    ColorizedTransformedMeshRef(ColorF color, Matrix tform, const Mesh &mesh)
        : color(color), tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() const;
};

struct TransformedMeshRRef
{
    Matrix tform;
    Mesh &mesh;
    TransformedMeshRRef(Matrix tform, Mesh &&mesh)
        : tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() &&;
};

struct ColorizedMeshRRef
{
    ColorF color;
    Mesh &mesh;
    ColorizedMeshRRef(ColorF color, Mesh &&mesh)
        : color(color), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() &&;
};

struct ColorizedTransformedMeshRRef
{
    ColorF color;
    Matrix tform;
    Mesh &mesh;
    ColorizedTransformedMeshRRef(ColorF color, Matrix tform, Mesh &&mesh)
        : color(color), tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() &&;
};

struct Mesh
{
    std::vector<Triangle> triangles;
    Image image;
    std::size_t size() const
    {
        return triangles.size();
    }
    Mesh(std::vector<Triangle> triangles = std::vector<Triangle>(), Image image = nullptr)
        : triangles(std::move(triangles)), image(image)
    {
    }
    explicit Mesh(std::shared_ptr<Mesh> rt)
        : Mesh(*rt)
    {
    }
    Mesh(const Mesh &) = default;
    Mesh(Mesh &&) = default;
    Mesh(const Mesh & rt, Matrix tform)
        : triangles(), image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&tform](const Triangle & t)->Triangle
        {
            return transform(tform, t);
        });
    }
    Mesh(const Mesh & rt, ColorF color)
        : triangles(), image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color](const Triangle & t)->Triangle
        {
            return colorize(color, t);
        });
    }
    Mesh(const Mesh & rt, ColorF color, Matrix tform)
        : triangles(), image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color, &tform](const Triangle & t)->Triangle
        {
            return colorize(color, transform(tform, t));
        });
    }
    Mesh(Mesh &&rt, Matrix tform)
        : triangles(), image(std::move(rt.image))
    {
        triangles = std::move(rt.triangles);
        for(Triangle &t : triangles)
        {
            t = transform(tform, t);
        }
    }
    Mesh(Mesh &&rt, ColorF color)
        : triangles(), image(std::move(rt.image))
    {
        triangles = std::move(rt.triangles);
        for(Triangle &t : triangles)
        {
            t = colorize(color, t);
        }
    }
    Mesh(Mesh &&rt, ColorF color, Matrix tform)
        : triangles(), image(std::move(rt.image))
    {
        triangles = std::move(rt.triangles);
        for(Triangle &t : triangles)
        {
            t = colorize(color, transform(tform, t));
        }
    }
    Mesh(TransformedMesh mesh)
        : Mesh(*mesh.mesh, mesh.tform)
    {
    }
    Mesh(ColorizedMesh mesh)
        : Mesh(*mesh.mesh, mesh.color)
    {
    }
    Mesh(ColorizedTransformedMesh mesh)
        : Mesh(*mesh.mesh, mesh.color, mesh.tform)
    {
    }
    Mesh(TransformedMeshRef mesh)
        : Mesh(mesh.mesh, mesh.tform)
    {
    }
    Mesh(ColorizedMeshRef mesh)
        : Mesh(mesh.mesh, mesh.color)
    {
    }
    Mesh(ColorizedTransformedMeshRef mesh)
        : Mesh(mesh.mesh, mesh.color, mesh.tform)
    {
    }
    Mesh(TransformedMeshRRef &&mesh)
        : Mesh(std::move(mesh.mesh), mesh.tform)
    {
    }
    Mesh(ColorizedMeshRRef &&mesh)
        : Mesh(std::move(mesh.mesh), mesh.color)
    {
    }
    Mesh(ColorizedTransformedMeshRRef &&mesh)
        : Mesh(std::move(mesh.mesh), mesh.color, mesh.tform)
    {
    }
    Mesh &operator =(const Mesh &rt) = default;
    Mesh &operator =(Mesh &&rt) = default;
    Mesh &operator =(TransformedMesh mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator =(ColorizedMesh mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator =(ColorizedTransformedMesh mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator =(TransformedMeshRef mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator =(ColorizedMeshRef mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator =(ColorizedTransformedMeshRef mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator =(TransformedMeshRRef &&mesh)
    {
        return *this = Mesh(std::move(mesh));
    }
    Mesh &operator =(ColorizedMeshRRef &&mesh)
    {
        return *this = Mesh(std::move(mesh));
    }
    Mesh &operator =(ColorizedTransformedMeshRRef &&mesh)
    {
        return *this = Mesh(std::move(mesh));
    }
    void append(const Mesh & rt)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.insert(triangles.end(), rt.triangles.begin(), rt.triangles.end());
    }
    void append(const Mesh & rt, Matrix tform)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.reserve(triangles.size() + rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&tform](const Triangle & t)->Triangle
        {
            return transform(tform, t);
        });
    }
    void append(const Mesh & rt, ColorF color)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.reserve(triangles.size() + rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color](const Triangle & t)->Triangle
        {
            return colorize(color, t);
        });
    }
    void append(const Mesh & rt, ColorF color, Matrix tform)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.reserve(triangles.size() + rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color, &tform](const Triangle & t)->Triangle
        {
            return colorize(color, transform(tform, t));
        });
    }
    void append(std::shared_ptr<Mesh> rt)
    {
        append(*rt);
    }
    void append(Triangle triangle)
    {
        triangles.push_back(triangle);
    }
    void append(TransformedMesh mesh)
    {
        append(*mesh.mesh, mesh.tform);
    }
    void append(ColorizedMesh mesh)
    {
        append(*mesh.mesh, mesh.color);
    }
    void append(ColorizedTransformedMesh mesh)
    {
        append(*mesh.mesh, mesh.color, mesh.tform);
    }
    void append(TransformedMeshRef mesh)
    {
        append(mesh.mesh, mesh.tform);
    }
    void append(ColorizedMeshRef mesh)
    {
        append(mesh.mesh, mesh.color);
    }
    void append(ColorizedTransformedMeshRef mesh)
    {
        append(mesh.mesh, mesh.color, mesh.tform);
    }
    void append(TransformedMeshRRef &&mesh)
    {
        append(mesh.mesh, mesh.tform);
    }
    void append(ColorizedMeshRRef &&mesh)
    {
        append(mesh.mesh, mesh.color);
    }
    void append(ColorizedTransformedMeshRRef &&mesh)
    {
        append(mesh.mesh, mesh.color, mesh.tform);
    }
    void clear()
    {
        triangles.clear();
        image = nullptr;
    }
    static Mesh read(stream::Reader &reader)
    {
        std::uint32_t triangleCount = stream::read<std::uint32_t>(reader);
        std::vector<Triangle> triangles;
        triangles.reserve(triangleCount);
        for(std::uint32_t i = 0; i < triangleCount; i++)
        {
            triangles.push_back(static_cast<Triangle>(stream::read<Triangle>(reader)));
        }
        Image image = stream::read<Image>(reader);
        return Mesh(triangles, image);
    }
    void write(stream::Writer &writer) const
    {
        std::uint32_t triangleCount = triangles.size();
        assert(triangleCount == triangles.size());
        stream::write<std::uint32_t>(writer, triangleCount);
        for(Triangle tri : triangles)
        {
            stream::write<Triangle>(writer, tri);
        }
        stream::write<Image>(writer, image);
    }
    bool operator ==(const Mesh &rt) const
    {
        if(image != rt.image)
            return false;
        if(size() != rt.size())
            return false;
        for(std::size_t i = 0; i < triangles.size(); i++)
            if(triangles[i] != rt.triangles[i])
                return false;
        return true;
    }
    bool operator !=(const Mesh &rt) const
    {
        return !operator ==(rt);
    }
};

inline TransformedMesh::operator std::shared_ptr<Mesh>() const
{
    return std::shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedMesh::operator std::shared_ptr<Mesh>() const
{
    return std::shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedTransformedMesh::operator std::shared_ptr<Mesh>() const
{
    return std::shared_ptr<Mesh>(new Mesh(*this));
}


inline TransformedMeshRef::operator std::shared_ptr<Mesh>() const
{
    return std::shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedMeshRef::operator std::shared_ptr<Mesh>() const
{
    return std::shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedTransformedMeshRef::operator std::shared_ptr<Mesh>() const
{
    return std::shared_ptr<Mesh>(new Mesh(*this));
}


inline TransformedMeshRRef::operator std::shared_ptr<Mesh>() &&
{
    return std::shared_ptr<Mesh>(new Mesh(std::move(*this)));
}

inline ColorizedMeshRRef::operator std::shared_ptr<Mesh>() &&
{
    return std::shared_ptr<Mesh>(new Mesh(std::move(*this)));
}

inline ColorizedTransformedMeshRRef::operator std::shared_ptr<Mesh>() &&
{
    return std::shared_ptr<Mesh>(new Mesh(std::move(*this)));
}


inline TransformedMeshRef transform(Matrix tform, const Mesh &mesh)
{
    return TransformedMeshRef(tform, mesh);
}

inline TransformedMeshRRef transform(Matrix tform, Mesh &&mesh)
{
    return TransformedMeshRRef(tform, std::move(mesh));
}

inline TransformedMesh transform(Matrix tform, std::shared_ptr<Mesh> mesh)
{
    assert(mesh != nullptr);
    return TransformedMesh(tform, mesh);
}

inline TransformedMesh transform(Matrix tform, TransformedMesh mesh)
{
    return TransformedMesh(transform(tform, mesh.tform), mesh.mesh);
}

inline TransformedMeshRef transform(Matrix tform, const TransformedMeshRef &mesh)
{
    return TransformedMeshRef(transform(tform, mesh.tform), mesh.mesh);
}

inline TransformedMeshRRef transform(Matrix tform, TransformedMeshRRef &&mesh)
{
    return TransformedMeshRRef(transform(tform, mesh.tform), std::move(mesh.mesh));
}

inline ColorizedTransformedMesh transform(Matrix tform, ColorizedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(Matrix tform, const ColorizedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMeshRRef transform(Matrix tform, ColorizedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(mesh.color, tform, std::move(mesh.mesh));
}

inline ColorizedTransformedMesh transform(Matrix tform, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(Matrix tform, const ColorizedTransformedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMeshRRef transform(Matrix tform, ColorizedTransformedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(mesh.color, transform(tform, mesh.tform), std::move(mesh.mesh));
}

inline ColorizedMeshRef colorize(ColorF color, const Mesh &mesh)
{
    return ColorizedMeshRef(color, mesh);
}

inline ColorizedMeshRRef colorize(ColorF color, Mesh &&mesh)
{
    return ColorizedMeshRRef(color, std::move(mesh));
}

inline ColorizedMesh colorize(ColorF color, std::shared_ptr<Mesh> mesh)
{
    assert(mesh != nullptr);
    return ColorizedMesh(color, mesh);
}

inline ColorizedMesh colorize(ColorF color, ColorizedMesh mesh)
{
    return ColorizedMesh(colorize(color, mesh.color), mesh.mesh);
}

inline ColorizedMeshRef colorize(ColorF color, const ColorizedMeshRef &mesh)
{
    return ColorizedMeshRef(colorize(color, mesh.color), mesh.mesh);
}

inline ColorizedMeshRRef colorize(ColorF color, ColorizedMeshRRef &&mesh)
{
    return ColorizedMeshRRef(colorize(color, mesh.color), std::move(mesh.mesh));
}

inline ColorizedTransformedMesh colorize(ColorF color, TransformedMesh mesh)
{
    return ColorizedTransformedMesh(color, mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef colorize(ColorF color, const TransformedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(color, mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMeshRRef colorize(ColorF color, TransformedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(color, mesh.tform, std::move(mesh.mesh));
}

inline ColorizedTransformedMesh colorize(ColorF color, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(colorize(color, mesh.color), mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef colorize(ColorF color, const ColorizedTransformedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(colorize(color, mesh.color), mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMeshRRef colorize(ColorF color, ColorizedTransformedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(colorize(color, mesh.color), mesh.tform, std::move(mesh.mesh));
}
}
}

#endif // MESH_H_INCLUDED
