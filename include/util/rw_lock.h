/*
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
#ifndef RW_LOCK_H_INCLUDED
#define RW_LOCK_H_INCLUDED

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <utility>
#include "util/cpu_relax.h"
#if __cplusplus > 201103L // c++14 or later
#include <shared_mutex>
#endif

namespace programmerjake
{
namespace voxels
{
template <bool isSpinLockV = false>
struct rw_lock_implementation;

template <>
struct rw_lock_implementation<false>
{
    static constexpr bool is_spinlock = false;
private:
#if __cplusplus > 201103L // c++14 or later
    std::shared_timed_mutex lock;
public:
    void reader_lock()
    {
        lock.lock_shared();
    }
    bool reader_try_lock()
    {
        return lock.try_lock_shared();
    }
    void reader_unlock()
    {
        return lock.unlock_shared();
    }
    void writer_lock()
    {
        return lock.lock();
    }
    bool writer_try_lock()
    {
        return lock.try_lock();
    }
    void writer_unlock()
    {
        return lock.unlock();
    }
#else
    std::mutex stateLock;
    std::condition_variable stateCond;
    std::size_t writerWaitingCount = 0;
    std::size_t readerWaitingCount = 0;
    static constexpr std::size_t readerCountForWriterLocked = ~(std::size_t)0;
    std::size_t readerCount = 0;
public:
    void reader_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        if(writerWaitingCount > 0 || readerCount == readerCountForWriterLocked)
        {
            readerWaitingCount++;
            while(writerWaitingCount > 0 || readerCount == readerCountForWriterLocked)
            {
                stateCond.wait(lockIt);
            }
            readerWaitingCount--;
        }
        readerCount++;
    }
    bool reader_try_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock, std::try_to_lock);
        if(!lockIt.owns_lock())
            return false;
        if(writerWaitingCount > 0 || readerCount == readerCountForWriterLocked)
        {
            return false;
        }
        readerCount++;
        return true;
    }
    void reader_unlock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assert(readerCount != readerCountForWriterLocked && readerCount > 0);
        readerCount--;
        if(readerCount == 0 && writerWaitingCount > 0)
            stateCond.notify_all();
    }
    void writer_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        if(readerCount != 0)
        {
            writerWaitingCount++;
            while(readerCount != 0)
            {
                stateCond.wait(lockIt);
            }
            writerWaitingCount--;
        }
        readerCount = readerCountForWriterLocked;
    }
    bool writer_try_lock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock, std::try_to_lock);
        if(!lockIt.owns_lock())
            return false;
        if(readerCount != 0)
            return false;
        readerCount = readerCountForWriterLocked;
        return true;
    }
    void writer_unlock()
    {
        std::unique_lock<std::mutex> lockIt(stateLock);
        assert(readerCount == readerCountForWriterLocked);
        readerCount = 0;
        if(readerWaitingCount > 0 || writerWaitingCount > 0)
            stateCond.notify_all();
    }
#endif
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

template <>
struct [[deprecated("untested")]] rw_lock_implementation<true>
{
    static constexpr bool is_spinlock = true;
private:
    static constexpr std::size_t readerCountForWriterLocked = ~(std::size_t)0;
    std::atomic_size_t readerCount; // if a writer has this locked then readerCount == readerCountForWriterLocked
    std::atomic_size_t writerCount; // number of writers waiting + the writer that owns this lock
public:
    constexpr rw_lock_implementation()
        : readerCount(0), writerCount(0)
    {
    }
    void reader_lock()
    {
        std::size_t value = readerCount.load(std::memory_order_relaxed);
        while(value == readerCountForWriterLocked || writerCount.load(std::memory_order_relaxed) > 0)
        {
            cpu_relax();
            value = readerCount.load(std::memory_order_relaxed);
        }
        while(value == readerCountForWriterLocked)
        {
            cpu_relax();
            value = readerCount.load(std::memory_order_relaxed);
        }
        while(!readerCount.compare_exchange_weak(value, value + 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            cpu_relax();
            while(value == readerCountForWriterLocked)
            {
                cpu_relax();
                value = readerCount.load(std::memory_order_relaxed);
            }
        }
    }
    bool reader_try_lock()
    {
        std::size_t value = readerCount.load(std::memory_order_relaxed);
        if(value == readerCountForWriterLocked || writerCount.load(std::memory_order_relaxed) > 0)
        {
            return false;
        }
        if(!readerCount.compare_exchange_strong(value, value + 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            return false;
        }
        return true;
    }
    void reader_unlock()
    {
        readerCount.fetch_sub(1, std::memory_order_release);
    }
    void writer_lock()
    {
        writerCount.fetch_add(1, std::memory_order_relaxed);
        std::size_t value = 0;
        while(!readerCount.compare_exchange_weak(value, readerCountForWriterLocked, std::memory_order_acquire, std::memory_order_relaxed))
        {
            cpu_relax();
            value = 0;
        }
    }
    bool writer_try_lock()
    {
        std::size_t value = 0;
        if(!readerCount.compare_exchange_weak(value, readerCountForWriterLocked, std::memory_order_acquire, std::memory_order_relaxed))
        {
            return false;
        }
        writerCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    void writer_unlock()
    {
        writerCount.fetch_sub(1, std::memory_order_relaxed);
        readerCount.store(0, std::memory_order_release);
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

typedef rw_lock_implementation<true> rw_spinlock [[deprecated("untested")]];
typedef rw_lock_implementation<false> rw_lock;
#if 1 // change when spinlock is checked
typedef rw_lock rw_fast_lock;
#else
typedef rw_spinlock rw_fast_lock;
#endif

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
        std::swap(theLock, rt.theLock);
        std::swap(locked, rt.locked);
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
        std::swap(theLock, rt.theLock);
        std::swap(locked, rt.locked);
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
