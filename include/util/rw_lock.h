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
/* rw_lock struct is derived from boost thread shared_mutex
 *
 * (C) Copyright 2006-8 Anthony Williams
 * (C) Copyright 2012 Vicente J. Botet Escriba
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */
#ifndef RW_LOCK_H_INCLUDED
#define RW_LOCK_H_INCLUDED

#include <mutex>
#include <condition_variable>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
struct rw_lock final
{
private:
    std::mutex stateLock;
    std::condition_variable sharedCond;
    std::condition_variable exclusiveCond;
    std::condition_variable upgradeCond;
    std::size_t sharedCount = 0;
    bool exclusive = false;
    bool upgrade = false;
    bool exclusiveWaitingBlocked = false;
    void assertFree() const
    {
        assert(!exclusive);
        assert(!upgrade);
        assert(sharedCount == 0);
    }
    void assertLocked() const
    {
        assert(exclusive);
        assert(sharedCount == 0);
        assert(!upgrade);
    }
    void assertLockShared() const
    {
        assert(!exclusive);
        assert(sharedCount > 0);
    }
    void assertLockUpgraded() const
    {
        assert(!exclusive);
        assert(upgrade);
        assert(sharedCount > 0);
    }
    void assertLockNotUpgraded() const
    {
        assert(!upgrade);
    }
    bool canLock() const
    {
        return sharedCount == 0 && !exclusive;
    }
    void exclusiveBlocked(bool blocked)
    {
        exclusiveWaitingBlocked = blocked;
    }
    void lock()
    {
        exclusive = true;
    }
    void unlock()
    {
        exclusive = false;
        exclusiveWaitingBlocked = false;
    }
#error finish
public:
    void reader_lock()
    {
#error finish
    }
    bool reader_try_lock()
    {
#error finish
    }
    void reader_unlock()
    {
#error finish
    }
    void upgradable_reader_lock()
    {
#error finish
    }
    bool upgradable_reader_try_lock()
    {
#error finish
    }
    void upgradable_reader_unlock()
    {
#error finish
    }
    void upgradable_reader_upgrade()
    {
#error finish
    }
    void upgradable_reader_downgrade()
    {
#error finish
    }
    bool upgradable_reader_try_upgrade()
    {
#error finish
    }
    void writer_lock()
    {
#error finish
    }
    bool writer_try_lock()
    {
#error finish
    }
    void writer_unlock()
    {
#error finish
    }
    void writer_downgrade_to_upgradable()
    {
#error finish
    }
    void writer_downgrade_to_reader()
    {
#error finish
    }
    class reader_view final
    {
        friend struct rw_lock_implementation;
    private:
        rw_lock_implementation &theLock;
        reader_view(rw_lock_implementation &theLock)
            : theLock(theLock)
        {
        }
    public:
        void lock()
        {
            theLock.reader_lock();
        }
        bool try_lock()
        {
            return theLock.reader_try_lock();
        }
        void unlock()
        {
            theLock.reader_unlock();
        }
    };
    class writer_view final
    {
        friend struct rw_lock_implementation;
    private:
        rw_lock_implementation &theLock;
        writer_view(rw_lock_implementation &theLock)
            : theLock(theLock)
        {
        }
    public:
        void lock()
        {
            theLock.writer_lock();
        }
        bool try_lock()
        {
            return theLock.writer_try_lock();
        }
        void unlock()
        {
            theLock.writer_unlock();
        }
    };
    reader_view reader()
    {
        return reader_view(*this);
    }
    writer_view writer()
    {
        return writer_view(*this);
    }
};
typedef rw_lock rw_fast_lock;

