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
    rw_lock(const rw_lock &) = delete;
    rw_lock &operator =(const rw_lock &) = delete;
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
    bool canLockShared() const
    {
        return !exclusive && !exclusiveWaitingBlocked;
    }
    bool moreShared() const
    {
        return sharedCount > 0;
    }
    std::size_t getSharedCount() const
    {
        return sharedCount;
    }
    std::size_t lockShared()
    {
        return ++sharedCount;
    }
    void unlockShared()
    {
        sharedCount--;
    }
    bool unlockSharedDowngrades()
    {
        if(upgrade)
        {
            upgrade = false;
            exclusive = true;
            return true;
        }
        exclusiveWaitingBlocked = false;
        return false;
    }
    void lockUpgrade()
    {
        sharedCount++;
        upgrade = true;
    }
    bool canLockUpgrade() const
    {
        return !exclusive && !exclusiveWaitingBlocked && !upgrade;
    }
    void unlockUpgrade()
    {
        upgrade = false;
        sharedCount--;
    }
    void releaseWaiters()
    {
        exclusiveCond.notify_one();
        sharedCond.notify_all();
    }
public:
    void reader_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        while(!canLockShared())
            sharedCond.wait(lockIt);
        lockShared();
    }
    bool reader_try_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        if(!canLockShared())
            return false;
        lockShared();
        return true;
    }
    void reader_unlock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assertLockShared();
        unlockShared();
        if(!moreShared())
        {
            if(upgrade)
            {
                upgrade = false;
                exclusive = false;
                lockIt.unlock();
                upgradeCond.notify_one();
            }
            else
            {
                exclusiveWaitingBlocked = false;
                lockIt.unlock();
            }
            releaseWaiters();
        }
    }
    void upgradable_reader_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        while(exclusive || exclusiveWaitingBlocked || upgrade)
        {
            sharedCond.wait(lockIt);
        }
        lockShared();
        upgrade = true;
    }
    bool upgradable_reader_try_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        if(exclusive || exclusiveWaitingBlocked || upgrade)
            return false;
        lockShared();
        upgrade = true;
        assertLockUpgraded();
        return true;
    }
    void upgradable_reader_unlock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        unlockUpgrade();
        if(!moreShared())
        {
            exclusiveWaitingBlocked = false;
            releaseWaiters();
        }
        else
        {
            sharedCond.notify_all();
        }
    }
    void upgradable_reader_upgrade()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assertLockUpgraded();
        unlockShared();
        while(moreShared())
            upgradeCond.wait(lockIt);
        upgrade = false;
        exclusive = false;
        assertLocked();
    }
    void upgradable_reader_downgrade()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assertLockUpgraded();
        upgrade = false;
        exclusiveWaitingBlocked = false;
        releaseWaiters();
    }
    bool upgradable_reader_try_upgrade()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assertLockUpgraded();
        if(!exclusive && !exclusiveWaitingBlocked && upgrade && sharedCount == 1)
        {
            sharedCount = 0;
            exclusive = true;
            upgrade = false;
            assertLocked();
            return true;
        }
        return false;
    }
    void writer_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        while(sharedCount > 0 || exclusive)
        {
            exclusiveWaitingBlocked = true;
            exclusiveCond.wait(lockIt);
        }
        exclusive = true;
    }
    bool writer_try_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        if(sharedCount > 0 || exclusive)
            return false;
        exclusive = true;
        return true;
    }
    void writer_unlock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assertLocked();
        exclusive = false;
        exclusiveWaitingBlocked = false;
        assertFree();
        releaseWaiters();
    }
    void writer_downgrade_to_upgradable()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assertLocked();
        exclusive = false;
        upgrade = true;
        lockShared();
        exclusiveWaitingBlocked = false;
        assertLockUpgraded();
        releaseWaiters();
    }
    void writer_downgrade_to_reader()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assertLocked();
        exclusive = false;
        lockShared();
        exclusiveWaitingBlocked = false;
        releaseWaiters();
    }
    class reader_view final
    {
        friend struct rw_lock;
    private:
        rw_lock &theLock;
        reader_view(rw_lock &theLock)
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
    class upgradable_reader_view final
    {
        friend struct rw_lock;
    private:
        rw_lock &theLock;
        upgradable_reader_view(rw_lock &theLock)
            : theLock(theLock)
        {
        }
    public:
        void lock()
        {
            theLock.upgradable_reader_lock();
        }
        bool try_lock()
        {
            return theLock.upgradable_reader_try_lock();
        }
        void unlock()
        {
            theLock.upgradable_reader_unlock();
        }
    };
    class writer_view final
    {
        friend struct rw_lock;
    private:
        rw_lock &theLock;
        writer_view(rw_lock &theLock)
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
    upgradable_reader_view upgradable_reader()
    {
        return upgradable_reader_view(*this);
    }
    writer_view writer()
    {
        return writer_view(*this);
    }
};

template <typename T>
class writer_lock;

template <typename T>
class upgradable_reader_lock;

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
    explicit reader_lock(upgradable_reader_lock<T> &&rt)
        : theLock(nullptr), locked(rt.owns_lock())
    {
        theLock(rt.release());
        if(locked)
            theLock->upgradable_reader_downgrade();
    }
    explicit reader_lock(writer_lock<T> &&rt)
        : theLock(nullptr), locked(rt.owns_lock())
    {
        theLock(rt.release());
        if(locked)
            theLock->writer_downgrade_to_reader();
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
        return retval;
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
class upgradable_reader_lock final
{
    T *theLock;
    bool locked;
public:
    constexpr upgradable_reader_lock() noexcept
        : theLock(nullptr), locked(false)
    {
    }
    explicit upgradable_reader_lock(T &lock)
        : theLock(&lock), locked(true)
    {
        lock.upgradable_reader_lock();
    }
    explicit upgradable_reader_lock(writer_lock<T> &&rt)
        : theLock(nullptr), locked(rt.owns_lock())
    {
        theLock(rt.release());
        if(locked)
            theLock->writer_downgrade_to_upgradable();
    }
    upgradable_reader_lock(T &lock, std::defer_lock_t) noexcept
        : theLock(&lock), locked(false)
    {
    }
    upgradable_reader_lock(T &lock, std::try_to_lock_t)
        : theLock(&lock), locked(lock.reader_try_lock())
    {
    }
    upgradable_reader_lock(T &lock, std::adopt_lock_t) noexcept
        : theLock(&lock), locked(true)
    {
    }
    ~upgradable_reader_lock()
    {
        if(theLock && locked)
            theLock->upgradable_reader_unlock();
    }
    upgradable_reader_lock(upgradable_reader_lock &&rt) noexcept
        : theLock(rt.theLock), locked(rt.locked)
    {
        rt.theLock = nullptr;
        rt.locked = false;
    }
    upgradable_reader_lock &operator =(upgradable_reader_lock &&rt)
    {
        if(theLock && locked)
            theLock->upgradable_reader_unlock();
        theLock = rt.theLock;
        locked = rt.locked;
        rt.theLock = nullptr;
        rt.locked = false;
        return *this;
    }
    void lock()
    {
        assert(theLock && !locked);
        theLock->upgradable_reader_lock();
        locked = true;
    }
    bool try_lock()
    {
        assert(theLock && !locked);
        locked = theLock->upgradable_reader_try_lock();
        return locked;
    }
    writer_lock<T> upgrade()
    {
        if(!theLock)
            return writer_lock<T>();
        if(!locked)
            return writer_lock<T>(*release(), std::defer_lock);
        theLock->upgradable_reader_upgrade();
        return writer_lock<T>(*release(), std::adopt_lock);
    }
    writer_lock<T> try_upgrade()
    {
        if(!theLock)
            return writer_lock<T>();
        if(!locked)
            return writer_lock<T>(*release(), std::defer_lock);
        if(!theLock->upgradable_reader_try_upgrade())
            return writer_lock<T>();
        return writer_lock<T>(*release(), std::adopt_lock);
    }
    void unlock()
    {
        assert(theLock && locked);
        theLock->upgradable_reader_unlock();
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
        return retval;
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
        return retval;
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
void swap(programmerjake::voxels::upgradable_reader_lock<T> &l, programmerjake::voxels::upgradable_reader_lock<T> &r) noexcept
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
