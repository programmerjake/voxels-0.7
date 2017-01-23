/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#include "render/mesh.h"
#include "render/renderer.h"
#include "platform/platform.h"
#include "render/generate.h"
#include <iostream>
#include <cassert>
#include "util/util.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
struct Renderer::Implementation final
{
    Mesh buffer = Mesh();
    RenderLayer bufferRenderLayer = RenderLayer();
    Transform bufferTransform = Transform::identity();
    bool isTransformIdentity = true;
    static bool isBufferSizeBig(std::size_t bufferSize)
    {
        return bufferSize >= 5000;
    }
    static bool isBufferSizeSmall(std::size_t bufferSize)
    {
        return bufferSize < 50;
    }
    void flush()
    {
        if(buffer.triangleCount() != 0)
        {
            Display::render(buffer, bufferTransform.positionMatrix, bufferRenderLayer);
        }
        buffer.clear();
    }
    void render(const Mesh &m, const Transform &tform, RenderLayer rl)
    {
        if(m.triangleCount() == 0)
            return;
        if(buffer.triangleCount() != 0)
        {
            if(!buffer.isAppendable(m) || bufferRenderLayer != rl)
            {
                flush();
            }
            else if(isBufferSizeBig(buffer.triangleCount()))
            {
                if(isTransformIdentity && isBufferSizeSmall(m.triangleCount()))
                {
                    buffer.append(m, tform);
                    flush();
                    return;
                }
                else if(bufferTransform == tform && isBufferSizeSmall(m.triangleCount()))
                {
                    buffer.append(m);
                    flush();
                    return;
                }
                else
                {
                    flush();
                }
            }
            else if(tform == bufferTransform)
            {
                buffer.append(m);
                return;
            }
            else if(isTransformIdentity)
            {
                buffer.append(m, tform);
                return;
            }
            else if(isBufferSizeSmall(buffer.triangleCount()))
            {
                buffer = Mesh(std::move(buffer), bufferTransform);
                bufferTransform = Transform::identity();
                isTransformIdentity = true;
                buffer.append(m, tform);
                return;
            }
            else
            {
                flush();
            }
        }
        assert(buffer.triangleCount() == 0);
        if(isBufferSizeBig(m.triangleCount()))
        {
            Display::render(m, tform.positionMatrix, rl);
            return;
        }
        buffer.append(m);
        bufferTransform = tform;
        bufferRenderLayer = rl;
        isTransformIdentity = (tform == Transform::identity());
    }
};

void Renderer::render(const Mesh &m, const Transform &tform)
{
    implementation->render(m, tform, currentRenderLayer);
}

void Renderer::render(const MeshBuffer &m)
{
    implementation->flush();
    Display::render(m, currentRenderLayer);
}

void Renderer::flush()
{
    implementation->flush();
}

Renderer Renderer::make()
{
    return Renderer(std::make_shared<Implementation>());
}

