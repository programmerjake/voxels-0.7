#ifndef LINKED_MAP_H_INCLUDED
#define LINKED_MAP_H_INCLUDED

#include <iterator>
#include <utility>
#include <tuple>
#include <stdexcept>
#include <cstddef>

template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
class linked_map
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
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
private:
    struct Node
    {
        value_type value;
        Node *hash_next;
        Node *list_next;
        Node *list_prev;
        const size_t cachedHash;
        Node(const key_type &key, const mapped_type &value, size_t cachedHash)
            : value(key, value), cachedHash(cachedHash)
        {
        }
        Node(const key_type &key, size_t cachedHash)
            : value(key, mapped_type()), cachedHash(cachedHash)
        {
        }
        Node(const value_type &kv_pair, size_t cachedHash)
            : value(kv_pair), cachedHash(cachedHash)
        {
        }
        Node(const key_type &key, mapped_type &&value, size_t cachedHash)
            : value(key, std::move(value)), cachedHash(cachedHash)
        {
        }
    };

    size_t bucket_count_;
    size_t node_count = 0;
    mutable Node **buckets = nullptr;
    Node *list_head = nullptr;
    Node *list_tail = nullptr;
    hasher the_hasher;
    key_equal the_comparer;

    static bool is_prime(size_t v)
    {
        if(v < 2)
            return false;
        if(v == 2 || v == 3 || v == 5 || v == 7 || v == 11 || v == 13) // all primes less than 17
            return true;
        if(v % 2 == 0 || v % 3 == 0 || v % 5 == 0 || v % 7 == 0 || v % 11 == 0 || v % 13 == 0)
            return false;
        if(v < 17 * 17) // if sqrt(v) < 17 then v is a prime cuz we checked all smaller primes
            return true;
        for(size_t divisor = 17; divisor * divisor <= v; divisor += 2)
        {
            if(v % divisor == 0)
                return false;
        }
        return true;
    }
    static size_t prime_ceiling(size_t v)
    {
        if(is_prime(v))
            return v;
        if(v % 2 == 0)
            v++;
        while(!is_prime(v))
            v += 2;
        return v;
    }
    void check_for_rehash()
    {
        if(node_count > 6 * bucket_count_)
        {
            rehash();
        }
    }
    void create_buckets() const
    {
        if(buckets != nullptr)
            return;
        buckets = new Node *[bucket_count_];
        for(size_t i = 0; i < bucket_count_; i++)
            buckets[i] = nullptr;
        for(Node *node = list_head; node != nullptr; node = node->list_next)
        {
            size_t current_hash = node->cachedHash % bucket_count_;
            node->hash_next = buckets[current_hash];
            buckets[current_hash] = node;
        }
    }
public:
    constexpr size_t bucket_count() const
    {
        return bucket_count_;
    }
    constexpr size_t size() const
    {
        return node_count;
    }
    void rehash(size_t new_bucket_count = 0)
    {
        size_t min_bucket_count = node_count / 4;
        if(min_bucket_count < 32)
            min_bucket_count = 32;
        if(new_bucket_count < min_bucket_count)
            new_bucket_count = min_bucket_count;
        new_bucket_count = prime_ceiling(new_bucket_count);
        if(new_bucket_count == bucket_count_)
            return;
        Node **new_hash_table = new Node *[new_bucket_count];
        for(size_t i = 0; i < new_bucket_count; i++)
        {
            new_hash_table[i] = nullptr;
        }
        for(Node *node = list_head; node != nullptr; node = node->list_next)
        {
            size_t current_hash = node->cachedHash % new_bucket_count;
            node->hash_next = new_hash_table[current_hash];
            new_hash_table[current_hash] = node;
        }
        if(buckets != nullptr)
            delete []buckets;
        buckets = new_hash_table;
        bucket_count_ = new_bucket_count;
    }
private:
    template <typename ValueT>
    struct iterator_base : public std::iterator<std::bidirectional_iterator_tag, ValueT>
    {
        friend class linked_map;
    protected:
        Node * pointer;
        const linked_map * container;
        constexpr iterator_base(Node * pointer, const linked_map * container)
            : pointer(pointer), container(container)
        {
        }
    public:
        constexpr iterator_base()
            : pointer(nullptr), container(nullptr)
        {
        }
        template <typename U>
        constexpr bool operator ==(iterator_base<U> r) const
        {
            return pointer == r.pointer;
        }
        template <typename U>
        constexpr bool operator !=(iterator_base<U> r) const
        {
            return pointer != r.pointer;
        }
        constexpr ValueT * operator ->() const
        {
            return &pointer->value;
        }
        constexpr ValueT & operator *() const
        {
            return pointer->value;
        }
        const iterator_base & operator ++()
        {
            if(pointer == nullptr)
                pointer = container->list_head;
            else
                pointer = pointer->list_next;
            return *this;
        }
        const iterator_base & operator --()
        {
            if(pointer == nullptr)
                pointer = container->list_tail;
            else
                pointer = pointer->list_prev;
            return *this;
        }
        const iterator_base operator ++(int)
        {
            Node * initial_pointer = pointer;
            if(pointer == nullptr)
                pointer = container->list_head;
            else
                pointer = pointer->list_next;
            return iterator_base(initial_pointer, container);
        }
        const iterator_base operator --(int)
        {
            Node * initial_pointer = pointer;
            if(pointer == nullptr)
                pointer = container->list_tail;
            else
                pointer = pointer->list_prev;
            return iterator_base(initial_pointer, container);
        }
    };
