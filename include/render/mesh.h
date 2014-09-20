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
#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "render/triangle.h"
#include "util/matrix.h"
#include "texture/image.h"
#include "stream/stream.h"
#include "util/variable_set.h"
#include "render/render_layer.h"
#include <vector>
#include <cassert>
#include <utility>
#include <memory>

using namespace std;

struct Mesh;

struct TransformedMesh
{
    Matrix tform;
    shared_ptr<Mesh> mesh;
    TransformedMesh(Matrix tform, shared_ptr<Mesh> mesh)
        : tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct ColorizedMesh
{
    ColorF color;
    shared_ptr<Mesh> mesh;
    ColorizedMesh(ColorF color, shared_ptr<Mesh> mesh)
        : color(color), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct ColorizedTransformedMesh
{
    ColorF color;
    Matrix tform;
    shared_ptr<Mesh> mesh;
    ColorizedTransformedMesh(ColorF color, Matrix tform, shared_ptr<Mesh> mesh)
        : color(color), tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct TransformedMeshRef
{
    Matrix tform;
    const Mesh &mesh;
    TransformedMeshRef(Matrix tform, const Mesh &mesh)
        : tform(tform), mesh(mesh)
    {
    }
    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
    operator TransformedMesh() const;
};

struct ColorizedMeshRef
{
    ColorF color;
    const Mesh &mesh;
    ColorizedMeshRef(ColorF color, const Mesh &mesh)
        : color(color), mesh(mesh)
    {
    }
    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
    operator ColorizedMesh() const;
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
    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
    operator ColorizedTransformedMesh() const;
};

struct Mesh
{
    vector<Triangle> triangles;
    Image image;
    size_t size() const
    {
        return triangles.size();
    }
    Mesh(vector<Triangle> triangles = vector<Triangle>(), Image image = nullptr)
        : triangles(std::move(triangles)), image(image)
    {
    }
    explicit Mesh(shared_ptr<Mesh> rt)
        : Mesh(*rt)
    {
    }
    Mesh(const Mesh & rt, Matrix tform)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&tform](const Triangle & t)->Triangle
        {
            return transform(tform, t);
        });
    }
    Mesh(const Mesh & rt, ColorF color)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color](const Triangle & t)->Triangle
        {
            return colorize(color, t);
        });
    }
    Mesh(const Mesh & rt, ColorF color, Matrix tform)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color, &tform](const Triangle & t)->Triangle
        {
            return colorize(color, transform(tform, t));
        });
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
    void append(shared_ptr<Mesh> rt)
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
    void clear()
    {
        triangles.clear();
        image = nullptr;
    }
    static shared_ptr<Mesh> read(stream::Reader &reader, VariableSet &variableSet)
    {
        uint32_t triangleCount = stream::read<uint32_t>(reader);
        vector<Triangle> triangles;
        triangles.reserve(triangleCount);
        for(uint32_t i = 0; i < triangleCount; i++)
        {
            triangles.push_back((Triangle)stream::read<Triangle>(reader));
        }
        Image image = stream::read<Image>(reader, variableSet);
        return make_shared<Mesh>(triangles, image);
    }
    void write(stream::Writer &writer, VariableSet &variableSet) const
    {
        uint32_t triangleCount = triangles.size();
        assert(triangleCount == triangles.size());
        stream::write<uint32_t>(writer, triangleCount);
        for(Triangle tri : triangles)
        {
            stream::write<Triangle>(writer, tri);
        }
        stream::write<Image>(writer, variableSet, image);
    }
};

inline TransformedMesh::operator Mesh() const
{
    return Mesh(*this);
}

inline TransformedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedMesh::operator Mesh() const
{
    return Mesh(*this);
}

inline ColorizedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedTransformedMesh::operator Mesh() const
{
    return Mesh(*this);
}

inline ColorizedTransformedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline TransformedMeshRef::operator Mesh() const
{
    return Mesh(*this);
}

inline TransformedMeshRef::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline TransformedMeshRef::operator TransformedMesh() const
{
    return TransformedMesh(tform, shared_ptr<Mesh>(new Mesh(mesh)));
}

inline ColorizedMeshRef::operator Mesh() const
{
    return Mesh(*this);
}

inline ColorizedMeshRef::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedMeshRef::operator ColorizedMesh() const
{
    return ColorizedMesh(color, shared_ptr<Mesh>(new Mesh(mesh)));
}

inline ColorizedTransformedMeshRef::operator Mesh() const
{
    return Mesh(*this);
}

inline ColorizedTransformedMeshRef::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedTransformedMeshRef::operator ColorizedTransformedMesh() const
{
    return ColorizedTransformedMesh(color, tform, shared_ptr<Mesh>(new Mesh(mesh)));
}

inline TransformedMeshRef transform(Matrix tform, const Mesh &mesh)
{
    return TransformedMeshRef(tform, mesh);
}

inline TransformedMesh transform(Matrix tform, shared_ptr<Mesh> mesh)
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

inline ColorizedTransformedMesh transform(Matrix tform, ColorizedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(Matrix tform, const ColorizedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMesh transform(Matrix tform, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(Matrix tform, const ColorizedTransformedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedMeshRef colorize(ColorF color, const Mesh &mesh)
{
    return ColorizedMeshRef(color, mesh);
}

inline ColorizedMesh colorize(ColorF color, shared_ptr<Mesh> mesh)
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

inline ColorizedTransformedMesh colorize(ColorF color, TransformedMesh mesh)
{
    return ColorizedTransformedMesh(color, mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef colorize(ColorF color, const TransformedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(color, mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMesh colorize(ColorF color, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(colorize(color, mesh.color), mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef colorize(ColorF color, const ColorizedTransformedMeshRef &mesh)
{
    return ColorizedTransformedMeshRef(colorize(color, mesh.color), mesh.tform, mesh.mesh);
}

#endif // MESH_H_INCLUDED