CutMesh cut(const Mesh &mesh, VectorF planeNormal, float planeD)
{
    Mesh front, coplanar, back;
    front.indexedTriangles.reserve(mesh.triangleCount() * 2);
    coplanar.indexedTriangles.reserve(mesh.triangleCount());
    back.indexedTriangles.reserve(mesh.triangleCount() * 2);
    front.vertices.reserve(mesh.vertices.size());
    coplanar.vertices.reserve(mesh.vertices.size());
    back.vertices.reserve(mesh.vertices.size());
    front.image = mesh.image;
    coplanar.image = mesh.image;
    back.image = mesh.image;
    struct VertexTranslations final
    {
        bool backValid : 1;
        bool coplanarValid : 1;
        bool frontValid : 1;
        IndexedTriangle::IndexType back;
        IndexedTriangle::IndexType coplanar;
        IndexedTriangle::IndexType front;
        constexpr VertexTranslations()
            : backValid(false), coplanarValid(false), frontValid(false), back(), coplanar(), front()
        {
        }
    };
    std::vector<VertexTranslations> vertexTranslations;
    vertexTranslations.resize(mesh.vertices.size());
    struct NewVertex final
    {
        bool backValid : 1;
        bool frontValid : 1;
        IndexedTriangle::IndexType back;
        IndexedTriangle::IndexType front;
        Vertex vertex;
        constexpr NewVertex(Vertex vertex)
            : backValid(false), frontValid(false), back(), front(), vertex(vertex)
        {
        }
    };
    std::vector<NewVertex> newVertices;
    IndexedTriangle::IndexType backVertices[CutTriangle::resultMaxVertexCount];
    std::size_t backVerticesCount = 0;
    IndexedTriangle::IndexType frontVertices[CutTriangle::resultMaxVertexCount];
    std::size_t frontVerticesCount = 0;
    auto addBack = [&](std::size_t index)
    {
        if(index < vertexTranslations.size())
        {
            auto &vertex = vertexTranslations[index];
            if(!vertex.backValid)
            {
                vertex.backValid = true;
                vertex.back = back.addVertex(mesh.vertices[index]);
            }
            backVertices[backVerticesCount++] = vertex.back;
        }
        else
        {
            auto &vertex = newVertices[index];
            if(!vertex.backValid)
            {
                vertex.backValid = true;
                vertex.back = back.addVertex(vertex.vertex);
            }
            backVertices[backVerticesCount++] = vertex.back;
        }
    };
    auto addFront = [&](std::size_t index)
    {
        if(index < vertexTranslations.size())
        {
            auto &vertex = vertexTranslations[index];
            if(!vertex.frontValid)
            {
                vertex.frontValid = true;
                vertex.front = front.addVertex(mesh.vertices[index]);
            }
            frontVertices[frontVerticesCount++] = vertex.front;
        }
        else
        {
            auto &vertex = newVertices[index - vertexTranslations.size()];
            if(!vertex.frontValid)
            {
                vertex.frontValid = true;
                vertex.front = front.addVertex(vertex.vertex);
            }
            frontVertices[frontVerticesCount++] = vertex.front;
        }
    };
    auto getVertexPosition = [&](std::size_t index)
    {
        assert(index < mesh.vertices.size());
        return mesh.vertices[index].p;
    };
    auto createInterpolated = [&](float t, std::size_t aIndex, std::size_t bIndex)
    {
        assert(aIndex < mesh.vertices.size());
        assert(bIndex < mesh.vertices.size());
        auto vertex = interpolate(t, mesh.vertices[aIndex], mesh.vertices[bIndex]);
        std::size_t retval = newVertices.size() + vertexTranslations.size();
        newVertices.emplace_back(vertex);
        return retval;
    };
    for(IndexedTriangle tri : mesh.indexedTriangles)
    {
        backVerticesCount = 0;
        frontVerticesCount = 0;
        bool isCoplanar = CutTriangle::cutTriangleAlgorithm<std::size_t>(planeNormal,
                                                                         planeD,
                                                                         tri.v[0],
                                                                         tri.v[1],
                                                                         tri.v[2],
                                                                         getVertexPosition,
                                                                         addBack,
                                                                         addFront,
                                                                         createInterpolated);
        if(isCoplanar)
        {
            IndexedTriangle resultTriangle;
            std::size_t resultTriangleVertexIndex = 0;
            for(auto index : tri.v)
            {
                auto &vertex = vertexTranslations[index];
                if(!vertex.coplanarValid)
                {
                    vertex.coplanarValid = true;
                    vertex.coplanar = coplanar.addVertex(mesh.vertices[index]);
                }
                resultTriangle.v[resultTriangleVertexIndex++] = vertex.coplanar;
            }
            coplanar.addTriangle(resultTriangle);
            continue;
        }
        IndexedTriangle frontTriangles[CutTriangle::resultMaxTriangleCount];
        std::size_t frontTriangleCount = 0;
        IndexedTriangle backTriangles[CutTriangle::resultMaxTriangleCount];
        std::size_t backTriangleCount = 0;
        CutTriangle::triangulate(
            frontTriangles, frontTriangleCount, frontVertices, frontVerticesCount);
        CutTriangle::triangulate(backTriangles, backTriangleCount, backVertices, backVerticesCount);
        for(std::size_t i = 0; i < frontTriangleCount; i++)
            front.addTriangle(frontTriangles[i]);
        for(std::size_t i = 0; i < frontTriangleCount; i++)
            back.addTriangle(backTriangles[i]);
    }
    return CutMesh(std::move(front), std::move(coplanar), std::move(back));
}

