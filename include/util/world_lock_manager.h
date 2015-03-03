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
#ifndef WORLD_LOCK_MANAGER_H_INCLUDED
#define WORLD_LOCK_MANAGER_H_INCLUDED
namespace programmerjake
{
namespace voxels
{
struct WorldLockManager final
{
    template <typename T>
    class LockManager final
    {
        T *the_lock;
    public:
        LockManager()
            : the_lock(nullptr)
        {
        }
        LockManager(const LockManager &) = delete;
        const LockManager &operator =(const LockManager &) = delete;
        ~LockManager()
        {
            clear();
        }
        void clear()
        {
            if(the_lock != nullptr)
                the_lock->unlock();
            the_lock = nullptr;
        }
        void set(T &new_lock)
        {
            if(the_lock != &new_lock)
            {
                clear();
                new_lock.lock();
                the_lock = &new_lock;
            }
        }
    };
    LockManager<std::mutex> block_biome_lock;
    //LockManager<std::mutex> biome_lock;
    void clear()
    {
        block_biome_lock.clear();
        //biome_lock.clear();
    }
};
}
}
#endif // WORLD_LOCK_MANAGER_H_INCLUDED
