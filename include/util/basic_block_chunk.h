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
#ifndef BASIC_BLOCK_CHUNK_H_INCLUDED
#define BASIC_BLOCK_CHUNK_H_INCLUDED

#include "util/position.h"
#include <array>
#include <iterator>
#include <utility>
#include <tuple>
#include <stdexcept>
#include <cstddef>
#include <mutex>
#include <memory>
#include <vector>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
template <typename BT, typename BiomeT, typename SCT, std::size_t ChunkShiftXV = 4, std::size_t ChunkShiftYV = 8, std::size_t ChunkShiftZV = 4, std::size_t SubchunkShiftXYZV = 3>
struct BasicBlockChunk
{
    typedef BT BlockType;
    typedef BiomeT BiomeType;
    typedef SCT SubchunkType;
    const PositionI basePosition;
    static constexpr std::size_t chunkShiftX = ChunkShiftXV;
    static constexpr std::size_t chunkShiftY = ChunkShiftYV;
    static constexpr std::size_t chunkShiftZ = ChunkShiftZV;
    static constexpr std::size_t subchunkShiftXYZ = SubchunkShiftXYZV;
    static constexpr std::int32_t chunkSizeX = (std::int32_t)1 << chunkShiftX;
    static constexpr std::int32_t chunkSizeY = (std::int32_t)1 << chunkShiftY;
    static constexpr std::int32_t chunkSizeZ = (std::int32_t)1 << chunkShiftZ;
    static constexpr std::int32_t subchunkSizeXYZ = (std::int32_t)1 << subchunkShiftXYZ;
    static constexpr std::int32_t subchunkCountX = chunkSizeX / subchunkSizeXYZ;
    static constexpr std::int32_t subchunkCountY = chunkSizeY / subchunkSizeXYZ;
    static constexpr std::int32_t subchunkCountZ = chunkSizeZ / subchunkSizeXYZ;
    static_assert(chunkSizeX > 0, "chunkSizeX must be positive");
    static_assert(chunkSizeY > 0, "chunkSizeY must be positive");
    static_assert(chunkSizeZ > 0, "chunkSizeZ must be positive");
    static_assert(subchunkSizeXYZ > 0, "subchunkSizeXYZ must be positive");
    static_assert((chunkSizeX & (chunkSizeX - 1)) == 0, "chunkSizeX must be a power of 2");
    static_assert((chunkSizeY & (chunkSizeY - 1)) == 0, "chunkSizeY must be a power of 2");
    static_assert((chunkSizeZ & (chunkSizeZ - 1)) == 0, "chunkSizeZ must be a power of 2");
    static_assert((subchunkSizeXYZ & (subchunkSizeXYZ - 1)) == 0, "subchunkSizeXYZ must be a power of 2");
    static_assert(subchunkSizeXYZ <= chunkSizeX && subchunkSizeXYZ <= chunkSizeY && subchunkSizeXYZ <= chunkSizeZ, "subchunkSizeXYZ must not be bigger than the chunk size");
    constexpr BasicBlockChunk(PositionI basePosition)
        : basePosition(basePosition),
        biomes(),
        blocks(),
        subchunks()
    {
    }
    static constexpr PositionI getChunkBasePosition(PositionI pos)
    {
        return PositionI(pos.x & ~(chunkSizeX - 1), pos.y & ~(chunkSizeY - 1), pos.z & ~(chunkSizeZ - 1), pos.d);
    }
    static constexpr VectorI getChunkBasePosition(VectorI pos)
    {
        return VectorI(pos.x & ~(chunkSizeX - 1), pos.y & ~(chunkSizeY - 1), pos.z & ~(chunkSizeZ - 1));
    }
    static constexpr PositionI getChunkRelativePosition(PositionI pos)
    {
        return PositionI(pos.x & (chunkSizeX - 1), pos.y & (chunkSizeY - 1), pos.z & (chunkSizeZ - 1), pos.d);
    }
    static constexpr VectorI getChunkRelativePosition(VectorI pos)
    {
        return VectorI(pos.x & (chunkSizeX - 1), pos.y & (chunkSizeY - 1), pos.z & (chunkSizeZ - 1));
    }
    static constexpr std::int32_t getSubchunkBaseAbsolutePosition(std::int32_t v)
    {
        return v & ~(subchunkSizeXYZ - 1);
    }
    static constexpr VectorI getSubchunkBaseAbsolutePosition(VectorI pos)
    {
        return VectorI(getSubchunkBaseAbsolutePosition(pos.x), getSubchunkBaseAbsolutePosition(pos.y), getSubchunkBaseAbsolutePosition(pos.z));
    }
    static constexpr PositionI getSubchunkBaseAbsolutePosition(PositionI pos)
    {
        return PositionI(getSubchunkBaseAbsolutePosition(pos.x), getSubchunkBaseAbsolutePosition(pos.y), getSubchunkBaseAbsolutePosition(pos.z), pos.d);
    }
    static constexpr VectorI getSubchunkBaseRelativePosition(VectorI pos)
    {
        return VectorI(pos.x & ((chunkSizeX - 1) & ~(subchunkSizeXYZ - 1)), pos.y & ((chunkSizeY - 1) & ~(subchunkSizeXYZ - 1)), pos.z & ((chunkSizeZ - 1) & ~(subchunkSizeXYZ - 1)));
    }
    static constexpr VectorI getSubchunkIndexFromChunkRelativePosition(VectorI pos)
    {
        return VectorI(pos.x >> subchunkShiftXYZ, pos.y >> subchunkShiftXYZ, pos.z >> subchunkShiftXYZ);
    }
    static constexpr VectorI getChunkRelativePositionFromSubchunkIndex(VectorI pos)
    {
        return VectorI(pos.x << subchunkShiftXYZ, pos.y << subchunkShiftXYZ, pos.z << subchunkShiftXYZ);
    }
    static constexpr VectorI getSubchunkIndexFromPosition(VectorI pos)
    {
        return getSubchunkIndexFromChunkRelativePosition(getChunkRelativePosition(pos));
    }
    static constexpr PositionI getSubchunkBaseRelativePosition(PositionI pos)
    {
        return PositionI(pos.x & ((chunkSizeX - 1) & ~(subchunkSizeXYZ - 1)), pos.y & ((chunkSizeY - 1) & ~(subchunkSizeXYZ - 1)), pos.z & ((chunkSizeZ - 1) & ~(subchunkSizeXYZ - 1)), pos.d);
    }
    static constexpr std::int32_t getSubchunkRelativePosition(std::int32_t v)
    {
        return v & (subchunkSizeXYZ - 1);
    }
    static constexpr VectorI getSubchunkRelativePosition(VectorI pos)
    {
        return VectorI(getSubchunkRelativePosition(pos.x), getSubchunkRelativePosition(pos.y), getSubchunkRelativePosition(pos.z));
    }
    static constexpr PositionI getSubchunkRelativePosition(PositionI pos)
    {
        return PositionI(getSubchunkRelativePosition(pos.x), getSubchunkRelativePosition(pos.y), getSubchunkRelativePosition(pos.z), pos.d);
    }
    checked_array<checked_array<BiomeType, chunkSizeZ>, chunkSizeX> biomes;
    class BlocksArray final
    {
    private:
        BlockType blocks[chunkSizeX * chunkSizeY * chunkSizeZ];
        static std::size_t getIndex(std::size_t indexX, std::size_t indexY, std::size_t indexZ)
        {
            assert(indexX < static_cast<std::size_t>(chunkSizeX) && indexY < static_cast<std::size_t>(chunkSizeY) && indexZ < static_cast<std::size_t>(chunkSizeZ));
            std::size_t subchunkIndexX = indexX / subchunkSizeXYZ;
            std::size_t subchunkIndexY = indexY / subchunkSizeXYZ;
            std::size_t subchunkIndexZ = indexZ / subchunkSizeXYZ;
            std::size_t subchunkOffsetX = indexX % subchunkSizeXYZ;
            std::size_t subchunkOffsetY = indexY % subchunkSizeXYZ;
            std::size_t subchunkOffsetZ = indexZ % subchunkSizeXYZ;
            std::size_t retval = subchunkIndexY;
            retval = retval * subchunkCountX + subchunkIndexX;
            retval = retval * subchunkCountZ + subchunkIndexZ;
            retval = retval * subchunkSizeXYZ + subchunkOffsetX;
            retval = retval * subchunkSizeXYZ + subchunkOffsetY;
            retval = retval * subchunkSizeXYZ + subchunkOffsetZ;
            return retval;
        }
    public:
        class IndexHelper1;
        class IndexHelper2 final
        {
            friend class IndexHelper1;
        private:
            BlockType *blocks;
            std::size_t indexX;
            std::size_t indexY;
            IndexHelper2(BlockType *blocks, std::size_t indexX, std::size_t indexY)
                : blocks(blocks), indexX(indexX), indexY(indexY)
            {
            }
        public:
            BlockType &operator[](std::size_t indexZ)
            {
                return blocks[getIndex(indexX, indexY, indexZ)];
            }
            BlockType &at(std::size_t index)
            {
                assert(index < size());
                return operator [](index);
            }
            static std::size_t size()
            {
                return chunkSizeZ;
            }
        };
        class IndexHelper1 final
        {
            friend class BlocksArray;
        private:
            BlockType *blocks;
            std::size_t indexX;
            IndexHelper1(BlockType *blocks, std::size_t indexX)
                : blocks(blocks), indexX(indexX)
            {
            }
        public:
            IndexHelper2 operator[](std::size_t indexY)
            {
                return IndexHelper2(blocks, indexX, indexY);
            }
            IndexHelper2 at(std::size_t index)
            {
                assert(index < size());
                return operator [](index);
            }
            static std::size_t size()
            {
                return chunkSizeY;
            }
        };
        class ConstIndexHelper1;
        class ConstIndexHelper2 final
        {
            friend class ConstIndexHelper1;
        private:
            const BlockType *blocks;
            std::size_t indexX;
            std::size_t indexY;
            ConstIndexHelper2(const BlockType *blocks, std::size_t indexX, std::size_t indexY)
                : blocks(blocks), indexX(indexX), indexY(indexY)
            {
            }
        public:
            const BlockType &operator[](std::size_t indexZ)
            {
                return blocks[getIndex(indexX, indexY, indexZ)];
            }
            const BlockType &at(std::size_t index)
            {
                assert(index < size());
                return operator [](index);
            }
            static std::size_t size()
            {
                return chunkSizeZ;
            }
        };
        class ConstIndexHelper1 final
        {
            friend class BlocksArray;
        private:
            const BlockType *blocks;
            std::size_t indexX;
            ConstIndexHelper1(const BlockType *blocks, std::size_t indexX)
                : blocks(blocks), indexX(indexX)
            {
            }
        public:
            ConstIndexHelper2 operator[](std::size_t indexY)
            {
                return ConstIndexHelper2(blocks, indexX, indexY);
            }
            ConstIndexHelper2 at(std::size_t index)
            {
                assert(index < size());
                return operator [](index);
            }
            static std::size_t size()
            {
                return chunkSizeY;
            }
        };
        ConstIndexHelper1 operator[](std::size_t indexX) const
        {
            return ConstIndexHelper1(blocks, indexX);
        }
        IndexHelper1 operator[](std::size_t indexX)
        {
            return IndexHelper1(blocks, indexX);
        }
        ConstIndexHelper1 at(std::size_t index) const
        {
            assert(index < size());
            return operator [](index);
        }
        IndexHelper1 at(std::size_t index)
        {
            assert(index < size());
            return operator [](index);
        }
        static std::size_t size()
        {
            return chunkSizeX;
        }
    };
    BlocksArray blocks;
    checked_array<checked_array<checked_array<SubchunkType, subchunkCountZ>, subchunkCountY>, subchunkCountX> subchunks;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename BlockChunk>
class BasicBlockChunkRelativePositionIterator final : public std::iterator<std::forward_iterator_tag, const VectorI>
{
#pragma GCC diagnostic pop
    VectorI pos;
    VectorI subchunkBasePos;
    VectorI subchunkRelativePos;
    constexpr BasicBlockChunkRelativePositionIterator(VectorI pos, VectorI subchunkBasePos, VectorI subchunkRelativePos)
        : pos(pos), subchunkBasePos(subchunkBasePos), subchunkRelativePos(subchunkRelativePos)
    {
    }
public:
    constexpr BasicBlockChunkRelativePositionIterator()
    {
    }
    static constexpr BasicBlockChunkRelativePositionIterator begin()
    {
        return BasicBlockChunkRelativePositionIterator(VectorI(0), VectorI(0), VectorI(0));
    }
    static constexpr BasicBlockChunkRelativePositionIterator end()
    {
        return BasicBlockChunkRelativePositionIterator(VectorI(0, 0, BlockChunk::chunkSizeZ), VectorI(0, 0, BlockChunk::chunkSizeZ), VectorI(0));
    }
    constexpr bool operator ==(const BasicBlockChunkRelativePositionIterator &rt) const
    {
        return pos == rt.pos;
    }
    constexpr bool operator !=(const BasicBlockChunkRelativePositionIterator &rt) const
    {
        return pos != rt.pos;
    }
    constexpr const VectorI &operator *() const
    {
        return pos;
    }
    constexpr const VectorI *operator ->() const
    {
        return &pos;
    }
    const BasicBlockChunkRelativePositionIterator &operator ++()
    {
        subchunkRelativePos.x++;
        if(subchunkRelativePos.x >= BlockChunk::subchunkSizeXYZ)
        {
            subchunkRelativePos.x = 0;
            subchunkRelativePos.y++;
            if(subchunkRelativePos.y >= BlockChunk::subchunkSizeXYZ)
            {
                subchunkRelativePos.y = 0;
                subchunkRelativePos.z++;
                if(subchunkRelativePos.z >= BlockChunk::subchunkSizeXYZ)
                {
                    subchunkRelativePos.z = 0;
                    subchunkBasePos.x += BlockChunk::subchunkSizeXYZ;
                    if(subchunkBasePos.x >= BlockChunk::chunkSizeX)
                    {
                        subchunkBasePos.x = 0;
                        subchunkBasePos.y += BlockChunk::subchunkSizeXYZ;
                        if(subchunkBasePos.y >= BlockChunk::chunkSizeY)
                        {
                            subchunkBasePos.y = 0;
                            subchunkBasePos.z += BlockChunk::subchunkSizeXYZ;
                        }
                    }
                }
            }
        }
        pos = subchunkBasePos + subchunkRelativePos;
        return *this;
    }
    BasicBlockChunkRelativePositionIterator operator ++(int)
    {
        BasicBlockChunkRelativePositionIterator retval = *this;
        operator ++();
        return retval;
    }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <typename ChunkType>
class ChunkMap
{
public:
    typedef ChunkType value_type;
private:
    struct Node
    {
        Node(const Node &) = delete;
        Node &operator =(const Node &) = delete;
        value_type value;
        Node *hash_next;
        const std::size_t cachedHash;
        Node(PositionI chunkBasePosition, std::size_t cachedHash)
            : value(chunkBasePosition), hash_next(), cachedHash(cachedHash)
        {
        }
    };