Mesh cutAndGetFront(Mesh mesh, VectorF planeNormal, float planeD)
{
    IndexedTriangle::IndexType currentVertices[CutTriangle::resultMaxVertexCount];
    std::size_t currentVerticesCount = 0;
    std::vector<IndexedTriangle> originalTriangles;
    originalTriangles.reserve(mesh.indexedTriangles.size());
    originalTriangles.swap(mesh.indexedTriangles);
    auto addVertex = [&](IndexedTriangle::IndexType vertex)
    {
        currentVertices[currentVerticesCount++] = vertex;
    };
    auto addVertexNoOperation = [](IndexedTriangle::IndexType)
    {
    };
    auto getVertexPosition = [&](IndexedTriangle::IndexType vertex)
    {
        assert(vertex < mesh.vertices.size());
        return mesh.vertices[vertex].p;
    };
    auto createInterpolated =
        [&](float t, IndexedTriangle::IndexType a, IndexedTriangle::IndexType b)
    {
        assert(a < mesh.vertices.size());
        assert(b < mesh.vertices.size());
        return mesh.addVertex(interpolate(t, mesh.vertices[a], mesh.vertices[b]));
    };
    for(IndexedTriangle tri : originalTriangles)
    {
        currentVerticesCount = 0;
        bool isCoplanar = CutTriangle::cutTriangleAlgorithm(planeNormal,
                                                            planeD,
                                                            tri.v[0],
                                                            tri.v[1],
                                                            tri.v[2],
                                                            getVertexPosition,
                                                            addVertexNoOperation,
                                                            addVertex,
                                                            createInterpolated);
        if(isCoplanar)
        {
            mesh.addTriangle(tri);
            continue;
        }
        IndexedTriangle currentTriangles[CutTriangle::resultMaxTriangleCount];
        std::size_t currentTriangleCount = 0;
        CutTriangle::triangulate(
            currentTriangles, currentTriangleCount, currentVertices, currentVerticesCount);
        for(std::size_t i = 0; i < currentTriangleCount; i++)
            mesh.addTriangle(currentTriangles[i]);
    }
    return mesh;
}

Mesh cutAndGetBack(Mesh mesh, VectorF planeNormal, float planeD)
{
    IndexedTriangle::IndexType currentVertices[CutTriangle::resultMaxVertexCount];
    std::size_t currentVerticesCount = 0;
    std::vector<IndexedTriangle> originalTriangles;
    originalTriangles.reserve(mesh.indexedTriangles.size());
    originalTriangles.swap(mesh.indexedTriangles);
    auto addVertex = [&](IndexedTriangle::IndexType vertex)
    {
        currentVertices[currentVerticesCount++] = vertex;
    };
    auto addVertexNoOperation = [](IndexedTriangle::IndexType)
    {
    };
    auto getVertexPosition = [&](IndexedTriangle::IndexType vertex)
    {
        assert(vertex < mesh.vertices.size());
        return mesh.vertices[vertex].p;
    };
    auto createInterpolated =
        [&](float t, IndexedTriangle::IndexType a, IndexedTriangle::IndexType b)
    {
        assert(a < mesh.vertices.size());
        assert(b < mesh.vertices.size());
        return mesh.addVertex(interpolate(t, mesh.vertices[a], mesh.vertices[b]));
    };
    for(IndexedTriangle tri : originalTriangles)
    {
        currentVerticesCount = 0;
        bool isCoplanar = CutTriangle::cutTriangleAlgorithm(planeNormal,
                                                            planeD,
                                                            tri.v[0],
                                                            tri.v[1],
                                                            tri.v[2],
                                                            getVertexPosition,
                                                            addVertex,
                                                            addVertexNoOperation,
                                                            createInterpolated);
        if(isCoplanar)
        {
            mesh.addTriangle(tri);
            continue;
        }
        IndexedTriangle currentTriangles[CutTriangle::resultMaxTriangleCount];
        std::size_t currentTriangleCount = 0;
        CutTriangle::triangulate(
            currentTriangles, currentTriangleCount, currentVertices, currentVerticesCount);
        for(std::size_t i = 0; i < currentTriangleCount; i++)
            mesh.addTriangle(currentTriangles[i]);
    }
    return mesh;
}

