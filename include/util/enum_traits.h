#ifndef ENUM_TRAITS_H_INCLUDED
#define ENUM_TRAITS_H_INCLUDED

#include <cstddef>
#include <iterator>
#include <type_traits>

using namespace std;

template <typename T>
struct enum_traits;

template <typename T>
struct enum_iterator : public std::iterator<std::random_access_iterator_tag, T>
{
    T value;
    constexpr enum_iterator()
    {
    }
    constexpr enum_iterator(T value)
        : value(value)
    {
    }
    constexpr const T operator *() const
    {
        return value;
    }
    friend constexpr enum_iterator<T> operator +(ptrdiff_t a, T b)
    {
        return enum_iterator<T>((T)(a + (ptrdiff_t)b));
    }
    constexpr enum_iterator<T> operator +(ptrdiff_t b) const
    {
        return enum_iterator<T>((T)((ptrdiff_t)value + b));
    }
    constexpr enum_iterator<T> operator -(ptrdiff_t b) const
    {
        return enum_iterator<T>((T)((ptrdiff_t)value - b));
    }
    constexpr ptrdiff_t operator -(enum_iterator b) const
    {
        return (ptrdiff_t)value - (ptrdiff_t)b.value;
    }
    constexpr const T operator [](ptrdiff_t b) const
    {
        return (T)((ptrdiff_t)value + b);
    }
    const enum_iterator & operator +=(ptrdiff_t b)
    {
        value = (T)((ptrdiff_t)value + b);
        return *this;
    }
    const enum_iterator & operator -=(ptrdiff_t b)
    {
        value = (T)((ptrdiff_t)value - b);
        return *this;
    }
    const enum_iterator & operator ++()
    {
        value = (T)((ptrdiff_t)value + 1);
        return *this;
    }
    const enum_iterator & operator --()
    {
        value = (T)((ptrdiff_t)value - 1);
        return *this;
    }
    enum_iterator operator ++(int)
    {
        T retval = value;
        value = (T)((ptrdiff_t)value + 1);
        return enum_iterator(retval);
    }
    enum_iterator operator --(int)
    {
        T retval = value;
        value = (T)((ptrdiff_t)value - 1);
        return enum_iterator(retval);
    }
    constexpr bool operator ==(enum_iterator b) const
    {
        return value == b.value;
    }
    constexpr bool operator !=(enum_iterator b) const
    {
        return value != b.value;
    }
    constexpr bool operator >=(enum_iterator b) const
    {
        return value >= b.value;
    }
    constexpr bool operator <=(enum_iterator b) const
    {
        return value <= b.value;
    }
    constexpr bool operator >(enum_iterator b) const
    {
        return value > b.value;
    }
    constexpr bool operator <(enum_iterator b) const
    {
        return value < b.value;
    }
};

template <typename T, T minV, T maxV>
struct enum_traits_default
{
    typedef typename std::enable_if<std::is_enum<T>::value, T>::type type;
    typedef typename std::underlying_type<T>::type rwtype;
    static constexpr T minimum = minV;
    static constexpr T maximum = maxV;
    static constexpr enum_iterator<T> begin()
    {
        return enum_iterator<T>(minimum);
    }
    static constexpr enum_iterator<T> end()
    {
        return enum_iterator<T>(maximum) + 1;
    }
    static constexpr size_t size()
    {
        return end() - begin();
    }
};

template <typename T>
struct enum_traits : public enum_traits_default<typename std::enable_if<std::is_enum<T>::value, T>::type, T::enum_first, T::enum_last>
{
};

#define DEFINE_ENUM_LIMITS(first, last) \
enum_first = first, \
enum_last = last,

template <typename T, typename EnumT>
struct enum_array
{
    typedef T value_type;
    typedef EnumT index_type;
    typedef value_type * iterator;
    typedef const value_type * const_iterator;
    value_type elements[enum_traits<index_type>::size()];
    value_type & operator [](index_type index)
    {
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    const value_type & operator [](index_type index) const
    {
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    static constexpr size_t size()
    {
        return enum_traits<index_type>::size();
    }
    iterator begin()
    {
        return &elements[0];
    }
    iterator end()
    {
        return begin() + size();
    }
    const_iterator begin() const
    {
        return &elements[0];
    }
    const_iterator end() const
    {
        return begin() + size();
    }
    const_iterator cbegin() const
    {
        return &elements[0];
    }
    const_iterator cend() const
    {
        return begin() + size();
    }
};

#endif // ENUM_TRAITS_H_INCLUDED
