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
#ifndef PARALLEL_MAP_H_INCLUDED
#define PARALLEL_MAP_H_INCLUDED

#include <iterator>
#include <utility>
#include <tuple>
#include <stdexcept>
#include <cstddef>
#include <mutex>
#include <memory>
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
class parallel_map
{
public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef Hash hasher;
    typedef Pred key_equal;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
private:
    struct Node
    {
        value_type value;
        Node *hash_next;
        size_type cachedHash;
        Node(const key_type &key, const mapped_type &value, size_type cachedHash)
            : value(key, value), cachedHash(cachedHash)
        {
        }
        Node(const key_type &key, size_type cachedHash)
            : value(key, mapped_type()), cachedHash(cachedHash)
        {
        }
        Node(const value_type &kv_pair, size_type cachedHash)
            : value(kv_pair), cachedHash(cachedHash)
        {
        }
        Node(const key_type &key, mapped_type &&value, size_type cachedHash)
            : value(key, std::move(value)), cachedHash(cachedHash)
        {
        }
    };

    size_type bucket_count = 0;
    Node **buckets = nullptr;
    std::shared_ptr<std::recursive_mutex> bucket_locks = nullptr;
    hasher the_hasher;
    key_equal the_comparer;

    static bool is_prime(size_type v)
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

        for(size_type divisor = 17; divisor * divisor <= v; divisor += 2)
        {
            if(v % divisor == 0)
            {
                return false;
            }
        }

        return true;
    }
    static size_type prime_ceiling(size_type v)
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
        size_type bucket_index;
        size_type bucket_count;
        Node **buckets;
        std::shared_ptr<std::recursive_mutex> bucket_locks;
        bool locked;
        iterator_imp(Node *node, size_type bucket_index, const parallel_map *parent_map, bool locked)
            : node(node), bucket_index(bucket_index), bucket_count(parent_map->bucket_count), buckets(parent_map->buckets), bucket_locks(parent_map->bucket_locks), locked(locked)
        {
        }
        iterator_imp(const parallel_map *parent_map)
            : node(nullptr), bucket_index(0), bucket_count(parent_map->bucket_count), buckets(parent_map->buckets), bucket_locks(parent_map->bucket_locks), locked(false)
        {
            lock();
            node = buckets[bucket_index];
            while(node == nullptr)
            {
                unlock();
                bucket_index++;
                if(bucket_index >= bucket_count)
                {
                    node = nullptr;
                    bucket_index = 0;
                    bucket_count = 0;
                    buckets = nullptr;
                    bucket_locks = nullptr;
                    return;
                }
                lock();
                node = buckets[bucket_index];
            }
        }
        constexpr iterator_imp()
            : node(nullptr), bucket_index(0), bucket_count(0), buckets(nullptr), bucket_locks(nullptr), locked(false)
        {
        }
        iterator_imp(const iterator_imp &rt)
            : node(rt.node), bucket_index(rt.bucket_index), bucket_count(rt.bucket_count), buckets(rt.buckets), bucket_locks(rt.bucket_locks), locked(false)
        {
        }
        iterator_imp(iterator_imp &&rt)
            : node(rt.node), bucket_index(rt.bucket_index), bucket_count(rt.bucket_count), buckets(rt.buckets), bucket_locks(rt.bucket_locks), locked(rt.locked)
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
            buckets = rt.buckets;
            bucket_locks = rt.bucket_locks;
            return *this;
        }
        const iterator_imp &operator =(iterator_imp &&rt)
        {
            using std::swap;
            swap(node, rt.node);
            swap(bucket_index, rt.bucket_index);
            swap(bucket_count, rt.bucket_count);
            swap(buckets, rt.buckets);
            swap(bucket_locks, rt.bucket_locks);
            swap(locked, rt.locked);
            return *this;
        }
        ~iterator_imp()
        {
            unlock();
        }
        std::recursive_mutex &get_lock()
        {
            return bucket_locks.get()[bucket_index];
        }
        void lock()
        {
            if(node && !locked)
            {
                get_lock().lock();
                locked = true;
            }
        }
        void unlock()
        {
            if(locked)
                get_lock().unlock();
            locked = false;
        }
        bool try_lock()
        {
            if(node && !locked)
            {
                if(get_lock().try_lock())
                {
                    locked = true;
                    return true;
                }
                return false;
            }
            return true;
        }
GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
        void operator ++()
        {
GCC_PRAGMA(diagnostic pop)
            if(node == nullptr)
                return;
            lock();
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
                    buckets = nullptr;
                    bucket_locks = nullptr;
                    return;
                }
                lock();
                node = buckets[bucket_index];
            }
        }
