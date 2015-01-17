#ifndef ENUM_TRAITS_H_INCLUDED
#define ENUM_TRAITS_H_INCLUDED

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
template <typename T, typename = void>
struct enum_traits;

template <typename T>
struct enum_iterator : public std::iterator<std::random_access_iterator_tag, const T>
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
    friend constexpr enum_iterator<T> operator +(std::ptrdiff_t a, T b)
    {
        return enum_iterator<T>((T)(a + (std::ptrdiff_t)b));
    }
    constexpr enum_iterator<T> operator +(std::ptrdiff_t b) const
    {
        return enum_iterator<T>((T)((std::ptrdiff_t)value + b));
    }
    constexpr enum_iterator<T> operator -(std::ptrdiff_t b) const
    {
        return enum_iterator<T>((T)((std::ptrdiff_t)value - b));
    }
    constexpr std::ptrdiff_t operator -(enum_iterator b) const
    {
        return (std::ptrdiff_t)value - (std::ptrdiff_t)b.value;
    }
    constexpr const T operator [](std::ptrdiff_t b) const
    {
        return (T)((std::ptrdiff_t)value + b);
    }
    const enum_iterator & operator +=(std::ptrdiff_t b)
    {
        value = (T)((std::ptrdiff_t)value + b);
        return *this;
    }
    const enum_iterator & operator -=(std::ptrdiff_t b)
    {
        value = (T)((std::ptrdiff_t)value - b);
        return *this;
    }
    const enum_iterator & operator ++()
    {
        value = (T)((std::ptrdiff_t)value + 1);
        return *this;
    }
    const enum_iterator & operator --()
    {
        value = (T)((std::ptrdiff_t)value - 1);
        return *this;
    }
    enum_iterator operator ++(int)
    {
        T retval = value;
        value = (T)((std::ptrdiff_t)value + 1);
        return enum_iterator(retval);
    }
    enum_iterator operator --(int)
    {
        T retval = value;
        value = (T)((std::ptrdiff_t)value - 1);
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
    static constexpr std::size_t size()
    {
        return end() - begin();
    }
};

template <typename T>
struct enum_traits<T, typename std::enable_if<std::is_enum<T>::value>::type> : public enum_traits_default<T, T::enum_first, T::enum_last>
{
};

#define DEFINE_ENUM_LIMITS(first, last) \
enum_first = first, \
enum_last = last,

template <typename T, typename BT>
struct enum_struct
{
    typedef T type;
    typedef BT base_type;
    static_assert(std::is_integral<base_type>::value, "base type must be an integral type");
    base_type value;
    explicit constexpr enum_struct(base_type value)
        : value(value)
    {
    }
    enum_struct() = default;
    explicit constexpr operator base_type() const
    {
        return value;
    }
};

#define DEFINE_ENUM_STRUCT_LIMITS(first, last) \
static constexpr type enum_first() {return first();} \
static constexpr type enum_last() {return last();}

template <typename T>
struct enum_traits<T, typename std::enable_if<std::is_base_of<enum_struct<T, typename T::base_type>, T>::value>::type>
{
    typedef T type;
    typedef typename T::base_type rwtype;
    static constexpr T minimum = T::enum_first();
    static constexpr T maximum = T::enum_last();
    static constexpr enum_iterator<T> begin()
    {
        return enum_iterator<T>(minimum);
    }
    static constexpr enum_iterator<T> end()
    {
        return enum_iterator<T>(maximum) + 1;
    }
    static constexpr std::size_t size()
    {
        return end() - begin();
    }
};

template <typename T, typename EnumT>
struct enum_array
{
    typedef T value_type;
    typedef EnumT index_type;
    typedef value_type * iterator;
    typedef const value_type * const_iterator;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    value_type elements[enum_traits<index_type>::size()];
    value_type & operator [](index_type index)
    {
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    const value_type & operator [](index_type index) const
    {
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    value_type & at(index_type index)
    {
        assert(index >= enum_traits<index_type>::minimum && index <= enum_traits<index_type>::maximum);
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    const value_type & at(index_type index) const
    {
        assert(index >= enum_traits<index_type>::minimum && index <= enum_traits<index_type>::maximum);
        return elements[enum_iterator<index_type>(index) - enum_traits<index_type>::begin()];
    }
    static constexpr std::size_t size()
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
    const_iterator from_pointer(const_pointer v) const
    {
        return v;
    }
    iterator from_pointer(pointer v) const
    {
        return v;
    }
};
}
}

#endif // ENUM_TRAITS_H_INCLUDED