    std::size_t bucket_count = 0;
    std::vector<Node *> buckets;
    std::shared_ptr<std::vector<std::recursive_mutex>> bucket_locks = nullptr;
    std::hash<PositionI> the_hasher;

    static bool is_prime(std::size_t v)
    {
        if(v < 2)
        {
            return false;
        }

        if(v == 2 || v == 3 || v == 5 || v == 7 || v == 11 || v == 13) // all primes less than 17
        {
            return true;
        }

        if(v % 2 == 0 || v % 3 == 0 || v % 5 == 0 || v % 7 == 0 || v % 11 == 0 || v % 13 == 0)
        {
            return false;
        }

        if(v < 17 * 17) // if sqrt(v) < 17 then v is a prime cuz we checked all smaller primes
        {
            return true;
        }

        for(std::size_t divisor = 17; divisor * divisor <= v; divisor += 2)
        {
            if(v % divisor == 0)
            {
                return false;
            }
        }

        return true;
    }
    static std::size_t prime_ceiling(std::size_t v)
    {
        if(is_prime(v))
        {
            return v;
        }

        if(v % 2 == 0)
        {
            v++;
        }

        while(!is_prime(v))
        {
            v += 2;
        }

        return v;
    }
    struct iterator_imp final
    {
        Node *node;
        std::size_t bucket_index;
        std::size_t bucket_count;
        std::vector<Node *> *pbuckets;
        std::shared_ptr<std::vector<std::recursive_mutex>> bucket_locks;
        bool locked;
        iterator_imp(Node *node, std::size_t bucket_index, ChunkMap *parent_map, bool locked)
            : node(node), bucket_index(bucket_index), bucket_count(parent_map->bucket_count), pbuckets(&parent_map->buckets), bucket_locks(parent_map->bucket_locks), locked(locked)
        {
        }
        iterator_imp(ChunkMap *parent_map)
            : node(nullptr), bucket_index(0), bucket_count(parent_map->bucket_count), pbuckets(&parent_map->buckets), bucket_locks(parent_map->bucket_locks), locked(false)
        {
            lock(false);
            node = (*pbuckets)[bucket_index];
            while(node == nullptr)
            {
                unlock();
                bucket_index++;
                if(bucket_index >= bucket_count)
                {
                    node = nullptr;
                    bucket_index = 0;
                    bucket_count = 0;
                    pbuckets = nullptr;
                    bucket_locks = nullptr;
                    return;
                }
                lock(false);
                node = (*pbuckets)[bucket_index];
            }
        }
        constexpr iterator_imp()
            : node(nullptr), bucket_index(0), bucket_count(0), pbuckets(nullptr), bucket_locks(nullptr), locked(false)
        {
        }
        iterator_imp(const iterator_imp &rt)
            : node(rt.node), bucket_index(rt.bucket_index), bucket_count(rt.bucket_count), pbuckets(rt.pbuckets), bucket_locks(rt.bucket_locks), locked(false)
        {
        }
        iterator_imp(iterator_imp &&rt)
            : node(rt.node), bucket_index(rt.bucket_index), bucket_count(rt.bucket_count), pbuckets(rt.pbuckets), bucket_locks(rt.bucket_locks), locked(rt.locked)
        {
            rt.locked = false;
        }
        const iterator_imp &operator =(const iterator_imp &rt)
        {
            if(this == &rt)
                return *this;
            unlock();
            node = rt.node;
            bucket_index = rt.bucket_index;
            bucket_count = rt.bucket_count;
            pbuckets = rt.pbuckets;
            bucket_locks = rt.bucket_locks;
            return *this;
        }
        const iterator_imp &operator =(iterator_imp &&rt)
        {
            using std::swap;
            swap(node, rt.node);
            swap(bucket_index, rt.bucket_index);
            swap(bucket_count, rt.bucket_count);
            swap(pbuckets, rt.pbuckets);
            swap(bucket_locks, rt.bucket_locks);
            swap(locked, rt.locked);
            return *this;
        }
        ~iterator_imp()
        {
            unlock();
        }
        void lock(bool checkNode)
        {
            assert(!checkNode || node != nullptr);
            if(!locked)
            {
                (*bucket_locks)[bucket_index].lock();
                locked = true;
            }
        }
        void unlock()
        {
            if(locked)
                (*bucket_locks)[bucket_index].unlock();
            locked = false;
        }
        bool try_lock()
        {
            assert(node != nullptr);
            if(!locked)
            {
                if((*bucket_locks)[bucket_index].try_lock())
                {
                    locked = true;
                    return true;
                }
                return false;
            }
            return true;
        }
        void operator ++()
        {
            if(node == nullptr)
                return;
            lock(false); // just checked
            node = node->hash_next;
            while(node == nullptr)
            {
                unlock();
                bucket_index++;
                if(bucket_index >= bucket_count)
                {
                    node = nullptr;
                    bucket_index = 0;
                    bucket_count = 0;
                    pbuckets = nullptr;
                    bucket_locks = nullptr;
                    return;
                }
                lock(false);
                node = (*pbuckets)[bucket_index];
            }
        }
        void operator ++(int)
        {
            operator ++();
        }
        bool operator ==(const iterator_imp &rt) const
        {
            return node == rt.node;
        }
        bool operator !=(const iterator_imp &rt) const
        {
            return !operator ==(rt);
        }
        value_type &operator *()
        {
            lock(true);
            return node->value;
        }
        value_type *operator ->()
        {
            lock(true);
            return &node->value;
        }
    };
public:
    struct iterator final : public std::iterator<std::forward_iterator_tag, value_type>
    {
        friend class ChunkMap;
        friend class const_iterator;
    private:
        iterator_imp imp;
        constexpr iterator(iterator_imp imp)
            : imp(std::move(imp))
        {
        }
        Node *get_node()
        {
            return imp.node;
        }
        std::size_t get_bucket_index()
        {
            return imp.bucket_index;
        }
    public:
        constexpr iterator()
        {
        }
        void lock()
        {
            imp.lock(true);
        }
        void unlock()
        {
            imp.unlock();
        }
        bool try_lock()
        {
            return imp.try_lock();
        }
        bool is_locked() const
        {
            return imp.locked;
        }
        typename ChunkMap::value_type &operator *()
        {
            return *imp;
        }
        typename ChunkMap::value_type *operator ->()
        {
            return &operator *();
        }
        bool operator ==(const iterator &rt) const
        {
            return imp == rt.imp;
        }
        bool operator !=(const iterator &rt) const
        {
            return imp != rt.imp;
        }
        iterator &operator ++()
        {
            ++imp;
            return *this;
        }
        iterator operator ++(int)
        {
            iterator retval = *this;
            ++imp;
            return retval;
        }
    };
    struct const_iterator final : public std::iterator<std::forward_iterator_tag, const value_type>
    {
        friend class ChunkMap;
    private:
        iterator_imp imp;
        constexpr const_iterator(iterator_imp imp)
            : imp(std::move(imp))
        {
        }
        Node *get_node()
        {
            return imp.node;
        }
        std::size_t get_bucket_index()
        {
            return imp.bucket_index;
        }
    public:
        constexpr const_iterator()
        {
        }
        constexpr const_iterator(const iterator &rt)
            : imp(rt.imp)
        {
        }
        constexpr const_iterator(iterator &&rt)
            : imp(std::move(rt.imp))
        {
        }
        void lock()
        {
            imp.lock(true);
        }
        void unlock()
        {
            imp.unlock();
        }
        bool try_lock()
        {
            return imp.try_lock();
        }
        bool is_locked() const
        {
            return imp.locked;
        }
        const typename ChunkMap::value_type &operator *()
        {
            return *imp;
        }
        const typename ChunkMap::value_type *operator ->()
        {
            return &operator *();
        }
        friend bool operator ==(const iterator &l, const const_iterator &r)
        {
            return const_iterator(l) == r;
        }
        friend bool operator !=(const iterator &l, const const_iterator &r)
        {
            return const_iterator(l) != r;
        }
        friend bool operator ==(const const_iterator &l, const iterator &r)
        {
            return l == const_iterator(r);
        }
        friend bool operator !=(const const_iterator &l, const iterator &r)
        {
            return l != const_iterator(r);
        }
        friend bool operator ==(const const_iterator &l, const const_iterator &r)
        {
            return l.imp == r.imp;
        }
        friend bool operator !=(const const_iterator &l, const const_iterator &r)
        {
            return l.imp != r.imp;
        }
        const_iterator &operator ++()
        {
            ++imp;
            return *this;
        }
        const_iterator operator ++(int)
        {
            iterator retval = *this;
            ++imp;
            return retval;
        }
    };
    explicit ChunkMap(std::size_t n)
        : bucket_count(prime_ceiling(n)), buckets(), the_hasher()
    {
        buckets.resize(bucket_count);
        for(std::size_t i = 0; i < bucket_count; i++)
            buckets[i] = nullptr;
        bucket_locks = std::make_shared<std::vector<std::recursive_mutex>>(bucket_count);
    }
    explicit ChunkMap()
        : ChunkMap(8191)
    {
    }
    ChunkMap(const ChunkMap &r, std::size_t bucket_count)
        : bucket_count(bucket_count), buckets(), the_hasher(r.the_hasher)
    {
        buckets.resize(bucket_count);
        for(std::size_t i = 0; i < bucket_count; i++)
            buckets[i] = nullptr;
        bucket_locks = std::make_shared<std::vector<std::recursive_mutex>>(bucket_count);

        for(const_iterator i = r.begin(); i != r.end(); ++i)
        {
            Node *new_node = new Node(*i.imp.node);
            std::size_t current_hash = new_node->cachedHash % bucket_count;
            new_node->hash_next = buckets[current_hash];
            buckets[current_hash] = new_node;
        }
    }
    ChunkMap(const ChunkMap &r)
        : ChunkMap(r, r.bucket_count)
    {
    }
    ~ChunkMap()
    {
        clear();
    }
    void clear()
    {
        for(std::size_t i = 0; i < bucket_count; i++)
        {
            std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[i]);
            while(buckets[i] != nullptr)
            {
                Node *delete_me = buckets[i];
                buckets[i] = delete_me->hash_next;
                delete delete_me;
            }
        }
    }
    const ChunkMap &operator =(const ChunkMap &r)
    {
        if(bucket_locks == r.bucket_locks)
        {
            return *this;
        }

        clear();

        if(r.bucket_count != bucket_count)
        {
            bucket_count = r.bucket_count;
            buckets.assign(bucket_count);
            bucket_locks = std::make_shared<std::vector<std::recursive_mutex>>(bucket_count);
        }

        for(std::size_t i = 0; i < bucket_count; i++)
        {
            buckets[i] = nullptr;
        }

        for(const_iterator i = r.begin(); i != r.end(); ++i)
        {
            Node *new_node = new Node(*i.imp.node);
            std::size_t current_hash = new_node->cachedHash % bucket_count;
            new_node->hash_next = buckets[current_hash];
            buckets[current_hash] = new_node;
        }

        return *this;
    }
    void swap(ChunkMap &r)
    {
        using std::swap;
        swap(bucket_count, r.bucket_count);
        swap(buckets, r.buckets);
        swap(bucket_locks, r.bucket_locks);
    }
    const ChunkMap &operator =(ChunkMap &&r)
    {
        swap(r);
        return *this;
    }
    iterator begin()
    {
        return iterator(iterator_imp(this));
    }
    iterator end()
    {
        return iterator(iterator_imp());
    }
    constexpr const_iterator begin() const
    {
        return const_iterator(iterator_imp(const_cast<ChunkMap *>(this)));
    }
    constexpr const_iterator end() const
    {
        return const_iterator(iterator_imp());
    }
    constexpr const_iterator cbegin() const
    {
        return const_iterator(iterator_imp(const_cast<ChunkMap *>(this)));
    }
    constexpr const_iterator cend() const
    {
        return const_iterator(iterator_imp());
    }
    iterator find(PositionI key)
    {
        std::size_t current_hash = (std::size_t)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(retval->value.basePosition == key)
            {
                lock_it.release();
                return iterator(iterator_imp(retval, current_hash, this, true));
            }
        }
        return end();
    }
    std::pair<iterator, bool> create(PositionI key)
    {
        std::size_t key_hash = (std::size_t)the_hasher(key);
        std::size_t current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(retval->value.basePosition == key)
            {
                lock_it.release();
                return std::pair<iterator, bool>(iterator(iterator_imp(retval, current_hash, this, true)), false);
            }
        }

