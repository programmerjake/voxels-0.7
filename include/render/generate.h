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
inline Mesh reverse(const Mesh &mesh)
{
    std::vector<Triangle> triangles = mesh.triangles;
    for(Triangle &tri : triangles)
    {
        using std::swap;
        swap(tri.p1, tri.p2);
        swap(tri.c1, tri.c2);
        swap(tri.t1, tri.t2);
        VectorF n1 = -tri.n2;
        VectorF n2 = -tri.n1;
        VectorF n3 = -tri.n3;
        tri.n1 = n1;
        tri.n2 = n2;
        tri.n3 = n3;
    }
    return Mesh(std::move(triangles), mesh.image);
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

inline Mesh lightMesh(Mesh m, std::function<ColorF(ColorF color, VectorF position, VectorF normal)> lightVertex)
{
    for(Triangle & tri : m.triangles)
    {
        tri.c1 = lightVertex(tri.c1, tri.p1, tri.n1);
        tri.c2 = lightVertex(tri.c2, tri.p2, tri.n2);
        tri.c3 = lightVertex(tri.c3, tri.p3, tri.n3);
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
    CutMesh()
        : front(), coplanar(), back()
    {
    }
};

inline CutMesh cut(const Mesh &mesh, VectorF planeNormal, float planeD)
{
    Mesh front, coplanar, back;
    front.triangles.reserve(mesh.size() * 2);
    coplanar.triangles.reserve(mesh.size());
    back.triangles.reserve(mesh.size() * 2);
    front.image = mesh.image;
    coplanar.image = mesh.image;
    back.image = mesh.image;
    for(Triangle tri : mesh.triangles)
    {
        CutTriangle ct = cut(tri, planeNormal, planeD);
        for(size_t i = 0; i < ct.frontTriangleCount; i++)
            front.append(ct.frontTriangles[i]);
        for(size_t i = 0; i < ct.coplanarTriangleCount; i++)
            coplanar.append(ct.coplanarTriangles[i]);
        for(size_t i = 0; i < ct.backTriangleCount; i++)
            back.append(ct.backTriangles[i]);
    }
    return CutMesh(std::move(front), std::move(coplanar), std::move(back));
}

inline Mesh cutAndGetFront(Mesh mesh, VectorF planeNormal, float planeD)
{
    static thread_local std::vector<Triangle> triangles;
    triangles = std::move(mesh.triangles);
    mesh.triangles.clear();
    mesh.triangles.reserve(triangles.size());
    for(Triangle tri : triangles)
    {
        CutTriangle ct = cut(tri, planeNormal, planeD);
        for(size_t i = 0; i < ct.frontTriangleCount; i++)
            mesh.append(ct.frontTriangles[i]);
        for(size_t i = 0; i < ct.coplanarTriangleCount; i++)
            mesh.append(ct.coplanarTriangles[i]);
    }
    return std::move(mesh);
}

inline Mesh cutAndGetBack(Mesh mesh, VectorF planeNormal, float planeD)
{
    static thread_local std::vector<Triangle> triangles;
    triangles = std::move(mesh.triangles);
    mesh.triangles.clear();
    mesh.triangles.reserve(triangles.size());
    for(Triangle tri : triangles)
    {
        CutTriangle ct = cut(tri, planeNormal, planeD);
        for(size_t i = 0; i < ct.backTriangleCount; i++)
            mesh.append(ct.backTriangles[i]);
        for(size_t i = 0; i < ct.coplanarTriangleCount; i++)
            mesh.append(ct.coplanarTriangles[i]);
    }
    return std::move(mesh);
}

namespace Generate
{
	inline Mesh quadrilateral(TextureDescriptor texture, VectorF p1, ColorF c1, VectorF p2, ColorF c2, VectorF p3, ColorF c3, VectorF p4, ColorF c4)
	{
		const TextureCoord t1 = TextureCoord(texture.minU, texture.minV);
		const TextureCoord t2 = TextureCoord(texture.maxU, texture.minV);
		const TextureCoord t3 = TextureCoord(texture.maxU, texture.maxV);
		const TextureCoord t4 = TextureCoord(texture.minU, texture.maxV);
		return Mesh(std::vector<Triangle>{Triangle(p1, c1, t1, p2, c2, t2, p3, c3, t3), Triangle(p3, c3, t3, p4, c4, t4, p1, c1, t1)}, texture.image);
	}

	/// make a box from <0, 0, 0> to <1, 1, 1>
	inline Mesh unitBox(TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz)
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
		retval.triangles.reserve(12);
		constexpr ColorF c = colorizeIdentity();
		if(nx)
		{
			retval.append(quadrilateral(nx,
									 p0, c,
									 p4, c,
									 p6, c,
									 p2, c
									 ));
		}
		if(px)
		{
			retval.append(quadrilateral(px,
									 p5, c,
									 p1, c,
									 p3, c,
									 p7, c
									 ));
		}
		if(ny)
		{
			retval.append(quadrilateral(ny,
									 p0, c,
									 p1, c,
									 p5, c,
									 p4, c
									 ));
		}
		if(py)
		{
			retval.append(quadrilateral(py,
									 p6, c,
									 p7, c,
									 p3, c,
									 p2, c
									 ));
		}
		if(nz)
		{
			retval.append(quadrilateral(nz,
									 p1, c,
									 p0, c,
									 p2, c,
									 p3, c
									 ));
		}
		if(pz)
		{
			retval.append(quadrilateral(pz,
									 p4, c,
									 p5, c,
									 p7, c,
									 p6, c
									 ));
		}
		return retval;
	}

	Mesh item3DImage(TextureDescriptor td, float thickness = -1);
	Mesh itemDamage(float damageValue);
}
}
}

#endif // GENERATE_H_INCLUDED