template <typename T>
class reader_lock final
{
    T *theLock;
    bool locked;
public:
    constexpr reader_lock() noexcept
        : theLock(nullptr), locked(false)
    {
    }
    explicit reader_lock(T &lock)
        : theLock(&lock), locked(true)
    {
        lock.reader_lock();
    }
    reader_lock(T &lock, std::defer_lock_t) noexcept
        : theLock(&lock), locked(false)
    {
    }
    reader_lock(T &lock, std::try_to_lock_t)
        : theLock(&lock), locked(lock.reader_try_lock())
    {
    }
    reader_lock(T &lock, std::adopt_lock_t) noexcept
        : theLock(&lock), locked(true)
    {
    }
    ~reader_lock()
    {
        if(theLock && locked)
            theLock->reader_unlock();
    }
    reader_lock(reader_lock &&rt) noexcept
        : theLock(rt.theLock), locked(rt.locked)
    {
        rt.theLock = nullptr;
        rt.locked = false;
    }
    reader_lock &operator =(reader_lock &&rt)
    {
        if(theLock && locked)
            theLock->reader_unlock();
        theLock = rt.theLock;
        locked = rt.locked;
        rt.theLock = nullptr;
        rt.locked = false;
        return *this;
    }
    void lock()
    {
        assert(theLock && !locked);
        theLock->reader_lock();
        locked = true;
    }
    bool try_lock()
    {
        assert(theLock && !locked);
        locked = theLock->reader_try_lock();
        return locked;
    }
    void unlock()
    {
        assert(theLock && locked);
        theLock->reader_unlock();
        locked = false;
    }
    void swap(reader_lock<T> &rt) noexcept
    {
        using std::swap;
        swap(theLock, rt.theLock);
        swap(locked, rt.locked);
    }
    T *release() noexcept
    {
        T *retval = theLock;
        theLock = nullptr;
        locked = false;
    }
    T *mutex() const noexcept
    {
        return theLock;
    }
    bool owns_lock() const noexcept
    {
        return locked;
    }
    explicit operator bool() const noexcept
    {
        return locked;
    }
};

template <typename T>
class writer_lock final
{
    T *theLock;
    bool locked;
public:
    constexpr writer_lock() noexcept
        : theLock(nullptr), locked(false)
    {
    }
    explicit writer_lock(T &lock)
        : theLock(&lock), locked(true)
    {
        lock.writer_lock();
    }
    writer_lock(T &lock, std::defer_lock_t) noexcept
        : theLock(&lock), locked(false)
    {
    }
    writer_lock(T &lock, std::try_to_lock_t)
        : theLock(&lock), locked(lock.writer_try_lock())
    {
    }
    writer_lock(T &lock, std::adopt_lock_t) noexcept
        : theLock(&lock), locked(true)
    {
    }
    ~writer_lock()
    {
        if(theLock && locked)
            theLock->writer_unlock();
    }
    writer_lock(writer_lock &&rt) noexcept
        : theLock(rt.theLock), locked(rt.locked)
    {
        rt.theLock = nullptr;
        rt.locked = false;
    }
    writer_lock &operator =(writer_lock &&rt)
    {
        if(theLock && locked)
            theLock->writer_unlock();
        theLock = rt.theLock;
        locked = rt.locked;
        rt.theLock = nullptr;
        rt.locked = false;
        return *this;
    }
    void lock()
    {
        assert(theLock && !locked);
        theLock->writer_lock();
        locked = true;
    }
    bool try_lock()
    {
        assert(theLock && !locked);
        locked = theLock->writer_try_lock();
        return locked;
    }
    void unlock()
    {
        assert(theLock && locked);
        theLock->writer_unlock();
        locked = false;
    }
    void swap(writer_lock<T> &rt) noexcept
    {
        using std::swap;
        swap(theLock, rt.theLock);
        swap(locked, rt.locked);
    }
    T *release() noexcept
    {
        T *retval = theLock;
        theLock = nullptr;
        locked = false;
    }
    T *mutex() const noexcept
    {
        return theLock;
    }
    bool owns_lock() const noexcept
    {
        return locked;
    }
    explicit operator bool() const noexcept
    {
        return locked;
    }
};
}
}

namespace std
{
template <typename T>
void swap(programmerjake::voxels::reader_lock<T> &l, programmerjake::voxels::reader_lock<T> &r) noexcept
{
    l.swap(r);
}

template <typename T>
void swap(programmerjake::voxels::writer_lock<T> &l, programmerjake::voxels::writer_lock<T> &r) noexcept
{
    l.swap(r);
}
}

#endif // RW_LOCK_H_INCLUDED