        Node *retval = new Node(key, key_hash);
        retval->hash_next = buckets[current_hash];
        buckets[current_hash] = retval;
        lock_it.release();
        return std::pair<iterator, bool>(iterator(iterator_imp(retval, current_hash, this, true)), true);
    }
    const_iterator find(PositionI key) const
    {
        std::size_t current_hash = (std::size_t)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(retval->value.basePosition == key)
            {
                lock_it.release();
                return const_iterator(iterator_imp(retval, current_hash, const_cast<ChunkMap *>(this), true));
            }
        }
        return cend();
    }
    value_type &operator [](PositionI key)
    {
        std::size_t key_hash = (std::size_t)the_hasher(key);
        std::size_t current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(retval->value.basePosition == key)
            {
                return retval->value;
            }
        }

        Node *retval = new Node(key, key_hash);
        retval->hash_next = buckets[current_hash];
        buckets[current_hash] = retval;
        return retval->value;
    }
    value_type &at(PositionI key)
    {
        std::size_t key_hash = (std::size_t)the_hasher(key);
        std::size_t current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(retval->value.basePosition == key)
            {
                return retval->value;
            }
        }
        throw std::out_of_range("key doesn't exist in ChunkMap<ChunkType>::at(PositionI)");
    }
    const value_type &at(PositionI key) const
    {
        std::size_t key_hash = (std::size_t)the_hasher(key);
        std::size_t current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return retval->value;
            }
        }
        throw std::out_of_range("key doesn't exist in ChunkMap<ChunkType>::at(PositionI)");
    }
    std::size_t erase(PositionI key)
    {
        std::size_t current_hash = (std::size_t)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it((*bucket_locks)[current_hash]);
        Node **pnode = &buckets[current_hash];
        for(Node *node = *pnode; node != nullptr; pnode = &node->hash_next, node = *pnode)
        {
            if(node->value.basePosition == key)
            {
                *pnode = node->hash_next;
                delete node;
                return 1;
            }
        }
        return 0;
    }
    iterator erase(const_iterator position)
    {
        Node *node = position.get_node();
        std::size_t current_hash = position.get_bucket_index();

        if(node == nullptr)
        {
            return end();
        }

        iterator retval = iterator(std::move(position.imp));
        ++retval;
        std::unique_lock<std::recursive_mutex> lock_it;
        if(retval.get_bucket_index() != current_hash || retval == end())
            lock_it = std::unique_lock<std::recursive_mutex>((*bucket_locks)[current_hash]);
        else if(!retval.is_locked())
            retval.lock();
        for(Node **pnode = &buckets[current_hash]; *pnode != nullptr; pnode = &(*pnode)->hash_next)
        {
            if(*pnode == node)
            {
                *pnode = node->hash_next;
                break;
            }
        }
        delete node;
        return std::move(retval);
    }
    std::size_t count(PositionI key) const
    {
        return find(key) != end() ? 1 : 0;
    }
    std::unique_lock<std::recursive_mutex> bucket_lock(PositionI key)
    {
        std::size_t current_hash = (std::size_t)the_hasher(key) % bucket_count;
        return std::unique_lock<std::recursive_mutex>((*bucket_locks)[current_hash]);
    }
    class node_ptr final
    {
        friend class ChunkMap;
    private:
        Node *node;
        void reset(Node *newNode)
        {
            if(node != nullptr)
                delete node;
            node = newNode;
        }
        Node *release()
        {
            Node *retval = node;
            node = nullptr;
            return retval;
        }
        Node *get() const
        {
            return node;
        }
    public:
        constexpr node_ptr()
            : node(nullptr)
        {
        }
        constexpr node_ptr(std::nullptr_t)
            : node(nullptr)
        {
        }
        node_ptr(node_ptr &&rt)
            : node(rt.node)
        {
            rt.node = nullptr;
        }
        ~node_ptr()
        {
            reset();
        }
        node_ptr &operator =(node_ptr &&rt)
        {
            reset(rt.release());
            return *this;
        }
        node_ptr &operator =(std::nullptr_t)
        {
            reset();
            return *this;
        }
        void reset()
        {
            reset(nullptr);
        }
        void swap(node_ptr &rt)
        {
            Node *temp = node;
            node = rt.node;
            rt.node = temp;
        }
        explicit operator bool() const
        {
            return node != nullptr;
        }
        bool operator !() const
        {
            return node == nullptr;
        }
        typename ChunkMap::value_type &operator *() const
        {
            return node->value;
        }
        typename ChunkMap::value_type *operator ->() const
        {
            return node->value;
        }
        friend bool operator ==(std::nullptr_t, const node_ptr &v)
        {
            return v.node == nullptr;
        }
        friend bool operator ==(const node_ptr &v, std::nullptr_t)
        {
            return v.node == nullptr;
        }
        friend bool operator !=(std::nullptr_t, const node_ptr &v)
        {
            return v.node != nullptr;
        }
        friend bool operator !=(const node_ptr &v, std::nullptr_t)
        {
            return v.node != nullptr;
        }
        bool operator ==(const node_ptr &rt) const
        {
            return node == rt.node;
        }
        bool operator !=(const node_ptr &rt) const
        {
            return node != rt.node;
        }
    };
    struct insert_result final
    {
        iterator position;
        bool inserted;
        node_ptr node;
    };
    insert_result insert(node_ptr node)
    {
        insert_result retval;
        retval.node = std::move(node);
        if(retval.node == nullptr)
        {
            retval.position = end();
            retval.inserted = false;
            return std::move(retval);
        }
        Node *pnode = retval.node.get();
        PositionI key = pnode->value.basePosition;
        std::size_t key_hash = (std::size_t)the_hasher(key);
        pnode->cachedHash = key_hash;
        std::size_t current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *pretval = buckets[current_hash]; pretval != nullptr; pretval = pretval->hash_next)
        {
            if(pretval->value.basePosition == key)
            {
                lock_it.release();
                retval.position = iterator(iterator_imp(pretval, current_hash, this, true));
                retval.inserted = false;
                return std::move(retval);
            }
        }

        Node *pretval = retval.node.release();
        pretval->hash_next = buckets[current_hash];
        buckets[current_hash] = pretval;
        lock_it.release();
        retval.position = iterator(iterator_imp(pretval, current_hash, this, true));
        retval.inserted = true;
        return std::move(retval);
    }
    node_ptr extract(const_iterator position)
    {
        Node *node = position.get_node();
        std::size_t current_hash = position.get_bucket_index();

        if(node == nullptr)
        {
            return node_ptr();
        }

        position.lock();
        for(Node **pnode = &buckets[current_hash]; *pnode != nullptr; pnode = &(*pnode)->hash_next)
        {
            if(*pnode == node)
            {
                *pnode = node->hash_next;
                break;
            }
        }
        node->hash_next = nullptr;
        return node_ptr(node);
    }
    node_ptr extract(PositionI key)
    {
        std::size_t current_hash = (std::size_t)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        Node **pnode = &buckets[current_hash];
        for(Node *node = *pnode; node != nullptr; pnode = &node->hash_next, node = *pnode)
        {
            if(node->value.basePosition == key)
            {
                *pnode = node->hash_next;
                node->hash_next = nullptr;
                return node_ptr(node);
            }
        }
        return node_ptr();
    }
};
#pragma GCC diagnostic pop
}
}

#endif // BASIC_BLOCK_CHUNK_H_INCLUDED
