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
#ifndef RC4_RANDOM_ENGINE_H_INCLUDED
#define RC4_RANDOM_ENGINE_H_INCLUDED

#include "stream/stream.h"
#include "util/checked_array.h"
#include <cstdint>
#include <cassert>
#include "util/logging.h"

namespace programmerjake
{
namespace voxels
{
/** RC4 pseudo-random number generator
 * @warning not cryptographically secure
 * @see <a href="http://en.wikipedia.org/wiki/RC4#Security">RC4 Security on Wikipedia</a>
 */
class rc4_random_engine final
{
public:
    typedef std::uint8_t word_type;
    typedef std::uint64_t seed_type;
private:
    static constexpr word_type word_type_max = (1 << 8) - 1;
    checked_array<word_type, static_cast<std::size_t>(word_type_max) + 1> state_array;
    word_type state_i, state_j;
    void initialize(const word_type *key, std::size_t key_size)
    {
        const word_type default_key_value = 0;
        if(key_size < 1)
        {
            key = &default_key_value;
            key_size = 1;
        }
        for(std::size_t i = 0; i < state_array.size(); i++)
        {
            state_array[i] = i;
        }
        for(std::size_t i = 0, j = 0; i < state_array.size(); i++)
        {
            j = (j + static_cast<std::size_t>(state_array[i]) + key[i % key_size]) % state_array.size();
            word_type temp = state_array[i];
            state_array[i] = state_array[j];
            state_array[j] = temp;
        }
        state_i = 0;
        state_j = 0;
    }
    void initialize(seed_type key)
    {
        checked_array<word_type, 8> array_key;
        array_key[0] = static_cast<word_type>(key); // store in little-endian
        array_key[1] = static_cast<word_type>(key >> 8);
        array_key[2] = static_cast<word_type>(key >> 16);
        array_key[3] = static_cast<word_type>(key >> 24);
        array_key[4] = static_cast<word_type>(key >> 32);
        array_key[5] = static_cast<word_type>(key >> 40);
        array_key[6] = static_cast<word_type>(key >> 48);
        array_key[7] = static_cast<word_type>(key >> 56);
        initialize(&array_key[0], array_key.size());
    }
    struct read_construct_tag_t
    {
    };
    explicit rc4_random_engine(read_construct_tag_t)
        : state_array(),
        state_i(),
        state_j()
    {
    }
public:
    typedef std::uint_fast32_t result_type;
    static constexpr seed_type default_seed = 0;
    explicit rc4_random_engine(seed_type key = default_seed)
        : state_array(),
        state_i(),
        state_j()
    {
        initialize(key);
    }
    explicit rc4_random_engine(const word_type *key, std::size_t key_size)
        : state_array(),
        state_i(),
        state_j()
    {
        assert(key_size == 0 || !key);
        initialize(key, key_size);
    }
    void seed(seed_type key = default_seed)
    {
        initialize(key);
    }
    void seed(const word_type *key, std::size_t key_size)
    {
        assert(key_size == 0 || !key);
        initialize(key, key_size);
    }
    result_type operator ()()
    {
        state_i = (state_i + 1) % state_array.size();
        state_j = (state_j + state_array[state_i]) % state_array.size();
        word_type temp = state_array[state_i];
        state_array[state_i] = state_array[state_j];
        state_array[state_j] = temp;
        word_type retval = state_array[(state_array[state_i] + state_array[state_j]) % state_array.size()];
        return retval;
    }
    void discard(unsigned long long count)
    {
        for(unsigned long long i = 0; i < count; i++)
            operator ()();
    }
    static constexpr result_type min()
    {
        return 0;
    }
    static constexpr result_type max()
    {
        return word_type_max;
    }
    void write(stream::Writer &writer) const
    {
        writer.writeBytes(&state_array[0], state_array.size());
        stream::write<word_type>(writer, state_i);
        stream::write<word_type>(writer, state_j);
    }
    static rc4_random_engine read(stream::Reader &reader)
    {
        rc4_random_engine retval{read_construct_tag_t()};
        reader.readAllBytes(&retval.state_array[0], retval.state_array.size());
        retval.state_i = stream::read<word_type>(reader);
        retval.state_j = stream::read<word_type>(reader);
        return retval;
    }
};
}
}

#endif // RC4_RANDOM_ENGINE_H_INCLUDED
