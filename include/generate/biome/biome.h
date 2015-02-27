/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
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

namespace programmerjake
{
namespace voxels
{
class BiomeDescriptor;
class RandomSource;
typedef const BiomeDescriptor *BiomeDescriptorPointer;
typedef std::size_t BiomeIndex;
class BiomeDescriptor
{
    friend class BiomeDescriptors_t;
private:
    BiomeIndex index;
public:
    BiomeIndex getIndex() const
    {
        return index;
    }
    const std::wstring name;
    const float temperature;
    const float humidity;
    const ColorF grassColor;
    const ColorF leavesColor;
    const ColorF waterColor;
    static ColorF makeBiomeGrassColor(float temperature, float humidity)
    {
        temperature = limit<float>(temperature, 0, 1);
        humidity = limit<float>(humidity, 0, 1) * temperature;
        return interpolate(temperature, RGBF(0.5f, 0.7f, 0.6f), interpolate(humidity, RGBF(0.75f, 0.7f, 0.33f), RGBF(0.25f, 1.0f, 0.2f)));
    }
    static ColorF makeBiomeLeavesColor(float temperature, float humidity)
    {
        temperature = limit<float>(temperature, 0, 1);
        humidity = limit<float>(humidity, 0, 1) * temperature;
        return interpolate(temperature, RGBF(0.4f, 0.63f, 0.5f), interpolate(humidity, RGBF(0.7f, 0.65f, 0.16f), RGBF(0.1f, 0.8f, 0.0f)));
    }
    static ColorF makeBiomeWaterColor(float temperature, float humidity)
    {
        temperature = limit<float>(temperature, 0, 1);
        humidity = limit<float>(humidity, 0, 1) * temperature;
        return interpolate(temperature, RGBF(0.0f, 0.0f, 1.0f), interpolate(humidity, RGBF(0.0f, 0.63f, 0.83f), RGBF(0.0f, 0.9f, 0.75f)));
    }
    virtual float getBiomeCorrespondence(float temperature, float humidity, PositionI pos, RandomSource &randomSource) const = 0;
protected:
    BiomeDescriptor(std::wstring name, float temperature, float humidity, ColorF grassColor, ColorF leavesColor, ColorF waterColor);
    BiomeDescriptor(std::wstring name, float temperature, float humidity)
        : BiomeDescriptor(name, temperature, humidity, makeBiomeGrassColor(temperature, humidity), makeBiomeLeavesColor(temperature, humidity), makeBiomeWaterColor(temperature, humidity))
    {
    }
};

class BiomeWeights;

class BiomeDescriptors_t final
{
    friend class BiomeDescriptor;
private:
    static linked_map<std::wstring, BiomeDescriptorPointer> *pBiomeNameMap;
    static std::vector<BiomeDescriptorPointer> *pBiomeVector;
    static void addBiome(BiomeDescriptor *biome)
    {
        if(pBiomeVector == nullptr)
        {
            pBiomeNameMap = new linked_map<std::wstring, BiomeDescriptorPointer>;
            pBiomeVector = new std::vector<BiomeDescriptorPointer>;
        }
        biome->index = pBiomeVector->size();
        pBiomeVector->push_back(biome);
        assert(0 == pBiomeNameMap->count(biome->name));
        pBiomeNameMap->insert(linked_map<std::wstring, BiomeDescriptorPointer>::value_type(biome->name, biome));
    }
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
        return find(biome->index);
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
    BiomeDescriptorPointer operator [](BiomeIndex bi) const
    {
        return *find(bi);
    }
    BiomeWeights getBiomeWeights(float temperature, float humidity, PositionI pos, RandomSource &randomSource) const;
};

static constexpr BiomeDescriptors_t BiomeDescriptors;

inline BiomeDescriptor::BiomeDescriptor(std::wstring name, float temperature, float humidity, ColorF grassColor, ColorF leavesColor, ColorF waterColor)
    : name(name), temperature(temperature), humidity(humidity), grassColor(grassColor), leavesColor(leavesColor), waterColor(waterColor)
{
    BiomeDescriptors.addBiome(this);
}

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
            return ~(std::size_t)0;
        return biome->getIndex();
    }
public:
    BiomeMap()
    {
        elements.reserve(BiomeDescriptors.size());
        for(BiomeDescriptorPointer v : BiomeDescriptors)
        {
            elements.emplace_back(std::piecewise_construct, std::tuple<BiomeDescriptorPointer>(v), std::tuple<>());
        }
    }
    std::size_t size() const
    {
        return elements.size();
    }
    bool empty() const
    {
        return size() == 0;
    }
    T &operator [](BiomeDescriptorPointer biome)
    {
        return std::get<1>(elements[getIndex(biome)]);
    }
    const T &operator [](BiomeDescriptorPointer biome) const
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
        std::swap(elements, rt.elements);
    }
};

class BiomeWeights final : public BiomeMap<float>
{
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
    ColorF grassColor, leavesColor, waterColor;
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
    BiomeProperties(BiomeWeights weights)
        : weights(weights)
    {
        weights.normalize();
        grassColor = calcColorInternal<&BiomeDescriptor::grassColor>();
        leavesColor = calcColorInternal<&BiomeDescriptor::leavesColor>();
        waterColor = calcColorInternal<&BiomeDescriptor::waterColor>();
        good = true;
    }
    BiomeProperties()
    {
    }
    void swap(BiomeProperties &rt)
    {
        BiomeProperties temp = std::move(*this);
        *this = std::move(rt);
        rt = std::move(temp);
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
