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
#ifndef NETWORK_EVENT_H_INCLUDED
#define NETWORK_EVENT_H_INCLUDED

#include "stream/stream.h"
#include "util/enum_traits.h"

namespace programmerjake
{
namespace voxels
{
enum class NetworkEventType : uint8_t
{
    Keepalive,
    SendNewChunk,
    SendBlockUpdate,
    RequestChunk,
    SendPlayerProperties,
    DEFINE_ENUM_LIMITS(Keepalive, SendPlayerProperties)
};

class NetworkEvent final
{
public:
    NetworkEventType type;
private:
    vector<uint8_t> bytes;
public:
    NetworkEvent(NetworkEventType type = NetworkEventType::Keepalive)
        : type(type), bytes()
    {
    }
    NetworkEvent(NetworkEventType type, const stream::MemoryWriter &writer)
        : type(type), bytes(writer.getBuffer())
    {
    }
    NetworkEvent(NetworkEventType type, stream::MemoryWriter &&writer)
        : type(type), bytes(std::move(writer).getBuffer())
    {
    }
    NetworkEvent(NetworkEventType type, const vector<uint8_t> & bytes)
        : type(type), bytes(bytes)
    {
    }
    NetworkEvent(NetworkEventType type, vector<uint8_t> && bytes)
        : type(type), bytes(bytes)
    {
    }
    void write(stream::Writer &writer) const
    {
        stream::write<NetworkEventType>(writer, type);
        uint32_t eventSize = bytes.size();
        assert((size_t)eventSize == bytes.size());
        stream::write<uint32_t>(writer, eventSize);
        writer.writeBytes(&bytes[0], bytes.size());
    }
    static NetworkEvent read(stream::Reader &reader)
    {
        NetworkEventType type = stream::read<NetworkEventType>(reader);
        uint32_t eventSize = stream::read<uint32_t>(reader);
        vector<uint8_t> bytes;
        bytes.resize((size_t)eventSize);
        reader.readBytes(&bytes[0], (size_t)eventSize);
        return NetworkEvent(type, std::move(bytes));
    }
    shared_ptr<stream::Reader> getReader() const &
    {
        return make_shared<stream::MemoryReader>(bytes);
    }
    shared_ptr<stream::Reader> getReader() &&
    {
        return make_shared<stream::MemoryReader>(std::move(bytes));
    }
};
}
}

#endif // NETWORK_EVENT_H_INCLUDED
