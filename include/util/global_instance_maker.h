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
#ifndef GLOBAL_INSTANCE_MAKER_H_INCLUDED
#define GLOBAL_INSTANCE_MAKER_H_INCLUDED

namespace programmerjake
{
namespace voxels
{
template <typename T>
class global_instance_maker final
{
private:
    static T *instance;
    static void init()
    {
        if(instance == nullptr)
            instance = new T();
    }
    static void deinit()
    {
        delete instance;
    }
    struct helper final
    {
        helper(const helper &) = delete;
        void operator =(const helper &) = delete;
        helper()
        {
            init();
        }
        ~helper()
        {
            deinit();
        }
        const T *passthrough(const T *retval)
        {
            return retval;
        }
    };
    static helper theHelper;
public:
    static const T *getInstance()
    {
        init();
        return theHelper.passthrough(instance);
    }
};

template <typename T>
typename global_instance_maker<T>::helper global_instance_maker<T>::theHelper;

template <typename T>
T *global_instance_maker<T>::instance = nullptr;
}
}

#endif // GLOBAL_INSTANCE_MAKER_H_INCLUDED
