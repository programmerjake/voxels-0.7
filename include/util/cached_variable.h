#ifndef CACHED_VARIABLE_H_INCLUDED
#define CACHED_VARIABLE_H_INCLUDED

#include <atomic>
#include <cstdint>

template <typename T>
class CachedVariable
{
    std::atomic_uint_fast8_t viewIndex;
    T view1, view2;
public:
    CachedVariable()
        : viewIndex(0), view1(), view2()
    {
    }
    CachedVariable(const T &v)
        : viewIndex(0), view1(v), view2()
    {
    }
    CachedVariable(T &&v)
        : viewIndex(0), view1(std::move(v)), view2()
    {
    }
    CachedVariable(const CachedVariable &rt)
        : CachedVariable(rt.read())
    {
    }
    const T &read() const
    {
        return viewIndex ? view2 : view1;
    }
    T &writeRef()
    {
        return viewIndex ? view1 : view2;
    }
    void finishWrite()
    {
        viewIndex ^= 1;
    }
    void write(const T &value)
    {
        writeRef() = value;
        finishWrite();
    }
    operator const T &() const
    {
        return read();
    }
    const T &operator =(const T &value)
    {
        writeRef() = value;
        finishWrite();
        return read();
    }
    const T &operator =(const CachedVariable &rt)
    {
        writeRef() = rt.read();
        finishWrite();
        return read();
    }
};

#endif // CACHED_VARIABLE_H_INCLUDED
