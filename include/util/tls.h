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
#ifndef TLS_H_INCLUDED
#define TLS_H_INCLUDED

#include <cstddef>
#include <atomic>
#include <exception>
#include <type_traits>
#include <cassert>
#include <memory>
#include "util/util.h"

#if _WIN64 || _WIN32
#ifdef _MSC_VER
#define USE_BUILTIN_TLS
#endif
#else
#define USE_BUILTIN_TLS
#endif

namespace programmerjake
{
namespace voxels
{
template <typename T, typename VariableTag>
class thread_local_variable;

class TLS final
{
    template <typename T, typename VariableTag>
    friend class thread_local_variable;
    TLS(const TLS &) = delete;
    TLS &operator =(const TLS &) = delete;
    void *operator new(std::size_t) = delete;
    void *operator new[](std::size_t) = delete;
    void operator delete(void *) = delete;
    void operator delete(void *, std::size_t) = delete;
    void operator delete[](void *) = delete;
    void operator delete[](void *, std::size_t) = delete;
private:
    static TLS *&getTlsSlowHelper()
    {
        static thread_local TLS *retval = nullptr;
        return retval;
    }
public:
#ifdef USE_BUILTIN_TLS
    static TLS &getSlow()
    {
        return *getTlsSlowHelper();
    }
    TLS()
    {
        getTlsSlowHelper() = this;
    }
    ~TLS() = default;
#else
    static TLS &getSlow();
    TLS();
    ~TLS();
private:
    static const std::size_t tls_memory_size;
    static std::atomic_size_t next_variable_position;
    char *memory;
    typedef void (*destructor_fn)(TLS &tls);
    struct destructor_list_node final
    {
        static constexpr std::size_t destructor_list_size = 256;
        destructor_fn destructors[destructor_list_size];
        std::size_t destructors_used = 0;
        destructor_list_node *next = nullptr;
    };
    destructor_list_node *destructor_list_head = nullptr;
    void add_destructor(destructor_fn fn);
    static std::size_t reserve_memory_slot(std::size_t variable_size);
    template <typename T, typename VariableTag>
    static std::size_t get_memory_slot()
    {
        static std::size_t retval = reserve_memory_slot(sizeof(T));
        return retval;
    }
#endif
};

template <typename T, typename VariableTag>
class thread_local_variable final
{
    thread_local_variable(const thread_local_variable &) = delete;
    thread_local_variable &operator =(const thread_local_variable &) = delete;
    void *operator new(std::size_t) = delete;
    void *operator new[](std::size_t) = delete;
    void operator delete(void *) = delete;
    void operator delete(void *, std::size_t) = delete;
    void operator delete[](void *) = delete;
    void operator delete[](void *, std::size_t) = delete;
private:
    struct wrapper final
    {
        T value;
        char constructed; // use char instead of bool because we don't have to worry about specific values of bool then
        template <typename ...Args>
        explicit wrapper(Args &&...args)
            : value(std::forward<Args>(args)...), constructed()
        {
            constructed = 1; // after value construction
        }
    };
#ifndef USE_BUILTIN_TLS
    struct thread_local_variable_slot_getter final
    {
        thread_local_variable_slot_getter()
        {
            TLS::get_memory_slot<wrapper, VariableTag>();
        }
        void link_in_helper()
        {
        }
    };
    static thread_local_variable_slot_getter thread_local_variable_slot_getter_v;
    static wrapper &get_wrapper(TLS &tls)
    {
        return *reinterpret_cast<wrapper *>(tls.memory + TLS::get_memory_slot<wrapper, VariableTag>());
    }
    static void destructor_fn(TLS &tls)
    {
        wrapper &w = get_wrapper(tls);
        if(w.constructed)
            w.wrapper::~wrapper();
    }
    static void add_to_destructor_list(TLS &tls)
    {
        tls.add_destructor(&destructor_fn);
    }
#endif
    wrapper *variable = nullptr;
public:
    template <typename ...Args>
    explicit thread_local_variable(TLS &tls, Args &&...args)
    {
#ifndef USE_BUILTIN_TLS
        thread_local_variable_slot_getter_v.link_in_helper(); // to generate the thread_local_variable_slot_getter
        wrapper &w = get_wrapper(tls);
        if(!w.constructed)
        {
            if(!std::is_trivially_destructible<T>::value)
                add_to_destructor_list(tls);
            new(static_cast<void *>(&w)) wrapper(std::forward<Args>(args)...);
        }
        variable = &w;
#else
        static thread_local wrapper w(std::forward<Args>(args)...);
        ignore_unused_variable_warning(tls);
        variable = &w;
#endif
    }
    explicit thread_local_variable(TLS &tls)
    {
#ifndef USE_BUILTIN_TLS
        thread_local_variable_slot_getter_v.link_in_helper(); // to generate the thread_local_variable_slot_getter
        wrapper &w = get_wrapper(tls);
        if(!w.constructed)
        {
            if(!std::is_trivially_destructible<T>::value)
                add_to_destructor_list(tls);
            new(static_cast<void *>(&w)) wrapper;
        }
        variable = &w;
#else
        static thread_local wrapper w;
        ignore_unused_variable_warning(tls);
        variable = &w;
#endif
    }
    T &get()
    {
        return variable->value;
    }
};

#ifndef USE_BUILTIN_TLS
template <typename T, typename VariableTag>
typename thread_local_variable<T, VariableTag>::thread_local_variable_slot_getter thread_local_variable<T, VariableTag>::thread_local_variable_slot_getter_v;
#else
#endif
}
}

#endif // TLS_H_INCLUDED
