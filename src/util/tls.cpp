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
#include "util/tls.h"
#include "util/logging.h"

namespace programmerjake
{
namespace voxels
{
#ifndef USE_BUILTIN_TLS
std::atomic_size_t TLS::next_variable_position(0);
const std::size_t TLS::tls_memory_size = 1 << 22;

TLS::TLS()
    : memory(new char[tls_memory_size])
{
    getTlsSlowHelper() = this;
    for(std::size_t i = 0; i < tls_memory_size; i++)
    {
        memory[i] = 0; // zero initialize memory to clear constructed flags
    }
}

TLS::~TLS()
{
    while(destructor_list_head != nullptr)
    {
        for(std::size_t i = destructor_list_head->destructors_used; i > 0; i--)
        {
            destructor_list_head->destructors[i - 1](*this);
        }
        destructor_list_node *next = destructor_list_head->next;
        delete destructor_list_head;
        destructor_list_head = next;
    }
    delete []memory;
}

namespace
{
TLS &makeForgottenTLS()
{
    TLS retval;
    return retval;
}
}

TLS &TLS::getSlow()
{
    TLS *&ptls = getTlsSlowHelper();
    if(ptls == nullptr)
    {
        ptls = makeForgottenTLS();
        getDebugLog() << "\n\nTLS::getSlow() : WARNING : forgot to make TLS instance in thread\n\n" << post;
    }
    return *ptls;
}

void TLS::add_destructor(destructor_fn fn)
{
    assert(fn);
    if(destructor_list_head == nullptr || destructor_list_head->destructors_used >= destructor_list_node::destructor_list_size)
    {
        destructor_list_node *new_node = new destructor_list_node;
        new_node->next = destructor_list_head;
        destructor_list_head = new_node;
    }
    destructor_list_head->destructors[destructor_list_head->destructors_used++] = fn;
}

std::size_t TLS::reserve_memory_slot(std::size_t variable_size)
{
    using namespace std; // for max_align_t;
    const std::size_t max_align = alignof(max_align_t);
    if(variable_size == 0)
        variable_size = 1; // always allocate different addresses
    variable_size += max_align - 1;
    variable_size /= max_align;
    variable_size *= max_align; // round up to next aligned size
    std::size_t retval = next_variable_position.fetch_add(variable_size, std::memory_order_relaxed);
    if(retval + variable_size > tls_memory_size)
    {
        std::terminate();
    }
    return retval;
}
#endif // USE_BUILTIN_TLS
}
}
