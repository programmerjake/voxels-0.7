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
#ifndef BIOME_H_INCLUDED
#define BIOME_H_INCLUDED

#include <atomic>
#include "util/linked_map.h"
#include "util/color.h"
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#include <tuple>
#include "util/util.h"
#include "util/position.h"
#include "block/block_struct.h"
#include "stream/stream.h"

namespace programmerjake
{
namespace voxels
{
class BiomeDescriptor;
class RandomSource;
typedef const BiomeDescriptor *BiomeDescriptorPointer;
typedef std::size_t BiomeIndex;

BiomeIndex getBiomeIndex(BiomeDescriptorPointer);

class BiomeWeights;

class BiomeDescriptors_t final
{
    friend class BiomeDescriptor;

private:
    static linked_map<std::wstring, BiomeDescriptorPointer> *pBiomeNameMap;
    static std::vector<BiomeDescriptorPointer> *pBiomeVector;
    static void addBiome(BiomeDescriptor *biome);

public:
    std::size_t size() const
    {
        if(pBiomeVector == nullptr)
            return 0;
        return pBiomeVector->size();
    }
    typedef std::vector<BiomeDescriptorPointer>::const_iterator iterator;
    iterator begin() const
    {
        assert(pBiomeVector != nullptr);
        return pBiomeVector->cbegin();
    }
    iterator end() const
    {
        assert(pBiomeVector != nullptr);
        return pBiomeVector->cend();
    }
    iterator find(BiomeIndex bi) const
    {
        assert(bi < size());
        return begin() + bi;
    }
    iterator find(BiomeDescriptorPointer biome) const
    {
        if(biome == nullptr)
            return end();
        return find(getBiomeIndex(biome));
    }
    iterator find(std::wstring name) const
    {
        assert(pBiomeVector != nullptr && pBiomeNameMap != nullptr);
        auto iter = pBiomeNameMap->find(name);
        if(iter == pBiomeNameMap->end())
            return end();
        BiomeDescriptorPointer biome = std::get<1>(*iter);
        return find(biome);
    }
    BiomeDescriptorPointer operator[](BiomeIndex bi) const
    {
        return *find(bi);
    }
    BiomeDescriptorPointer operator[](std::wstring name) const
    {
        iterator iter = find(name);
        if(iter == end())
            return nullptr;
        return *iter;
    }
    BiomeWeights getBiomeWeights(float temperature,
                                 float humidity,
                                 PositionI pos,
                                 RandomSource &randomSource) const;
    void writeDescriptor(stream::Writer &writer, BiomeDescriptorPointer descriptor) const;
    BiomeDescriptorPointer readDescriptor(stream::Reader &reader) const;
};

static constexpr BiomeDescriptors_t BiomeDescriptors;

template <typename T>
class BiomeMap
{
public:
    typedef BiomeDescriptorPointer key_type;
    typedef T mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;

private:
    typedef std::vector<value_type> ElementsType;

public:
    typedef typename ElementsType::iterator iterator;
    typedef typename ElementsType::const_iterator const_iterator;
    typedef typename ElementsType::size_type size_type;
    typedef typename ElementsType::difference_type difference_type;

private:
    ElementsType elements;
    static std::size_t getIndex(BiomeDescriptorPointer biome)
    {
        if(biome == nullptr)
            return ~static_cast<std::size_t>(0);
        return getBiomeIndex(biome);
    }

public:
    BiomeMap() : elements()
    {
        elements.reserve(BiomeDescriptors.size());
        for(BiomeDescriptorPointer v : BiomeDescriptors)
        {
            elements.emplace_back(
                std::piecewise_construct, std::tuple<BiomeDescriptorPointer>(v), std::tuple<>());
        }
    }
    BiomeMap(const BiomeMap &rt) : elements()
    {
        elements.reserve(BiomeDescriptors.size());
        for(BiomeDescriptorPointer v : BiomeDescriptors)
        {
            elements.emplace_back(std::piecewise_construct,
                                  std::tuple<BiomeDescriptorPointer>(v),
                                  std::tuple<const T &>(rt.at(v)));
        }
    }
    BiomeMap(BiomeMap &&rt) : elements()
    {
        elements.reserve(BiomeDescriptors.size());
        for(BiomeDescriptorPointer v : BiomeDescriptors)
        {
            elements.emplace_back(std::piecewise_construct,
                                  std::tuple<BiomeDescriptorPointer>(v),
                                  std::tuple<T &&>(std::move(rt.at(v))));
        }
    }
    BiomeMap &operator=(const BiomeMap &rt)
    {
        assert(size() == rt.size());
        auto j = rt.begin();
        for(auto i = begin(); i != end(); i++, j++)
        {
            std::get<1>(*i) = std::get<1>(*j);
        }
        return *this;
    }
    BiomeMap &operator=(BiomeMap &&rt)
    {
        assert(size() == rt.size());
        auto j = rt.begin();
        for(auto i = begin(); i != end(); i++, j++)
        {
            std::get<1>(*i) = std::get<1>(std::move(*j));
        }
        return *this;
    }
    std::size_t size() const
    {
        return elements.size();
    }
    bool empty() const
    {
        return size() == 0;
    }
    T &operator[](BiomeDescriptorPointer biome)
    {
        return std::get<1>(elements[getIndex(biome)]);
    }
    const T &operator[](BiomeDescriptorPointer biome) const
    {
        return std::get<1>(elements[getIndex(biome)]);
    }
    T &at(BiomeDescriptorPointer biome)
    {
        return std::get<1>(elements.at(getIndex(biome)));
    }
    const T &at(BiomeDescriptorPointer biome) const
    {
        return std::get<1>(elements.at(getIndex(biome)));
    }
    iterator begin()
    {
        return elements.begin();
    }
    const_iterator begin() const
    {
        return elements.cbegin();
    }
    const_iterator cbegin() const
    {
        return elements.cbegin();
    }
    iterator end()
    {
        return elements.end();
    }
    const_iterator end() const
    {
        return elements.cend();
    }
    const_iterator cend() const
    {
        return elements.cend();
    }
    iterator find(BiomeDescriptorPointer biome)
    {
        std::size_t index = getIndex(biome);
        if(index >= size())
            return end();
        return elements.begin() + index;
    }
    const_iterator find(BiomeDescriptorPointer biome) const
    {
        std::size_t index = getIndex(biome);
        if(index >= size())
            return cend();
        return elements.cbegin() + index;
    }
    size_type count(BiomeDescriptorPointer biome) const
    {
        std::size_t index = getIndex(biome);
        if(index >= size())
            return 0;
        return 1;
    }
    std::pair<iterator, iterator> equal_range(BiomeDescriptorPointer biome)
    {
        iterator iter = find(biome);
        iterator next_iter = iter;
        if(iter != end())
            ++next_iter;
        return std::pair<iterator, iterator>(iter, next_iter);
    }
    std::pair<const_iterator, const_iterator> equal_range(BiomeDescriptorPointer biome) const
    {
        const_iterator iter = find(biome);
        const_iterator next_iter = iter;
        if(iter != end())
            ++next_iter;
        return std::pair<const_iterator, const_iterator>(iter, next_iter);
    }
    void swap(BiomeMap &rt)
    {
        using std::swap;
        swap(elements, rt.elements);
    }
};

GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
class BiomeWeights final : public BiomeMap<float>
{
    GCC_PRAGMA(diagnostic pop)
public:
    void normalize()
    {
        float totalWeight = 0;
        for(const value_type &v : *this)
        {
            totalWeight += std::get<1>(v);
        }
        for(value_type &v : *this)
        {
            std::get<1>(v) /= totalWeight;
        }
    }
};

struct BiomeProperties final
{
private:
    BiomeWeights weights;
    bool good = false;
    template <const ColorF BiomeDescriptor::*member>
    ColorF calcColorInternal() const
    {
        ColorF retval = RGBAF(0, 0, 0, 0);
        for(const BiomeWeights::value_type &v : weights)
        {
            float weight = std::get<1>(v);
            const BiomeDescriptor &biome = *std::get<0>(v);
            ColorF c = biome.*member;
            retval.r += c.r * weight;
            retval.g += c.g * weight;
            retval.b += c.b * weight;
            retval.a += c.a * weight;
        }
        return retval;
    }
    template <const float BiomeDescriptor::*member>
    float calcFloatInternal() const
    {
        float retval = 0;
        for(const BiomeWeights::value_type &v : weights)
        {
            float weight = std::get<1>(v);
            const BiomeDescriptor &biome = *std::get<0>(v);
            retval += biome.*member * weight;
        }
        return retval;
    }
    ColorF grassColor = ColorF(), leavesColor = ColorF(), waterColor = ColorF();
    BiomeDescriptorPointer dominantBiome = nullptr;

public:
    const BiomeWeights &getWeights() const
    {
        return weights;
    }
    ColorF getGrassColor() const
    {
        return grassColor;
    }
    ColorF getLeavesColor() const
    {
        return leavesColor;
    }
    ColorF getWaterColor() const
    {
        return waterColor;
    }
    BiomeDescriptorPointer getDominantBiome() const
    {
        return dominantBiome;
    }
    explicit BiomeProperties(BiomeWeights weights);
    BiomeProperties() : weights()
    {
    }
    void swap(BiomeProperties &rt)
    {
        BiomeProperties temp = std::move(*this);
        *this = std::move(rt);
        rt = std::move(temp);
    }
    BiomeProperties(const BiomeProperties &) = default;
    BiomeProperties(BiomeProperties &&) = default;
    BiomeProperties &operator=(const BiomeProperties &) = default;
    BiomeProperties &operator=(BiomeProperties &&) = default;
    void write(stream::Writer &writer) const
    {
        assert(good);
        assert(static_cast<std::uint16_t>(weights.size()) == weights.size());
        stream::write<std::uint16_t>(writer, static_cast<std::uint16_t>(weights.size()));
        for(BiomeWeights::value_type v : weights)
        {
            BiomeDescriptors.writeDescriptor(writer, std::get<0>(v));
            stream::write<float32_t>(writer, std::get<1>(v));
        }
    }
    static BiomeProperties read(stream::Reader &reader)
    {
        BiomeWeights weights;
        for(auto &v : weights)
        {
            std::get<1>(v) = 0;
        }
        std::size_t count = static_cast<std::uint16_t>(stream::read<std::uint16_t>(reader));
        for(std::size_t i = 0; i < count; i++)
        {
            BiomeDescriptorPointer bd = BiomeDescriptors.readDescriptor(reader);
            if(bd == nullptr)
                throw stream::InvalidDataValueException("read null biome descriptor");
            float value = stream::read_limited<float32_t>(reader, 0, 1);
            weights[bd] = value;
        }
        return BiomeProperties(std::move(weights));
    }
};
}
}

namespace std
{
template <typename T>
void swap(programmerjake::voxels::BiomeMap<T> &a, programmerjake::voxels::BiomeMap<T> &b)
{
    a.swap(b);
}
}

#endif // BIOME_H_INCLUDED