namespace Generate
{
namespace
{
bool isPixelTransparent(ColorI c)
{
    return c.a < 0x80;
}
}

Mesh item3DImage(TextureDescriptor td, float thickness)
{
    assert(td);
    assert(td.maxU <= 1 && td.minU >= 0 && td.maxV <= 1 && td.minV >= 0);
    Image image = td.image;
    unsigned iw = image.width();
    unsigned ih = image.height();
    float fMinX = iw * td.minU;
    float fMaxX = iw * td.maxU;
    float fMinY = ih * (1 - td.maxV);
    float fMaxY = ih * (1 - td.minV);
    int minX = ifloor(1e-5f + fMinX);
    int maxX = ifloor(-1e-5f + fMaxX);
    int minY = ifloor(1e-5f + fMinY);
    int maxY = ifloor(-1e-5f + fMaxY);
    assert(minX <= maxX && minY <= maxY);
    float scale = std::min<float>(1.0f / (1 + maxX - minX), 1.0f / (1 + maxY - minY));
    if(thickness <= 0)
        thickness = scale;
    TextureDescriptor backTD = td;
    backTD.maxU = td.minU;
    backTD.minU = td.maxU;
    float faceMinX = scale * (fMinX - minX);
    float faceMinY = scale * (maxY - fMaxY + 1);
    float faceMaxX = faceMinX + scale * (fMaxX - fMinX);
    float faceMaxY = faceMinY + scale * (fMaxY - fMinY);
    Mesh retval = transform(
        Transform::translate(faceMinX, faceMinY, -0.5f)
            .concat(Transform::scale(faceMaxX - faceMinX, faceMaxY - faceMinY, thickness)),
        unitBox(TextureDescriptor(),
                TextureDescriptor(),
                TextureDescriptor(),
                TextureDescriptor(),
                backTD,
                td));
    for(int iy = minY; iy <= maxY; iy++)
    {
        for(int ix = minX; ix <= maxX; ix++)
        {
            bool transparent = isPixelTransparent(image.getPixel(ix, iy));
            if(transparent)
                continue;
            bool transparentNIX = true, transparentPIX = true, transparentNIY = true,
                 transparentPIY =
                     true; // image transparent for negative and positive image coordinates
            if(ix > minX)
                transparentNIX = isPixelTransparent(image.getPixel(ix - 1, iy));
            if(ix < maxX)
                transparentPIX = isPixelTransparent(image.getPixel(ix + 1, iy));
            if(iy > minY)
                transparentNIY = isPixelTransparent(image.getPixel(ix, iy - 1));
            if(iy < maxY)
                transparentPIY = isPixelTransparent(image.getPixel(ix, iy + 1));
            float pixelU = (ix + 0.5f) / iw;
            float pixelV = 1.0f - (iy + 0.5f) / ih;
            TextureDescriptor pixelTD = TextureDescriptor(image, pixelU, pixelU, pixelV, pixelV);
            TextureDescriptor nx, px, ny, py;
            if(transparentNIX)
                nx = pixelTD;
            if(transparentPIX)
                px = pixelTD;
            if(transparentNIY)
                py = pixelTD;
            if(transparentPIY)
                ny = pixelTD;
            Transform tform =
                Transform::translate(static_cast<float>(ix - minX),
                                     static_cast<float>(maxY - iy),
                                     -0.5f).concat(Transform::scale(scale, scale, thickness));
            retval.append(transform(
                tform, unitBox(nx, px, ny, py, TextureDescriptor(), TextureDescriptor())));
        }
    }
    return std::move(retval);
}

Mesh itemDamage(float damageValue)
{
    damageValue = limit<float>(damageValue, 0, 1);
    if(damageValue == 0)
        return Mesh();
    TextureDescriptor backgroundTexture = TextureAtlas::DamageBarGray.td();
    TextureDescriptor foregroundTexture = TextureAtlas::DamageBarGreen.td();
    if(damageValue > 1.0f / 3)
    {
        foregroundTexture = TextureAtlas::DamageBarYellow.td();
    }
    if(damageValue > 2.0f / 3)
    {
        foregroundTexture = TextureAtlas::DamageBarRed.td();
    }
    const float minX = 2 / 16.0f;
    const float maxX = 14 / 16.0f;
    float splitX = interpolate(damageValue, maxX, minX);
    const float minY = 2 / 16.0f;
    const float maxY = 4 / 16.0f;
    constexpr ColorF c = colorizeIdentity();
    const VectorF nxny = VectorF(minX, minY, 0);
    VectorF cxny = VectorF(splitX, minY, 0);
    const VectorF pxny = VectorF(maxX, minY, 0);
    const VectorF nxpy = VectorF(minX, maxY, 0);
    VectorF cxpy = VectorF(splitX, maxY, 0);
    const VectorF pxpy = VectorF(maxX, maxY, 0);
    Mesh retval = quadrilateral(foregroundTexture, nxny, c, cxny, c, cxpy, c, nxpy, c);
    retval.append(quadrilateral(backgroundTexture, cxny, c, pxny, c, pxpy, c, cxpy, c));
    return retval;
}
}
}
}
