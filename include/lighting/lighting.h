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
#ifndef LIGHTING_H_INCLUDED
#define LIGHTING_H_INCLUDED

#include "util/util.h"
#include "util/position.h"
#include <cstdint>
#include <algorithm>
#include "util/block_face.h"
#include "util/color.h"
#include "util/dimension.h"
#include <array>
#include <tuple>

namespace programmerjake
{
namespace voxels
{
struct WorldLightingProperties final
{
    float skyBrightness = 1, minBrightness = 0;
    constexpr WorldLightingProperties()
    {
    }
    constexpr WorldLightingProperties(float skyBrightness, float minBrightness)
        : skyBrightness(skyBrightness), minBrightness(minBrightness)
    {
    }
    WorldLightingProperties(float skyBrightness, Dimension d)
        : skyBrightness(skyBrightness), minBrightness(getZeroBrightnessLevel(d))
    {
    }
    bool operator ==(const WorldLightingProperties &rt) const
    {
        return skyBrightness == rt.skyBrightness && minBrightness == rt.minBrightness;
    }
    bool operator !=(const WorldLightingProperties &rt) const
    {
        return !operator ==(rt);
    }
};
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::WorldLightingProperties>
{
    hash<float> hasher;
    size_t operator ()(programmerjake::voxels::WorldLightingProperties wlp) const
    {
        return hasher(wlp.skyBrightness) + 3 * hasher(wlp.minBrightness);
    }
};
}

namespace programmerjake
{
namespace voxels
{
struct Lighting final
{
    typedef std::uint_fast8_t LightValueType;
    static constexpr int lightBitWidth = 4;
    static constexpr LightValueType maxLight = ((LightValueType)1 << lightBitWidth) - 1;
    static constexpr float toFloat(LightValueType v)
    {
        return (float)(int)v / (int)maxLight;
    }
    static constexpr LightValueType ensureInValidRange(LightValueType v)
    {
        return ensureInRange<LightValueType>(v, 0, maxLight);
    }
    LightValueType directSkylight;
    LightValueType indirectSkylight;
    LightValueType indirectArtificalLight;
private:
    template <typename T>
    static constexpr T constexpr_max(T a, T b)
    {
        return (a < b) ? b : a;
    }
    template <typename T>
    static constexpr T constexpr_min(T a, T b)
    {
        return (a < b) ? a : b;
    }
public:
    constexpr float toFloat(float skyBrightness, float minBrightness) const
    {
        return ensureInRange<float>(constexpr_max<float>(toFloat(indirectSkylight) * skyBrightness, toFloat(indirectArtificalLight)), 0, 1) * (1 - minBrightness) + minBrightness;
    }
    constexpr float toFloat(WorldLightingProperties wlp) const
    {
        return toFloat(wlp.skyBrightness, wlp.minBrightness);
    }
    constexpr Lighting()
        : directSkylight(0), indirectSkylight(0), indirectArtificalLight(0)
    {
    }
    constexpr Lighting(LightValueType directSkylight, LightValueType indirectSkylight, LightValueType indirectArtificalLight)
        : directSkylight(ensureInValidRange(directSkylight)), indirectSkylight(ensureInValidRange(constexpr_max(directSkylight, indirectSkylight))), indirectArtificalLight(ensureInValidRange(indirectArtificalLight))
    {
    }
    enum MakeDirectOnlyType
    {
        MakeDirectOnly
    };
    constexpr Lighting(LightValueType directSkylight, LightValueType indirectSkylight, LightValueType indirectArtificalLight, MakeDirectOnlyType)
        : directSkylight(ensureInValidRange(directSkylight)), indirectSkylight(ensureInValidRange(indirectSkylight)), indirectArtificalLight(ensureInValidRange(indirectArtificalLight))
    {
    }
    static constexpr Lighting makeSkyLighting()
    {
        return Lighting(maxLight, maxLight, 0);
    }
    static constexpr Lighting makeDirectOnlyLighting()
    {
        return Lighting(maxLight, 0, 0, MakeDirectOnly);
    }
    static constexpr Lighting makeArtificialLighting(LightValueType indirectArtificalLight)
    {
        return Lighting(0, 0, indirectArtificalLight);
    }
    static constexpr Lighting makeMaxLight()
    {
        return Lighting(maxLight, maxLight, maxLight);
    }
    constexpr Lighting combine(Lighting r) const
    {
        return Lighting(constexpr_max(directSkylight, r.directSkylight), constexpr_max(indirectSkylight, r.indirectSkylight), constexpr_max(indirectArtificalLight, r.indirectArtificalLight), MakeDirectOnly);
    }
    constexpr Lighting minimize(Lighting r) const
    {
        return Lighting(constexpr_min(directSkylight, r.directSkylight), constexpr_min(indirectSkylight, r.indirectSkylight), constexpr_min(indirectArtificalLight, r.indirectArtificalLight), MakeDirectOnly);
    }
private:
    static constexpr LightValueType sum(LightValueType a, LightValueType b)
    {
        return constexpr_min<LightValueType>(a + b, maxLight);
    }
public:
    constexpr Lighting sum(Lighting r) const
    {
        return Lighting(sum(directSkylight, r.directSkylight), sum(indirectSkylight, r.indirectSkylight), sum(indirectArtificalLight, r.indirectArtificalLight), MakeDirectOnly);
    }
private:
    static constexpr LightValueType reduce(LightValueType a, LightValueType b)
    {
        return (a < b ? 0 : a - b);
    }
public:
    constexpr Lighting reduce(Lighting r) const
    {
        return Lighting(reduce(directSkylight, r.directSkylight), reduce(indirectSkylight, r.indirectSkylight), reduce(indirectArtificalLight, r.indirectArtificalLight));
    }
    constexpr Lighting stripDirectSkylight() const
    {
        return Lighting(0, indirectSkylight, indirectArtificalLight);
    }
    constexpr bool operator ==(Lighting r) const
    {
        return directSkylight == r.directSkylight && indirectSkylight == r.indirectSkylight && indirectArtificalLight == r.indirectArtificalLight;
    }
    constexpr bool operator !=(Lighting r) const
    {
        return !operator ==(r);
    }
};

struct PackedLighting final
{
    typedef Lighting::LightValueType LightValueType;
    static constexpr int lightBitWidth = Lighting::lightBitWidth;
LightValueType directSkylight:
    lightBitWidth;
LightValueType indirectSkylight:
    lightBitWidth;
LightValueType indirectArtificalLight:
    lightBitWidth;
    constexpr explicit PackedLighting(Lighting l = Lighting())
        : directSkylight(l.directSkylight), indirectSkylight(l.indirectSkylight), indirectArtificalLight(l.indirectArtificalLight)
    {
    }
    constexpr explicit operator Lighting() const
    {
        return Lighting(directSkylight, indirectSkylight, indirectArtificalLight, Lighting::MakeDirectOnly);
    }
};

struct LightProperties final
{
    Lighting emissiveValue;
    Lighting reduceValue;
    constexpr LightProperties(Lighting emissiveValue = Lighting(0, 0, 0), Lighting reduceValue = Lighting(0, 1, 1))
        : emissiveValue(emissiveValue), reduceValue(reduceValue)
    {
    }
    constexpr Lighting eval(Lighting nx, Lighting px, Lighting ny, Lighting py, Lighting nz, Lighting pz) const
    {
        return emissiveValue.combine(nx.stripDirectSkylight().reduce(reduceValue))
               .combine(px.stripDirectSkylight().reduce(reduceValue))
               .combine(ny.stripDirectSkylight().reduce(reduceValue))
               .combine(py.reduce(reduceValue))
               .combine(nz.stripDirectSkylight().reduce(reduceValue))
               .combine(pz.stripDirectSkylight().reduce(reduceValue));
    }
    constexpr Lighting eval(Lighting inputLighting) const
    {
        return emissiveValue.combine(inputLighting.reduce(reduceValue));
    }
    constexpr Lighting createNewLighting(Lighting oldLighting) const
    {
        return emissiveValue.combine(oldLighting.minimize(Lighting::makeMaxLight().reduce(reduceValue)));
    }
    constexpr Lighting calculateTransmittedLighting(Lighting lighting) const
    {
        return lighting.reduce(reduceValue);
    }
    template <typename ...Args>
    constexpr Lighting calculateTransmittedLighting(Lighting lighting, Args ...args) const
    {
        return calculateTransmittedLighting(lighting).combine(calculateTransmittedLighting(args...));
    }
    constexpr LightProperties compose(LightProperties inFrontValue) const
    {
        return LightProperties(inFrontValue.emissiveValue.combine(inFrontValue.calculateTransmittedLighting(emissiveValue)), reduceValue.sum(inFrontValue.reduceValue));
    }
    constexpr LightProperties combine(LightProperties other) const
    {
        return LightProperties(other.emissiveValue.combine(emissiveValue), other.reduceValue.minimize(reduceValue));
    }
    constexpr bool isTotallyTransparent() const
    {
        return reduceValue.indirectSkylight <= 1 && reduceValue.indirectArtificalLight <= 1;
    }
};

struct BlockLighting final
{
    checked_array<checked_array<checked_array<float, 2>, 2>, 2> lightValues;
    constexpr ColorF eval(VectorF relativePosition) const
    {
        return GrayscaleF(interpolate(relativePosition.x,
                                      interpolate(relativePosition.y,
                                              interpolate(relativePosition.z,
                                                      lightValues[0][0][0],
                                                      lightValues[0][0][1]),
                                              interpolate(relativePosition.z,
                                                      lightValues[0][1][0],
                                                      lightValues[0][1][1])),
                                      interpolate(relativePosition.y,
                                              interpolate(relativePosition.z,
                                                      lightValues[1][0][0],
                                                      lightValues[1][0][1]),
                                              interpolate(relativePosition.z,
                                                      lightValues[1][1][0],
                                                      lightValues[1][1][1]))));
    }
private:
    float evalVertex(const checked_array<checked_array<checked_array<float, 3>, 3>, 3> &blockValues, VectorI offset);
public:
    constexpr BlockLighting()
        : lightValues{{{{{{0, 0}}, {{0, 0}}}}, {{{{0, 0}}, {{0, 0}}}}}}
    {
    }
    BlockLighting(checked_array<checked_array<checked_array<std::pair<LightProperties, Lighting>, 3>, 3>, 3> blocks, WorldLightingProperties wlp);
private:
    static constexpr VectorF getLightVector()
    {
        return VectorF(0, 1, 0);
    }
    static constexpr float getNormalFactorHelper(float v)
    {
        return 0.5f + (v < 0 ? v * 0.25f : v * 0.5f);
    }
    static constexpr float getNormalFactor(VectorF normal)
    {
        return 0.4f + 0.6f * getNormalFactorHelper(dot(normal, getLightVector()));
    }
public:
    constexpr ColorF lightVertex(VectorF relativePosition, ColorF vertexColor, VectorF normal) const
    {
        return colorize(scaleF(eval(relativePosition), getNormalFactor(normal)), vertexColor);
    }
};
}
}

#endif // LIGHTING_H_INCLUDED
