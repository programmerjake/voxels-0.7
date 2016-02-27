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
#ifndef GENERATE_H_INCLUDED
#define GENERATE_H_INCLUDED

#include "render/mesh.h"
#include "texture/texture_descriptor.h"
#include <utility>
#include <functional>

namespace programmerjake
{
namespace voxels
{
inline Mesh reverse(Mesh mesh)
{
    for(IndexedTriangle &tri : mesh.indexedTriangles)
    {
        tri = reverse(tri);
    }
    for(Vertex &v : mesh.vertices)
    {
        v = reverse(v);
    }
    return mesh;
}

inline Mesh &&reverse(Mesh &&mesh)
{
    for(IndexedTriangle &tri : mesh.indexedTriangles)
    {
        tri = reverse(tri);
    }
    for(Vertex &v : mesh.vertices)
    {
        v = reverse(v);
    }
    return std::move(mesh);
}

inline std::shared_ptr<Mesh> reverse(std::shared_ptr<Mesh> mesh)
{
    if(!mesh)
        return nullptr;
    return std::make_shared<Mesh>(reverse(*mesh));
}

inline TransformedMesh reverse(TransformedMesh mesh)
{
    mesh.mesh = reverse(mesh.mesh);
    return mesh;
}

inline ColorizedTransformedMesh reverse(ColorizedTransformedMesh mesh)
{
    mesh.mesh = reverse(mesh.mesh);
    return mesh;
}

inline ColorizedMesh reverse(ColorizedMesh mesh)
{
    mesh.mesh = reverse(mesh.mesh);
    return mesh;
}

inline TransformedMeshRRef reverse(TransformedMeshRRef &&mesh)
{
    reverse(mesh.mesh);
    return std::move(mesh);
}

inline ColorizedTransformedMeshRRef reverse(ColorizedTransformedMeshRRef &&mesh)
{
    reverse(mesh.mesh);
    return std::move(mesh);
}

inline ColorizedMeshRRef reverse(ColorizedMeshRRef &&mesh)
{
    reverse(mesh.mesh);
    return std::move(mesh);
}

template <typename Fn>
inline Mesh lightMesh(
    Mesh m, Fn &lightVertex)
{
    for(Vertex &v : m.vertices)
    {
        ColorF color = v.c;
        VectorF position = v.p;
        VectorF normal = v.n;
        v.c = lightVertex(color, position, normal);
    }
    return m;
}

struct CutMesh
{
    Mesh front, coplanar, back;
    CutMesh(Mesh front, Mesh coplanar, Mesh back)
        : front(std::move(front)), coplanar(std::move(coplanar)), back(std::move(back))
    {
    }
    CutMesh() : front(), coplanar(), back()
    {
    }
};

CutMesh cut(const Mesh &mesh, VectorF planeNormal, float planeD); // in mesh.cpp
Mesh cutAndGetFront(Mesh mesh, VectorF planeNormal, float planeD); // in mesh.cpp
Mesh cutAndGetBack(Mesh mesh, VectorF planeNormal, float planeD); // in mesh.cpp

namespace Generate
{
inline Mesh quadrilateral(TextureDescriptor texture,
                          VectorF p1,
                          ColorF c1,
                          VectorF p2,
                          ColorF c2,
                          VectorF p3,
                          ColorF c3,
                          VectorF p4,
                          ColorF c4)
{
    const TextureCoord t1 = TextureCoord(texture.minU, texture.minV);
    const TextureCoord t2 = TextureCoord(texture.maxU, texture.minV);
    const TextureCoord t3 = TextureCoord(texture.maxU, texture.maxV);
    const TextureCoord t4 = TextureCoord(texture.minU, texture.maxV);
    const VectorF normal = Triangle(p1, p2, p3).n1;
    Mesh retval(texture.image);
    auto v1 = retval.addVertex(Vertex(t1, p1, c1, normal));
    auto v2 = retval.addVertex(Vertex(t2, p2, c2, normal));
    auto v3 = retval.addVertex(Vertex(t3, p3, c3, normal));
    auto v4 = retval.addVertex(Vertex(t4, p4, c4, normal));
    retval.addTriangle(IndexedTriangle(v1, v2, v3));
    retval.addTriangle(IndexedTriangle(v3, v4, v1));
    return retval;
}

/// make a box from <0, 0, 0> to <1, 1, 1>
inline Mesh unitBox(TextureDescriptor nx,
                    TextureDescriptor px,
                    TextureDescriptor ny,
                    TextureDescriptor py,
                    TextureDescriptor nz,
                    TextureDescriptor pz)
{
    const VectorF p0 = VectorF(0, 0, 0);
    const VectorF p1 = VectorF(1, 0, 0);
    const VectorF p2 = VectorF(0, 1, 0);
    const VectorF p3 = VectorF(1, 1, 0);
    const VectorF p4 = VectorF(0, 0, 1);
    const VectorF p5 = VectorF(1, 0, 1);
    const VectorF p6 = VectorF(0, 1, 1);
    const VectorF p7 = VectorF(1, 1, 1);
    Mesh retval;
    constexpr ColorF c = colorizeIdentity();
    if(nx)
    {
        retval.append(quadrilateral(nx, p0, c, p4, c, p6, c, p2, c));
    }
    if(px)
    {
        retval.append(quadrilateral(px, p5, c, p1, c, p3, c, p7, c));
    }
    if(ny)
    {
        retval.append(quadrilateral(ny, p0, c, p1, c, p5, c, p4, c));
    }
    if(py)
    {
        retval.append(quadrilateral(py, p6, c, p7, c, p3, c, p2, c));
    }
    if(nz)
    {
        retval.append(quadrilateral(nz, p1, c, p0, c, p2, c, p3, c));
    }
    if(pz)
    {
        retval.append(quadrilateral(pz, p4, c, p5, c, p7, c, p6, c));
    }
    return retval;
}

Mesh item3DImage(TextureDescriptor td, float thickness = -1);
Mesh itemDamage(float damageValue);
}
}
}

#endif // GENERATE_H_INCLUDED
