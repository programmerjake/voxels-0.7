#ifndef CIRCULAR_DEQUE_H_INCLUDED
#define CIRCULAR_DEQUE_H_INCLUDED

#include <iterator>

template <typename T, size_t arraySize>
class circularDeque final
{
public:
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;
    static constexpr size_type capacity()
    {
        return arraySize - 1;
    }
private:
    size_type frontIndex, backIndex;
    value_type array[arraySize];
public:
    friend class iterator;
    class iterator final : public std::iterator<std::random_access_iterator_tag, value_type>
    {
        friend class circularDeque;
        friend class const_iterator;
    private:
        circularDeque *container;
        size_t index;
        iterator(circularDeque *container, size_t index)
            : container(container), index(index)
        {
        }
    public:
        iterator()
            : container(nullptr)
        {
        }
        iterator &operator +=(difference_type n)
        {
            if(-n > (difference_type)index)
            {
                n = n % arraySize + arraySize;
            }

            index += n;
            index %= arraySize;

            if(index < 0)
            {
                index += arraySize;
            }

            return *this;
        }
        iterator &operator -=(difference_type n)
        {
            return *this += -n;
        }
        friend iterator operator +(difference_type n, iterator i)
        {
            return i += n;
        }
        friend iterator operator +(iterator i, difference_type n)
        {
            return i += n;
        }
        friend iterator operator -(iterator i, difference_type n)
        {
            return i -= n;
        }
        difference_type operator -(const iterator &r) const
        {
            assert(container == r.container && container != nullptr);
            difference_type loc = index + arraySize - container->frontIndex;

            if(loc >= arraySize)
            {
                loc -= arraySize;
            }

            if(loc >= arraySize)
            {
                loc -= arraySize;
            }

            difference_type rloc = r.index + arraySize - container->frontIndex;

            if(rloc >= arraySize)
            {
                rloc -= arraySize;
            }

            if(rloc >= arraySize)
            {
                rloc -= arraySize;
            }

            return loc - rloc;
        }
        T &operator [](difference_type n) const
        {
            return *(*this + n);
        }
        T &operator *() const
        {
            return container->array[index];
        }
        T *operator ->() const
        {
            return container->array + index;
        }
        const iterator &operator --()
        {
            if(index == 0)
            {
                index = arraySize - 1;
            }
            else
            {
                index--;
            }

            return *this;
        }
        iterator operator --(int)
        {
            iterator retval = *this;

            if(index == 0)
            {
                index = arraySize - 1;
            }
            else
            {
                index--;
            }

            return retval;
        }
        const iterator &operator ++()
        {
            if(index >= arraySize - 1)
            {
                index = 0;
            }
            else
            {
                index++;
            }

            return *this;
        }
        iterator operator ++(int)
        {
            iterator retval = *this;

            if(index >= arraySize - 1)
            {
                index = 0;
            }
            else
            {
                index++;
            }

            return retval;
        }
        friend bool operator ==(const iterator &l, const iterator &r)
        {
            return l.index == r.index;
        }
        friend bool operator !=(const iterator &l, const iterator &r)
        {
            return l.index != r.index;
        }
        friend bool operator >(const iterator &l, const iterator &r)
        {
            return (l - r) > 0;
        }
        friend bool operator >=(const iterator &l, const iterator &r)
        {
            return (l - r) >= 0;
        }
        friend bool operator <(const iterator &l, const iterator &r)
        {
            return (l - r) < 0;
        }
        friend bool operator <=(const iterator &l, const iterator &r)
        {
            return (l - r) <= 0;
        }
    };

