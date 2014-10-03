#ifndef LIGHTING_H_INCLUDED
#define LIGHTING_H_INCLUDED

#include "util/util.h"
#include "util/position.h"
#include <cstdint>
#include <algorithm>
#include "util/block_face.h"
#include <array>
#include <tuple>

using namespace std;

struct Lighting final
{
    typedef uint_fast8_t LightValueType;
    static constexpr LightValueType maxLight = 0xF;
    static constexpr float toFloat(LightValueType v)
    {
        return (float)(int)v / (int)maxLight;
    }
    static constexpr LightValueType limitToValidRange(LightValueType v)
    {
        return limit<LightValueType>(v, 0, maxLight);
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
        return limit<float>(constexpr_max<float>(toFloat(indirectSkylight) * skyBrightness, toFloat(indirectArtificalLight)), 0, 1) * (1 - minBrightness) + minBrightness;
    }
    constexpr Lighting()
        : directSkylight(0), indirectSkylight(0), indirectArtificalLight(0)
    {
    }
    constexpr Lighting(LightValueType directSkylight, LightValueType indirectSkylight, LightValueType indirectArtificalLight)
        : directSkylight(limitToValidRange(directSkylight)), indirectSkylight(limitToValidRange(constexpr_max(directSkylight, indirectSkylight))), indirectArtificalLight(limitToValidRange(indirectArtificalLight))
    {
    }
    static constexpr Lighting makeSkyLighting()
    {
        return Lighting(maxLight, maxLight, 0);
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
        return Lighting(constexpr_max(directSkylight, r.directSkylight), constexpr_max(indirectSkylight, r.indirectSkylight), constexpr_max(indirectArtificalLight, r.indirectArtificalLight));
    }
    constexpr Lighting minimize(Lighting r) const
    {
        return Lighting(constexpr_min(directSkylight, r.directSkylight), constexpr_min(indirectSkylight, r.indirectSkylight), constexpr_min(indirectArtificalLight, r.indirectArtificalLight));
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
    constexpr Lighting createNewLighting(Lighting oldLighting) const
    {
        return emissiveValue.combine(oldLighting.minimize(Lighting::makeMaxLight().reduce(reduceValue)));
    }
};

#endif // LIGHTING_H_INCLUDED
