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

#ifndef PHYSICS_COLLISION_H_INCLUDED
#define PHYSICS_COLLISION_H_INCLUDED

#include "util/vector.h"
#include <vector>
#include "util/checked_array.h"

namespace programmerjake
{
namespace voxels
{
namespace physics
{
struct ContactPoint final
{
    VectorF point;
    VectorF normal;
    float penetration;
    constexpr ContactPoint(VectorF point, VectorF normal, float penetration)
        : point(point), normal(normal), penetration(penetration)
    {
    }
    constexpr ContactPoint() : point(), normal(0), penetration()
    {
    }
    constexpr ContactPoint reversed() const
    {
        return ContactPoint(point - penetration * normal, -normal, penetration);
    }
};

struct Collision final
{
    static constexpr std::size_t maxContacts = 4;
    ContactPoint contacts[maxContacts];
    std::size_t contactCount;
    std::size_t myObjectIndex;
    std::size_t otherObjectIndex;
    static constexpr std::size_t NoObjectIndex = ~static_cast<std::size_t>(0);
    constexpr Collision()
        : contacts(), contactCount(0), myObjectIndex(NoObjectIndex), otherObjectIndex(NoObjectIndex)
    {
    }
    constexpr Collision(ContactPoint contact1, std::size_t myObjectIndex, std::size_t otherObjectIndex)
        : contacts{contact1},
          contactCount(1),
          myObjectIndex(myObjectIndex),
          otherObjectIndex(otherObjectIndex)
    {
    }
    constexpr Collision(ContactPoint contact1,
                        ContactPoint contact2,
                        std::size_t myObjectIndex,
                        std::size_t otherObjectIndex)
        : contacts{contact1, contact2},
          contactCount(2),
          myObjectIndex(myObjectIndex),
          otherObjectIndex(otherObjectIndex)
    {
    }
    constexpr Collision(ContactPoint contact1,
                        ContactPoint contact2,
                        ContactPoint contact3,
                        std::size_t myObjectIndex,
                        std::size_t otherObjectIndex)
        : contacts{contact1, contact2, contact3},
          contactCount(3),
          myObjectIndex(myObjectIndex),
          otherObjectIndex(otherObjectIndex)
    {
    }
    constexpr Collision(ContactPoint contact1,
                        ContactPoint contact2,
                        ContactPoint contact3,
                        ContactPoint contact4,
                        std::size_t myObjectIndex,
                        std::size_t otherObjectIndex)
        : contacts{contact1, contact2, contact3, contact4},
          contactCount(4),
          myObjectIndex(myObjectIndex),
          otherObjectIndex(otherObjectIndex)
    {
    }
    Collision(const ContactPoint *contacts,
                        std::size_t contactCount,
                        std::size_t myObjectIndex,
                        std::size_t otherObjectIndex)
        : contacts(),
          contactCount(contactCount),
          myObjectIndex(myObjectIndex),
          otherObjectIndex(otherObjectIndex)
    {
        assert(contactCount <= maxContacts);
        for(std::size_t i = 0; i < contactCount; i++)
        {
            this->contacts[i] = contacts[i];
        }
    }
    Collision(const std::vector<ContactPoint> &contacts,
                        std::size_t myObjectIndex,
                        std::size_t otherObjectIndex)
        : contacts(),
          contactCount(contacts.size()),
          myObjectIndex(myObjectIndex),
          otherObjectIndex(otherObjectIndex)
    {
        assert(contactCount <= maxContacts);
        for(std::size_t i = 0; i < contactCount; i++)
        {
            this->contacts[i] = contacts[i];
        }
    }
    template <std::size_t N>
    Collision(const checked_array<ContactPoint, N> &contacts,
                        std::size_t contactCount,
                        std::size_t myObjectIndex,
                        std::size_t otherObjectIndex)
        : contacts(),
          contactCount(contactCount),
          myObjectIndex(myObjectIndex),
          otherObjectIndex(otherObjectIndex)
    {
        assert(contactCount <= maxContacts && contactCount <= N);
        for(std::size_t i = 0; i < contactCount; i++)
        {
            this->contacts[i] = contacts[i];
        }
    }
    constexpr bool empty() const
    {
        return contactCount == 0;
    }
    Collision reversed() const
    {
        if(empty())
            return Collision();
        Collision retval(*this);
        for(std::size_t i = 0; i < contactCount; i++)
            retval.contacts[i] = retval.contacts[i].reversed();
        retval.myObjectIndex = otherObjectIndex;
        retval.otherObjectIndex = myObjectIndex;
        return retval;
    }
};
}
}
}

#endif /* PHYSICS_COLLISION_H_INCLUDED */