GCC_PRAGMA(diagnostic push)
GCC_PRAGMA(diagnostic ignored "-Weffc++")
        void operator ++(int)
        {
GCC_PRAGMA(diagnostic pop)
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
            lock();
            return node->value;
        }
        value_type *operator ->()
        {
            lock();
            return &node->value;
        }
    };
public:
    struct iterator final : public std::iterator<std::forward_iterator_tag, value_type>
    {
        friend class parallel_map;
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
        size_type get_bucket_index()
        {
            return imp.bucket_index;
        }
    public:
        constexpr iterator()
        {
        }
        void lock()
        {
            imp.lock();
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
        typename parallel_map::value_type &operator *()
        {
            return *imp;
        }
        typename parallel_map::value_type *operator ->()
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
        friend class parallel_map;
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
        size_type get_bucket_index()
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
            imp.lock();
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
        const typename parallel_map::value_type &operator *()
        {
            return *imp;
        }
        const typename parallel_map::value_type *operator ->()
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
    explicit parallel_map(size_type n, const hasher &hf = hasher(), const key_equal &eql = key_equal())
        : bucket_count(prime_ceiling(n)), the_hasher(hf), the_comparer(eql)
    {
        buckets = new Node *[bucket_count];
        for(size_type i = 0; i < bucket_count; i++)
            buckets[i] = nullptr;
        bucket_locks = std::shared_ptr<std::recursive_mutex>(new std::recursive_mutex[bucket_count], [](std::recursive_mutex *v)
        {
            delete []v;
        });
    }
    explicit parallel_map(const hasher &hf = hasher(), const key_equal &eql = key_equal())
        : parallel_map(8191, hf, eql)
    {
    }
    parallel_map(const parallel_map &r, size_type bucket_count)
        : bucket_count(bucket_count), the_hasher(r.the_hasher), the_comparer(r.the_comparer)
    {
        buckets = new Node *[bucket_count];
        for(size_type i = 0; i < bucket_count; i++)
            buckets[i] = nullptr;
        bucket_locks = std::shared_ptr<std::recursive_mutex>(new std::recursive_mutex[bucket_count], [](std::recursive_mutex *v)
        {
            delete []v;
        });

        for(const_iterator i = r.begin(); i != r.end(); ++i)
        {
            Node *new_node = new Node(*i.imp.node);
            size_type current_hash = new_node->cachedHash % bucket_count;
            new_node->hash_next = buckets[current_hash];
            buckets[current_hash] = new_node;
        }
    }
    parallel_map(const parallel_map &r)
        : parallel_map(r, r.bucket_count)
    {
    }
    ~parallel_map()
    {
        clear();

        delete []buckets;
    }
    void clear()
    {
        for(size_type i = 0; i < bucket_count; i++)
        {
            std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[i]);
            while(buckets[i] != nullptr)
            {
                Node *delete_me = buckets[i];
                buckets[i] = delete_me->hash_next;
                delete delete_me;
            }
        }
    }
    const parallel_map &operator =(const parallel_map &r)
    {
        if(buckets == r.buckets)
        {
            return *this;
        }

        clear();

        if(r.bucket_count != bucket_count)
        {
            delete []buckets;
            bucket_count = r.bucket_count;
            buckets = new Node *[bucket_count];
            bucket_locks = std::shared_ptr<std::recursive_mutex>(new std::recursive_mutex[bucket_count], [](std::recursive_mutex *v)
            {
                delete []v;
            });
        }

        for(size_type i = 0; i < bucket_count; i++)
        {
            buckets[i] = nullptr;
        }

        for(const_iterator i = r.begin(); i != r.end(); ++i)
        {
            Node *new_node = new Node(*i.imp.node);
            size_type current_hash = new_node->cachedHash % bucket_count;
            new_node->hash_next = buckets[current_hash];
            buckets[current_hash] = new_node;
        }

        return *this;
    }
    void swap(parallel_map &r)
    {
        using std::swap;
        swap(bucket_count, r.bucket_count);
        swap(buckets, r.buckets);
        swap(bucket_locks, r.bucket_locks);
    }
    const parallel_map &operator =(parallel_map && r)
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
        return const_iterator(iterator_imp(this));
    }
    constexpr const_iterator end() const
    {
        return const_iterator(iterator_imp());
    }
    constexpr const_iterator cbegin() const
    {
        return const_iterator(iterator_imp(this));
    }
    constexpr const_iterator cend() const
    {
        return const_iterator(iterator_imp());
    }
    iterator find(const key_type &key)
    {
        size_type current_hash = (size_type)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                lock_it.release();
                return iterator(iterator_imp(retval, current_hash, this, true));
            }
        }
        return end();
    }
    std::pair<iterator, bool> insert(const value_type &kv_pair)
    {
        const key_type &key = std::get<0>(kv_pair);
        size_type key_hash = (size_type)the_hasher(key);
        size_type current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                lock_it.release();
                return std::pair<iterator, bool>(iterator(iterator_imp(retval, current_hash, this, true)), false);
            }
        }

        Node *retval = new Node(kv_pair, key_hash);
        retval->hash_next = buckets[current_hash];
        buckets[current_hash] = retval;
        lock_it.release();
        return std::pair<iterator, bool>(iterator(iterator_imp(retval, current_hash, this, true)), true);
    }
    const_iterator find(const key_type &key) const
    {
        size_type current_hash = (size_type)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                lock_it.release();
                return const_iterator(iterator_imp(retval, current_hash, this, true));
            }
        }
        return cend();
    }
    mapped_type &operator [](const key_type &key)
    {
        size_type key_hash = (size_type)the_hasher(key);
        size_type current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return std::get<1>(retval->value);
            }
        }

        Node *retval = new Node(key, key_hash);
        retval->hash_next = buckets[current_hash];
        buckets[current_hash] = retval;
        return std::get<1>(retval->value);
    }
    mapped_type &at(const key_type &key)
    {
        size_type key_hash = (size_type)the_hasher(key);
        size_type current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return std::get<1>(retval->value);
            }
        }
        throw std::out_of_range("key doesn't exist in parallel_map<KeyType, ValueType>::at(const KeyType&)");
    }
    const mapped_type &at(const key_type &key) const
    {
        size_type key_hash = (size_type)the_hasher(key);
        size_type current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *retval = buckets[current_hash]; retval != nullptr; retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return std::get<1>(retval->value);
            }
        }
        throw std::out_of_range("key doesn't exist in parallel_map<KeyType, ValueType>::at(const KeyType&)");
    }
    size_type erase(const key_type &key)
    {
        size_type current_hash = (size_type)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        Node **pnode = &buckets[current_hash];
        for(Node *node = *pnode; node != nullptr; pnode = &node->hash_next, node = *pnode)
        {
            if(the_comparer(std::get<0>(node->value), key))
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
        size_type current_hash = position.get_bucket_index();

        if(node == nullptr)
        {
            return end();
        }

        iterator retval = iterator(std::move(position.imp));
        ++retval;
        std::unique_lock<std::recursive_mutex> lock_it;
        if(retval.get_bucket_index() != current_hash || retval == end())
            lock_it = std::unique_lock<std::recursive_mutex>(bucket_locks.get()[current_hash]);
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
    size_type count(const key_type &key) const
    {
        return find(key) != end() ? 1 : 0;
    }
    std::unique_lock<std::recursive_mutex> bucket_lock(const key_type &key)
    {
        size_type current_hash = (size_type)the_hasher(key) % bucket_count;
        return std::unique_lock<std::recursive_mutex>(bucket_locks.get()[current_hash]);
    }
    class node_ptr final
    {
        friend class parallel_map;
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
        typename parallel_map::value_type &operator *() const
        {
            return node->value;
        }
        typename parallel_map::value_type *operator ->() const
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
        const key_type &key = std::get<0>(pnode->value);
        size_type key_hash = (size_type)the_hasher(key);
        pnode->cachedHash = key_hash;
        size_type current_hash = key_hash % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        for(Node *pretval = buckets[current_hash]; pretval != nullptr; pretval = pretval->hash_next)
        {
            if(the_comparer(std::get<0>(pretval->value), key))
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
        size_type current_hash = position.get_bucket_index();

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
    node_ptr extract(const key_type &key)
    {
        size_type current_hash = (size_type)the_hasher(key) % bucket_count;
        std::unique_lock<std::recursive_mutex> lock_it(bucket_locks.get()[current_hash]);
        Node **pnode = &buckets[current_hash];
        for(Node *node = *pnode; node != nullptr; pnode = &node->hash_next, node = *pnode)
        {
            if(the_comparer(std::get<0>(node->value), key))
            {
                *pnode = node->hash_next;
                node->hash_next = nullptr;
                return node_ptr(node);
            }
        }
        return node_ptr();
    }
};
}
}

#endif // PARALLEL_MAP_H_INCLUDED