    friend class const_iterator;
    class const_iterator final : public std::iterator<std::random_access_iterator_tag, const value_type>
    {
        friend class circularDeque;
    private:
        const circularDeque *container;
        size_t index;
        const_iterator(const circularDeque *container, size_t index)
            : container(container), index(index)
        {
        }
    public:
        const_iterator()
            : container(nullptr)
        {
        }
        const_iterator(const iterator &v)
            : container(v.container), index(v.index)
        {
        }
        const_iterator &operator +=(difference_type n)
        {
            if(-n > (difference_type)index)
            {
                n = n % arraySize + arraySize;
            }

            index += n;
            index %= arraySize;

            if(index < 0)
            {
                index += arraySize;
            }

            return *this;
        }
        const_iterator &operator -=(difference_type n)
        {
            return *this += -n;
        }
        friend const_iterator operator +(difference_type n, const_iterator i)
        {
            return i += n;
        }
        friend const_iterator operator +(const_iterator i, difference_type n)
        {
            return i += n;
        }
        friend const_iterator operator -(const_iterator i, difference_type n)
        {
            return i -= n;
        }
        difference_type operator -(const const_iterator &r) const
        {
            assert(container == r.container && container != nullptr);
            difference_type loc = index + arraySize - container->frontIndex;

            if((size_t)loc >= arraySize)
            {
                loc -= arraySize;
            }

            if((size_t)loc >= arraySize)
            {
                loc -= arraySize;
            }

            difference_type rloc = r.index + arraySize - container->frontIndex;

            if((size_t)rloc >= arraySize)
            {
                rloc -= arraySize;
            }

            if((size_t)rloc >= arraySize)
            {
                rloc -= arraySize;
            }

            return loc - rloc;
        }
        const T &operator [](difference_type n) const
        {
            return *(*this + n);
        }
        const T &operator *() const
        {
            return container->array[index];
        }
        const T *operator ->() const
        {
            return container->array + index;
        }
        const const_iterator &operator --()
        {
            if(index == 0)
            {
                index = arraySize - 1;
            }
            else
            {
                index--;
            }

            return *this;
        }
        const_iterator operator --(int)
        {
            const_iterator retval = *this;

            if(index == 0)
            {
                index = arraySize - 1;
            }
            else
            {
                index--;
            }

            return retval;
        }
        const const_iterator &operator ++()
        {
            if(index >= arraySize - 1)
            {
                index = 0;
            }
            else
            {
                index++;
            }

            return *this;
        }
        const_iterator operator ++(int)
        {
            const_iterator retval = *this;

            if(index >= arraySize - 1)
            {
                index = 0;
            }
            else
            {
                index++;
            }

            return retval;
        }
        friend bool operator ==(const const_iterator &l, const const_iterator &r)
        {
            return l.index == r.index;
        }
        friend bool operator !=(const const_iterator &l, const const_iterator &r)
        {
            return l.index != r.index;
        }
        friend bool operator >(const const_iterator &l, const const_iterator &r)
        {
            return (l - r) > 0;
        }
        friend bool operator >=(const const_iterator &l, const const_iterator &r)
        {
            return (l - r) >= 0;
        }
        friend bool operator <(const const_iterator &l, const const_iterator &r)
        {
            return (l - r) < 0;
        }
        friend bool operator <=(const const_iterator &l, const const_iterator &r)
        {
            return (l - r) <= 0;
        }
    };

    typedef std::reverse_iterator<iterator> reverse_iterator;

    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    circularDeque()
        : frontIndex(0), backIndex(0)
    {
    }

    iterator begin()
    {
        return iterator(this, frontIndex);
    }

    const_iterator begin() const
    {
        return const_iterator(this, frontIndex);
    }

    const_iterator cbegin() const
    {
        return const_iterator(this, frontIndex);
    }

    iterator end()
    {
        return iterator(this, backIndex);
    }

    const_iterator end() const
    {
        return const_iterator(this, backIndex);
    }

    const_iterator cend() const
    {
        return const_iterator(this, backIndex);
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(cend());
    }

    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(cbegin());
    }

    T &front()
    {
        return *begin();
    }

    const T &front() const
    {
        return *begin();
    }

    T &back()
    {
        return end()[-1];
    }

    const T &back() const
    {
        return end()[-1];
    }

    size_type size() const
    {
        return cend() - cbegin();
    }

    T &at(size_type pos)
    {
        if(pos >= size())
        {
            throw out_of_range("position out of range in circularDeque::at");
        }

        return begin()[pos];
    }

    const T &at(size_type pos) const
    {
        if(pos >= size())
        {
            throw out_of_range("position out of range in circularDeque::at");
        }

        return cbegin()[pos];
    }

    T &operator [](size_type pos)
    {
        return begin()[pos];
    }

    const T &operator [](size_type pos) const
    {
        return cbegin()[pos];
    }

    bool empty() const
    {
        return frontIndex == backIndex;
    }

    void clear()
    {
        frontIndex = backIndex = 0;
    }

    void push_front(const T &v)
    {
        if(frontIndex-- == 0)
        {
            frontIndex = arraySize - 1;
        }

        array[frontIndex] = v;
    }

    void push_front(T  &&v)
    {
        if(frontIndex-- == 0)
        {
            frontIndex = arraySize - 1;
        }

        array[frontIndex] = std::move(v);
    }

    void push_back(const T &v)
    {
        array[backIndex] = v;

        if(++backIndex >= arraySize)
        {
            backIndex = 0;
        }
    }

    void push_back(T  &&v)
    {
        array[backIndex] = std::move(v);

        if(++backIndex >= arraySize)
        {
            backIndex = 0;
        }
    }

    void pop_front()
    {
        array[frontIndex] = T();

        if(++frontIndex >= arraySize)
        {
            frontIndex = 0;
        }
    }

    void pop_back()
    {
        if(backIndex-- == 0)
        {
            backIndex = arraySize - 1;
        }

        array[backIndex] = T();
    }

    void swap(circularDeque &other)
    {
        circularDeque<T, arraySize> temp = std::move(*this);
        *this = std::move(other);
        other = std::move(temp);
    }
};

#endif // CIRCULAR_DEQUE_H_INCLUDED
