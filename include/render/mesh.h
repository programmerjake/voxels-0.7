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
    Transform tform;
    std::shared_ptr<Mesh> mesh;
    TransformedMesh(Transform tform, std::shared_ptr<Mesh> mesh) : tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedMesh
{
    ColorF color;
    std::shared_ptr<Mesh> mesh;
    ColorizedMesh(ColorF color, std::shared_ptr<Mesh> mesh) : color(color), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedTransformedMesh
{
    ColorF color;
    Transform tform;
    std::shared_ptr<Mesh> mesh;
    ColorizedTransformedMesh(ColorF color, Transform tform, std::shared_ptr<Mesh> mesh)
        : color(color), tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator std::shared_ptr<Mesh>() const;
};

struct TransformedMeshRef
{
    Transform tform;
    const Mesh &mesh;
    TransformedMeshRef(Transform tform, const Mesh &mesh) : tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedMeshRef
{
    ColorF color;
    const Mesh &mesh;
    ColorizedMeshRef(ColorF color, const Mesh &mesh) : color(color), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() const;
};

struct ColorizedTransformedMeshRef
{
    ColorF color;
    Transform tform;
    const Mesh &mesh;
    ColorizedTransformedMeshRef(ColorF color, Transform tform, const Mesh &mesh)
        : color(color), tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() const;
};

struct TransformedMeshRRef
{
    Transform tform;
    Mesh &mesh;
    TransformedMeshRRef(Transform tform, Mesh &&mesh) : tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() && ;
};

struct ColorizedMeshRRef
{
    ColorF color;
    Mesh &mesh;
    ColorizedMeshRRef(ColorF color, Mesh &&mesh) : color(color), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() && ;
};

struct ColorizedTransformedMeshRRef
{
    ColorF color;
    Transform tform;
    Mesh &mesh;
    ColorizedTransformedMeshRRef(ColorF color, Transform tform, Mesh &&mesh)
        : color(color), tform(tform), mesh(mesh)
    {
    }
    operator std::shared_ptr<Mesh>() && ;
};

struct Mesh
{
    std::vector<IndexedTriangle> indexedTriangles;
    std::vector<Vertex> vertices;
    Image image;
    std::size_t triangleCount() const
    {
        return indexedTriangles.size();
    }
    std::size_t vertexCount() const
    {
        return vertices.size();
    }
    bool empty() const
    {
        return indexedTriangles.empty();
    }
    Mesh(std::vector<Triangle> triangles, Image image = nullptr)
        : indexedTriangles(), vertices(), image(image)
    {
        indexedTriangles.reserve(triangles.size());
        const std::size_t vertexCount = triangles.size() * 3;
        assert(vertexCount <= IndexedTriangle::indexMaxValue());
        vertices.reserve(vertexCount);
        for(std::size_t i = 0; i < triangles.size(); i++)
        {
            auto index = static_cast<IndexedTriangle::IndexType>(i * 3);
            indexedTriangles.push_back(IndexedTriangle(index, index + 1, index + 2));
            vertices.push_back(triangles[i].v1());
            vertices.push_back(triangles[i].v2());
            vertices.push_back(triangles[i].v3());
        }
    }
    Mesh(std::vector<IndexedTriangle> indexedTriangles,
         std::vector<Vertex> vertices,
         Image image = nullptr)
        : indexedTriangles(std::move(indexedTriangles)), vertices(std::move(vertices)), image(image)
    {
    }
    explicit Mesh(std::shared_ptr<Mesh> rt) : Mesh(*rt)
    {
    }
    explicit Mesh(Image image = nullptr) : indexedTriangles(), vertices(), image(image)
    {
    }
    Mesh(const Mesh &) = default;
    Mesh(Mesh &&) = default;
    Mesh(const Mesh &rt, const Transform &tform)
        : indexedTriangles(rt.indexedTriangles), vertices(), image(rt.image)
    {
        vertices.reserve(rt.vertices.size());
        std::transform(rt.vertices.begin(),
                       rt.vertices.end(),
                       back_inserter(vertices),
                       [&tform](const Vertex &t) -> Vertex
                       {
                           return transform(tform, t);
                       });
    }
    Mesh(const Mesh &rt, ColorF color) : indexedTriangles(rt.indexedTriangles), image(rt.image)
    {
        vertices.reserve(rt.vertices.size());
        std::transform(rt.vertices.begin(),
                       rt.vertices.end(),
                       back_inserter(vertices),
                       [&color](const Vertex &t) -> Vertex
                       {
                           return colorize(color, t);
                       });
    }
    Mesh(const Mesh &rt, ColorF color, const Transform &tform)
        : indexedTriangles(rt.indexedTriangles), image(rt.image)
    {
        vertices.reserve(rt.vertices.size());
        std::transform(rt.vertices.begin(),
                       rt.vertices.end(),
                       back_inserter(vertices),
                       [&color, &tform](const Vertex &t) -> Vertex
                       {
                           return colorize(color, transform(tform, t));
                       });
    }
    Mesh(Mesh &&rt, const Transform &tform)
        : indexedTriangles(std::move(rt.indexedTriangles)),
          vertices(std::move(rt.vertices)),
          image(std::move(rt.image))
    {
        for(Vertex &v : vertices)
        {
            v = transform(tform, v);
        }
    }
    Mesh(Mesh &&rt, ColorF color)
        : indexedTriangles(std::move(rt.indexedTriangles)),
          vertices(std::move(rt.vertices)),
          image(std::move(rt.image))
    {
        for(Vertex &v : vertices)
        {
            v = colorize(color, v);
        }
    }
    Mesh(Mesh &&rt, ColorF color, const Transform &tform)
        : indexedTriangles(std::move(rt.indexedTriangles)),
          vertices(std::move(rt.vertices)),
          image(std::move(rt.image))
    {
        for(Vertex &v : vertices)
        {
            v = colorize(color, transform(tform, v));
        }
    }
    Mesh(TransformedMesh mesh) : Mesh(*mesh.mesh, mesh.tform)
    {
    }
    Mesh(ColorizedMesh mesh) : Mesh(*mesh.mesh, mesh.color)
    {
    }
    Mesh(ColorizedTransformedMesh mesh) : Mesh(*mesh.mesh, mesh.color, mesh.tform)
    {
    }
    Mesh(TransformedMeshRef mesh) : Mesh(mesh.mesh, mesh.tform)
    {
    }
    Mesh(ColorizedMeshRef mesh) : Mesh(mesh.mesh, mesh.color)
    {
    }
    Mesh(ColorizedTransformedMeshRef mesh) : Mesh(mesh.mesh, mesh.color, mesh.tform)
    {
    }
    Mesh(TransformedMeshRRef &&mesh) : Mesh(std::move(mesh.mesh), mesh.tform)
    {
    }
    Mesh(ColorizedMeshRRef &&mesh) : Mesh(std::move(mesh.mesh), mesh.color)
    {
    }
    Mesh(ColorizedTransformedMeshRRef &&mesh) : Mesh(std::move(mesh.mesh), mesh.color, mesh.tform)
    {
    }
    Mesh &operator=(const Mesh &rt) = default;
    Mesh &operator=(Mesh &&rt) = default;
    Mesh &operator=(TransformedMesh mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator=(ColorizedMesh mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator=(ColorizedTransformedMesh mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator=(TransformedMeshRef mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator=(ColorizedMeshRef mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator=(ColorizedTransformedMeshRef mesh)
    {
        return *this = Mesh(mesh);
    }
    Mesh &operator=(TransformedMeshRRef &&mesh)
    {
        return *this = Mesh(std::move(mesh));
    }
    Mesh &operator=(ColorizedMeshRRef &&mesh)
    {
        return *this = Mesh(std::move(mesh));
    }
    Mesh &operator=(ColorizedTransformedMeshRRef &&mesh)
    {
        return *this = Mesh(std::move(mesh));
    }
    bool isAppendable(const Mesh &rt) const
    {
        if(rt.vertices.size() + vertices.size() > IndexedTriangle::indexMaxValue())
            return false;
        return rt.image == nullptr || image == nullptr || image == rt.image;
    }
    void append(const Mesh &rt)
    {
        assert(isAppendable(rt));
        if(rt.image != nullptr)
            image = rt.image;
        auto translateOffset = static_cast<IndexedTriangle::IndexType>(vertices.size());
        std::size_t translateStartIndex = indexedTriangles.size();
        indexedTriangles.insert(
            indexedTriangles.end(), rt.indexedTriangles.begin(), rt.indexedTriangles.end());
        vertices.insert(vertices.end(), rt.vertices.begin(), rt.vertices.end());
        for(std::size_t i = translateStartIndex; i < indexedTriangles.size(); i++)
        {
            indexedTriangles[i] = indexedTriangles[i].offsettedBy(translateOffset);
        }
    }
    void append(const Mesh &rt, const Transform &tform)
    {
        assert(isAppendable(rt));
        if(rt.image != nullptr)
            image = rt.image;
        auto translateOffset = static_cast<IndexedTriangle::IndexType>(vertices.size());
        std::size_t translateStartIndex = indexedTriangles.size();
        indexedTriangles.insert(
            indexedTriangles.end(), rt.indexedTriangles.begin(), rt.indexedTriangles.end());
        for(std::size_t i = translateStartIndex; i < indexedTriangles.size(); i++)
        {
            indexedTriangles[i] = indexedTriangles[i].offsettedBy(translateOffset);
        }
        std::transform(rt.vertices.begin(),
                       rt.vertices.end(),
                       back_inserter(vertices),
                       [&tform](const Vertex &t) -> Vertex
                       {
                           return transform(tform, t);
                       });
    }
    void append(const Mesh &rt, ColorF color)
    {
        assert(isAppendable(rt));
        if(rt.image != nullptr)
            image = rt.image;
        auto translateOffset = static_cast<IndexedTriangle::IndexType>(vertices.size());
        std::size_t translateStartIndex = indexedTriangles.size();
        indexedTriangles.insert(
            indexedTriangles.end(), rt.indexedTriangles.begin(), rt.indexedTriangles.end());
        for(std::size_t i = translateStartIndex; i < indexedTriangles.size(); i++)
        {
            indexedTriangles[i] = indexedTriangles[i].offsettedBy(translateOffset);
        }
        std::transform(rt.vertices.begin(),
                       rt.vertices.end(),
                       back_inserter(vertices),
                       [&color](const Vertex &t) -> Vertex
                       {
                           return colorize(color, t);
                       });
    }
    void append(const Mesh &rt, ColorF color, const Transform &tform)
    {
        assert(isAppendable(rt));
        if(rt.image != nullptr)
            image = rt.image;
        auto translateOffset = static_cast<IndexedTriangle::IndexType>(vertices.size());
        std::size_t translateStartIndex = indexedTriangles.size();
        indexedTriangles.insert(
            indexedTriangles.end(), rt.indexedTriangles.begin(), rt.indexedTriangles.end());
        for(std::size_t i = translateStartIndex; i < indexedTriangles.size(); i++)
        {
            indexedTriangles[i] = indexedTriangles[i].offsettedBy(translateOffset);
        }
        std::transform(rt.vertices.begin(),
                       rt.vertices.end(),
                       back_inserter(vertices),
                       [&color, &tform](const Vertex &t) -> Vertex
                       {
                           return colorize(color, transform(tform, t));
                       });
    }
    void append(std::shared_ptr<Mesh> rt)
    {
        append(*rt);
    }
    IndexedTriangle::IndexType addVertex(const Vertex &v)
    {
        assert(vertices.size() < IndexedTriangle::indexMaxValue());
        auto retval = static_cast<IndexedTriangle::IndexType>(vertices.size());
        vertices.push_back(v);
        return retval;
    }
    void append(Triangle triangle)
    {
        auto v0 = addVertex(triangle.v1());
        auto v1 = addVertex(triangle.v2());
        auto v2 = addVertex(triangle.v3());
        indexedTriangles.push_back(IndexedTriangle(v0, v1, v2));
    }
    void addTriangle(IndexedTriangle triangle)
    {
        indexedTriangles.push_back(triangle);
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
        indexedTriangles.clear();
        vertices.clear();
        image = nullptr;
    }
    static Mesh read(stream::Reader &reader)
    {
        std::uint32_t triangleCount = stream::read<std::uint32_t>(reader);
        IndexedTriangle::IndexType vertexCount = stream::read<IndexedTriangle::IndexType>(reader);
        Mesh retval;
        retval.indexedTriangles.reserve(triangleCount);
        retval.vertices.reserve(vertexCount);
        for(std::uint32_t i = 0; i < triangleCount; i++)
        {
            retval.indexedTriangles.push_back(
                static_cast<IndexedTriangle>(stream::read<IndexedTriangle>(reader)));
        }
        for(std::uint32_t i = 0; i < vertexCount; i++)
        {
            retval.vertices.push_back(static_cast<Vertex>(stream::read<Vertex>(reader)));
        }
        retval.image = stream::read<Image>(reader);
        return retval;
    }
    void write(stream::Writer &writer) const
    {
        std::uint32_t triangleCount = this->triangleCount();
        assert(triangleCount == this->triangleCount());
        IndexedTriangle::IndexType vertexCount = this->vertexCount();
        assert(vertexCount == this->vertexCount());
        stream::write<std::uint32_t>(writer, triangleCount);
        stream::write<IndexedTriangle::IndexType>(writer, vertexCount);
        for(const IndexedTriangle &tri : indexedTriangles)
        {
            stream::write<IndexedTriangle>(writer, tri);
        }
        for(const Vertex &vertex : vertices)
        {
            stream::write<Vertex>(writer, vertex);
        }
        stream::write<Image>(writer, image);
    }
    bool operator==(const Mesh &rt) const
    {
        if(image != rt.image)
            return false;
        if(triangleCount() != rt.triangleCount())
            return false;
        if(vertexCount() != rt.vertexCount())
            return false;
        for(std::size_t i = 0; i < indexedTriangles.size(); i++)
            if(indexedTriangles[i] != rt.indexedTriangles[i])
                return false;
        for(std::size_t i = 0; i < vertices.size(); i++)
            if(vertices[i] != rt.vertices[i])
                return false;
        return true;
    }
    bool operator!=(const Mesh &rt) const
    {
        return !operator==(rt);
    }
    friend Mesh optimize(Mesh mesh);
    friend Mesh optimizeNoReorderTriangles(Mesh mesh);
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


inline TransformedMeshRef transform(const Transform &tform, const Mesh &mesh)
{
    return TransformedMeshRef(tform, mesh);
}

inline TransformedMeshRRef transform(const Transform &tform, Mesh &&mesh)
{
    return TransformedMeshRRef(tform, std::move(mesh));
}

inline TransformedMesh transform(const Transform &tform, std::shared_ptr<Mesh> mesh)
{
    assert(mesh != nullptr);
    return TransformedMesh(tform, mesh);
}

inline TransformedMesh transform(const Transform &tform, TransformedMesh mesh)
{
    return TransformedMesh(transform(tform, mesh.tform), mesh.mesh);
}

inline TransformedMeshRef transform(const Transform &tform, const TransformedMeshRef &mesh)
{
    return TransformedMeshRef(transform(tform, mesh.tform), mesh.mesh);
}

inline TransformedMeshRRef transform(const Transform &tform, TransformedMeshRRef &&mesh)
{
    return TransformedMeshRRef(transform(tform, mesh.tform), std::move(mesh.mesh));
}

inline ColorizedTransformedMesh transform(const Transform &tform, ColorizedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(const Transform &tform, const ColorizedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMeshRRef transform(const Transform &tform, ColorizedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(mesh.color, tform, std::move(mesh.mesh));
}

inline ColorizedTransformedMesh transform(const Transform &tform, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(const Transform &tform,
                                             const ColorizedTransformedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMeshRRef transform(const Transform &tform,
                                              ColorizedTransformedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(
        mesh.color, transform(tform, mesh.tform), std::move(mesh.mesh));
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
    return ColorizedTransformedMeshRRef(
        colorize(color, mesh.color), mesh.tform, std::move(mesh.mesh));
}
}
}

#endif // MESH_H_INCLUDED
