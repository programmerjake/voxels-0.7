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
#ifndef PHYSICS_PROPERTIES_H_INCLUDED
#define PHYSICS_PROPERTIES_H_INCLUDED

#include <cstdint>
#include "stream/stream.h"
#include "util/vector.h"

namespace programmerjake
{
namespace voxels
{
namespace physics
{
struct Properties final
{
    typedef std::uint32_t CollisionMaskType;
    static constexpr CollisionMaskType blockCollisionMask = 1 << 0;
    static constexpr CollisionMaskType itemCollisionMask = 1 << 1;
    static constexpr CollisionMaskType playerCollisionMask = 1 << 2;
    static constexpr CollisionMaskType defaultCollisionMask = 0xFFFFFF & ~itemCollisionMask;
    float coefficientOfStaticFriction;
    float coefficientOfKineticFriction;
    float coefficientOfRestitution;
    float dragFactor;
    CollisionMaskType
        myCollisionMask; // for checking when this object is colliding with other objects
    CollisionMaskType
        othersCollisionMask; // for checking when other objects are colliding with this object
    VectorF gravity;
    struct WrappedCoefficientOfStaticFriction final
    {
        float coefficientOfStaticFriction;
    };
    friend constexpr WrappedCoefficientOfStaticFriction makeCoefficientOfStaticFriction(float value)
    {
        return WrappedCoefficientOfStaticFriction{value};
    }
    struct WrappedCoefficientOfKineticFriction final
    {
        float coefficientOfKineticFriction;
    };
    friend constexpr WrappedCoefficientOfKineticFriction makeCoefficientOfKineticFriction(
        float value)
    {
        return WrappedCoefficientOfKineticFriction{value};
    }
    struct WrappedCoefficientOfRestitution final
    {
        float coefficientOfRestitution;
    };
    friend constexpr WrappedCoefficientOfRestitution makeCoefficientOfRestitution(float value)
    {
        return WrappedCoefficientOfRestitution{value};
    }
    struct WrappedDragFactor final
    {
        float dragFactor;
    };
    friend constexpr WrappedDragFactor makeDragFactor(float value)
    {
        return WrappedDragFactor{value};
    }
    struct WrappedMyCollisionMask final
    {
        CollisionMaskType myCollisionMask;
    };
    friend constexpr WrappedMyCollisionMask makeMyCollisionMask(CollisionMaskType value)
    {
        return WrappedMyCollisionMask{value};
    }
    struct WrappedOthersCollisionMask final
    {
        CollisionMaskType othersCollisionMask;
    };
    friend constexpr WrappedOthersCollisionMask makeOthersCollisionMask(CollisionMaskType value)
    {
        return WrappedOthersCollisionMask{value};
    }
    struct WrappedGravity final
    {
        VectorF gravity;
    };
    friend constexpr WrappedGravity makeGravity(VectorF value)
    {
        return WrappedGravity{value};
    }
    explicit constexpr Properties()
        : coefficientOfStaticFriction(0.9f),
          coefficientOfKineticFriction(0.4f),
          coefficientOfRestitution(0.8f),
          dragFactor(1.0f),
          myCollisionMask(defaultCollisionMask),
          othersCollisionMask(defaultCollisionMask),
          gravity(defaultGravityVector)
    {
    }

private:
    struct ConstructTag final
    {
    };
    template <typename... Args>
    Properties(ConstructTag, Args...) = delete;
    constexpr Properties(ConstructTag, Properties source) : Properties(source)
    {
    }
    template <typename... Args>
    constexpr Properties(ConstructTag,
                         Properties source,
                         WrappedCoefficientOfStaticFriction arg,
                         Args... args)
        : coefficientOfStaticFriction(arg.coefficientOfStaticFriction),
          coefficientOfKineticFriction(source.coefficientOfKineticFriction),
          coefficientOfRestitution(source.coefficientOfRestitution),
          dragFactor(source.dragFactor),
          myCollisionMask(source.myCollisionMask),
          othersCollisionMask(source.othersCollisionMask),
          gravity(source.gravity)
    {
    }
    template <typename... Args>
    constexpr Properties(ConstructTag,
                         Properties source,
                         WrappedCoefficientOfKineticFriction arg,
                         Args... args)
        : coefficientOfStaticFriction(source.coefficientOfStaticFriction),
          coefficientOfKineticFriction(arg.coefficientOfKineticFriction),
          coefficientOfRestitution(source.coefficientOfRestitution),
          dragFactor(source.dragFactor),
          myCollisionMask(source.myCollisionMask),
          othersCollisionMask(source.othersCollisionMask),
          gravity(source.gravity)
    {
    }
    template <typename... Args>
    constexpr Properties(ConstructTag,
                         Properties source,
                         WrappedCoefficientOfRestitution arg,
                         Args... args)
        : coefficientOfStaticFriction(source.coefficientOfStaticFriction),
          coefficientOfKineticFriction(source.coefficientOfKineticFriction),
          coefficientOfRestitution(arg.coefficientOfRestitution),
          dragFactor(source.dragFactor),
          myCollisionMask(source.myCollisionMask),
          othersCollisionMask(source.othersCollisionMask),
          gravity(source.gravity)
    {
    }
    template <typename... Args>
    constexpr Properties(ConstructTag, Properties source, WrappedDragFactor arg, Args... args)
        : coefficientOfStaticFriction(source.coefficientOfStaticFriction),
          coefficientOfKineticFriction(source.coefficientOfKineticFriction),
          coefficientOfRestitution(source.coefficientOfRestitution),
          dragFactor(arg.dragFactor),
          myCollisionMask(source.myCollisionMask),
          othersCollisionMask(source.othersCollisionMask),
          gravity(source.gravity)
    {
    }
    template <typename... Args>
    constexpr Properties(ConstructTag, Properties source, WrappedMyCollisionMask arg, Args... args)
        : coefficientOfStaticFriction(source.coefficientOfStaticFriction),
          coefficientOfKineticFriction(source.coefficientOfKineticFriction),
          coefficientOfRestitution(source.coefficientOfRestitution),
          dragFactor(source.dragFactor),
          myCollisionMask(arg.myCollisionMask),
          othersCollisionMask(source.othersCollisionMask),
          gravity(source.gravity)
    {
    }
    template <typename... Args>
    constexpr Properties(ConstructTag,
                         Properties source,
                         WrappedOthersCollisionMask arg,
                         Args... args)
        : coefficientOfStaticFriction(source.coefficientOfStaticFriction),
          coefficientOfKineticFriction(source.coefficientOfKineticFriction),
          coefficientOfRestitution(source.coefficientOfRestitution),
          dragFactor(source.dragFactor),
          myCollisionMask(source.myCollisionMask),
          othersCollisionMask(arg.othersCollisionMask),
          gravity(source.gravity)
    {
    }
    template <typename... Args>
    constexpr Properties(ConstructTag, Properties source, WrappedGravity arg, Args... args)
        : coefficientOfStaticFriction(source.coefficientOfStaticFriction),
          coefficientOfKineticFriction(source.coefficientOfKineticFriction),
          coefficientOfRestitution(source.coefficientOfRestitution),
          dragFactor(source.dragFactor),
          myCollisionMask(source.myCollisionMask),
          othersCollisionMask(source.othersCollisionMask),
          gravity(arg.gravity)
    {
    }

public:
    template <typename... Args>
    explicit constexpr Properties(Args... args)
        : Properties(ConstructTag(), Properties(), args...)
    {
    }
    static Properties read(stream::Reader &reader)
    {
        Properties retval;
        retval.coefficientOfStaticFriction = stream::read_limited<float32_t>(reader, 0, 1e5);
        retval.coefficientOfKineticFriction = stream::read_limited<float32_t>(reader, 0, 1e5);
        retval.coefficientOfRestitution = stream::read_limited<float32_t>(reader, 0, 1);
        retval.dragFactor = stream::read_limited<float32_t>(reader, 0, 1e5);
        retval.myCollisionMask = stream::read<CollisionMaskType>(reader);
        retval.othersCollisionMask = stream::read<CollisionMaskType>(reader);
        retval.gravity = stream::read<VectorF>(reader);
        return retval;
    }
    void write(stream::Writer &writer) const
    {
        stream::write<float32_t>(writer, coefficientOfStaticFriction);
        stream::write<float32_t>(writer, coefficientOfKineticFriction);
        stream::write<float32_t>(writer, coefficientOfRestitution);
        stream::write<float32_t>(writer, dragFactor);
        stream::write<CollisionMaskType>(writer, myCollisionMask);
        stream::write<CollisionMaskType>(writer, othersCollisionMask);
        stream::write<VectorF>(writer, gravity);
    }
    constexpr bool canCollideWith(const Properties &r) const
    {
        return (myCollisionMask & r.othersCollisionMask) != 0;
    }
};
}
}
}

#endif /* PHYSICS_PROPERTIES_H_INCLUDED */