public:
    struct iterator final : public iterator_base<value_type>
    {
        friend class linked_map;
        constexpr iterator()
        {
        }
    private:
        constexpr iterator(Node * pointer, const linked_map * container)
            : iterator_base<value_type>(pointer, container)
        {
        }
    };
    struct const_iterator final : public iterator_base<const value_type>
    {
        friend class linked_map;
        constexpr const_iterator()
        {
        }
        constexpr const_iterator(iterator r)
            : const_iterator(r.pointer, r.container)
        {
        }
    private:
        constexpr const_iterator(Node * pointer, const linked_map * container)
            : iterator_base<const value_type>(pointer, container)
        {
        }
    };
    explicit linked_map(size_t n, const hasher &hf = hasher(), const key_equal &eql = key_equal())
        : bucket_count_(prime_ceiling(n)), the_hasher(hf), the_comparer(eql)
    {
    }
    linked_map()
        : linked_map(0)
    {
    }
    linked_map(const linked_map &r)
        : bucket_count_(r.bucket_count_), node_count(r.node_count), buckets(nullptr), list_head(nullptr), list_tail(nullptr), the_hasher(r.the_hasher), the_comparer(r.the_comparer)
    {
        buckets = new Node *[bucket_count_];
        for(size_t i = 0; i < bucket_count_; i++)
            buckets[i] = nullptr;
        for(const Node *node = r.list_head; node != nullptr; node = node->list_next)
        {
            Node *new_node = new Node(*node);
            new_node->list_next = nullptr;
            new_node->list_prev = list_tail;
            if(list_tail == nullptr)
                list_head = new_node;
            else
                list_tail->list_next = new_node;
            list_tail = new_node;
            size_t current_hash = new_node->cachedHash % bucket_count_;
            new_node->hash_next = buckets[current_hash];
            buckets[current_hash] = new_node;
        }
    }
    linked_map(linked_map &&r)
        : bucket_count_(r.bucket_count_), node_count(r.node_count), buckets(r.buckets), list_head(r.list_head), list_tail(r.list_tail), the_hasher(r.the_hasher), the_comparer(r.the_comparer)
    {
        r.node_count = 0;
        r.buckets = nullptr;
        r.list_head = nullptr;
        r.list_tail = nullptr;
    }
    ~linked_map()
    {
        clear();
        if(buckets != nullptr)
            delete []buckets;
    }
    void clear()
    {
        while(list_head != nullptr)
        {
            Node * deleteMe = list_head;
            list_head = list_head->list_next;
            delete deleteMe;
        }
        node_count = 0;
        list_tail = nullptr;
        if(buckets != nullptr)
            delete []buckets;
        buckets = nullptr;
    }
    const linked_map &operator =(const linked_map &r)
    {
        if(&r == this)
            return *this;
        clear();
        if(buckets != nullptr)
            delete []buckets;
        bucket_count_ = r.bucket_count_;
        node_count = r.node_count;
        buckets = new Node *[bucket_count_];
        for(size_t i = 0; i < bucket_count_; i++)
            buckets[i] = nullptr;
        for(const Node *node = r.list_head; node != nullptr; node = node->list_next)
        {
            Node *new_node = new Node(*node);
            new_node->list_next = nullptr;
            new_node->list_prev = list_tail;
            if(list_tail == nullptr)
                list_head = new_node;
            else
                list_tail->list_next = new_node;
            list_tail = new_node;
            size_t current_hash = new_node->cachedHash % bucket_count_;
            new_node->hash_next = buckets[current_hash];
            buckets[current_hash] = new_node;
        }
        return *this;
    }
    void swap(linked_map &r)
    {
        std::swap(bucket_count_, r.bucket_count_);
        std::swap(buckets, r.buckets);
        std::swap(node_count, r.node_count);
        std::swap(list_head, r.list_head);
        std::swap(list_tail, r.list_tail);
    }
    const linked_map &operator =(linked_map &&r)
    {
        swap(r);
        return *this;
    }
    constexpr bool empty() const
    {
        return node_count == 0;
    }
    iterator begin()
    {
        return iterator(list_head, this);
    }
    iterator end()
    {
        return iterator(nullptr, this);
    }
    constexpr const_iterator begin() const
    {
        return const_iterator(list_head, this);
    }
    constexpr const_iterator end() const
    {
        return const_iterator(nullptr, this);
    }
    constexpr const_iterator cbegin() const
    {
        return const_iterator(list_head, this);
    }
    constexpr const_iterator cend() const
    {
        return const_iterator(nullptr, this);
    }
    iterator find(const key_type &key)
    {
        create_buckets();
        size_t current_hash = (size_t)the_hasher(key) % bucket_count_;
        for(Node *retval = buckets[current_hash];retval != nullptr;retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return iterator(retval, this);
            }
        }
        return end();
    }
    std::pair<iterator, bool> insert(const value_type &kv_pair)
    {
        const key_type &key = std::get<0>(kv_pair);
        create_buckets();
        size_t key_hash = (size_t)the_hasher(key);
        size_t current_hash = key_hash % bucket_count_;
        for(Node *retval = buckets[current_hash];retval != nullptr;retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return std::pair<iterator, bool>(iterator(retval, this), false);
            }
        }
        Node *retval = new Node(kv_pair, key_hash);
        retval->hash_next = buckets[current_hash];
        buckets[current_hash] = retval;
        retval->list_next = nullptr;
        retval->list_prev = list_tail;
        if(list_tail == nullptr)
            list_head = retval;
        else
            list_tail->list_next = retval;
        list_tail = retval;
        node_count++;
        return std::pair<iterator, bool>(iterator(retval, this), true);
    }
    const_iterator find(const key_type &key) const
    {
        create_buckets();
        size_t current_hash = (size_t)the_hasher(key) % bucket_count_;
        for(Node *retval = buckets[current_hash];retval != nullptr;retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return const_iterator(retval, this);
            }
        }
        return end();
    }
    mapped_type &operator [](const key_type &key)
    {
        create_buckets();
        size_t key_hash = (size_t)the_hasher(key);
        size_t current_hash = key_hash % bucket_count_;
        for(Node *retval = buckets[current_hash];retval != nullptr;retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return std::get<1>(retval->value);
            }
        }
        Node *retval = new Node(key, key_hash);
        retval->hash_next = buckets[current_hash];
        buckets[current_hash] = retval;
        retval->list_next = nullptr;
        retval->list_prev = list_tail;
        if(list_tail == nullptr)
            list_head = retval;
        else
            list_tail->list_next = retval;
        list_tail = retval;
        node_count++;
        return std::get<1>(retval->value);
    }
    mapped_type &at(const key_type &key)
    {
        create_buckets();
        size_t key_hash = (size_t)the_hasher(key);
        size_t current_hash = key_hash % bucket_count_;
        for(Node *retval = buckets[current_hash];retval != nullptr;retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return std::get<1>(retval->value);
            }
        }
        throw std::out_of_range("key doesn't exist in linked_map<KeyType, ValueType>::at(const KeyType&)");
    }
    const mapped_type &at(const key_type &key) const
    {
        create_buckets();
        size_t key_hash = (size_t)the_hasher(key);
        size_t current_hash = key_hash % bucket_count_;
        for(Node *retval = buckets[current_hash];retval != nullptr;retval = retval->hash_next)
        {
            if(the_comparer(std::get<0>(retval->value), key))
            {
                return std::get<1>(retval->value);
            }
        }
        throw std::out_of_range("key doesn't exist in linked_map<KeyType, ValueType>::at(const KeyType&) const");
    }
    size_t erase(const key_type &key)
    {
        create_buckets();
        size_t current_hash = (size_t)the_hasher(key) % bucket_count_;
        Node **pnode = &buckets[current_hash];
        for(Node *node = *pnode; node != nullptr; pnode = &node->hash_next, node = *pnode)
        {
            if(the_comparer(std::get<0>(node->value), key))
            {
                *pnode = node->hash_next;
                if(node->list_prev != nullptr)
                    node->list_prev->list_next = node->list_next;
                else
                    list_head = node->list_next;
                if(node->list_next != nullptr)
                    node->list_next->list_prev = node->list_prev;
                else
                    list_tail = node->list_prev;
                delete node;
                node_count--;
                return 1;
            }
        }
        return 0;
    }
    iterator erase(const_iterator position)
    {
        Node *node = (Node *)position.pointer;
        if(node == nullptr)
            return end();
        iterator retval = iterator(node->list_next, this);
        if(buckets != nullptr)
        {
            size_t current_hash = node->cachedHash % bucket_count_;
            for(Node **pnode = &buckets[current_hash]; *pnode != nullptr; pnode = &(*pnode)->hash_next)
            {
                if(*pnode == node)
                {
                    *pnode = node->hash_next;
                    break;
                }
            }
        }
        if(node->list_prev != nullptr)
            node->list_prev->list_next = node->list_next;
        else
            list_head = node->list_next;
        if(node->list_next != nullptr)
            node->list_next->list_prev = node->list_prev;
        else
            list_tail = node->list_prev;
        delete node;
        node_count--;
        return retval;
    }
    size_t count(const key_type &key) const
    {
        return find(key) != end() ? 1 : 0;
    }
    void reserve(size_t n)
    {
        if(n <= node_count)
            return;
        rehash(n / 4);
    }
};

#endif // LINKED_MAP_H_INCLUDED
