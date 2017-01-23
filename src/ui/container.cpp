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
#include "ui/container.h"
#include <vector>

namespace programmerjake
{
namespace voxels
{
namespace ui
{
constexpr std::size_t Container::npos;

void Container::render(Renderer &renderer, float minZ, float maxZ, bool hasFocus)
{
    assert(minZ < maxZ - 1e-5);
    struct MyElementType final
    {
        std::shared_ptr<Element> element;
        std::size_t depth;
        bool hasFocus;
        bool overlaps(const MyElementType &other) const
        {
            if(element->minX >= other.element->maxX || element->maxX <= other.element->minX
               || element->minY >= other.element->maxY
               || element->maxY <= other.element->minY)
                return false;
            return true;
        }
        MyElementType() : element(), depth(), hasFocus()
        {
        }
    };
    std::vector<MyElementType> myElements;
    myElements.reserve(elements.size());
    std::size_t maxDepth = 0;
    for(std::size_t i = 0; i < elements.size(); i++)
    {
        MyElementType e;
        e.element = elements[i];
        e.depth = 0;
        e.hasFocus = (hasFocus && i == currentFocusIndex);
        for(const MyElementType &other : myElements)
        {
            if(e.overlaps(other) && e.depth <= other.depth)
            {
                e.depth = 1 + other.depth;
            }
        }
        if(e.depth > maxDepth)
            maxDepth = e.depth;
        myElements.push_back(std::move(e));
    }
    float logMinZ = std::log(minZ), logMaxZ = std::log(maxZ);
    std::vector<float> depths(maxDepth + 2);
    for(std::size_t i = 0; i < depths.size(); i++)
        depths[i] = std::exp(interpolate(static_cast<float>(i) / (maxDepth + 1), logMaxZ, logMinZ));
    for(const MyElementType &e : myElements)
    {
        float minZ = depths[e.depth + 1];
        float maxZ = depths[e.depth];
        assert(minZ < maxZ - 1e-5);
        e.element->render(renderer, minZ, maxZ, e.hasFocus);
    }
}
}
}
}
