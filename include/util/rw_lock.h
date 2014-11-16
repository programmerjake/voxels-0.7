#ifndef RW_LOCK_H_INCLUDED
#define RW_LOCK_H_INCLUDED

#include <atomic>
#include <mutex>
#include <condition_variable>
#include "util/cpu_relax.h"

template <bool isSpinLockV = false>
struct rw_lock;

template <>
struct rw_lock<false>
{
    static constexpr bool is_spinlock = false;
private:
    std::mutex stateLock;
    std::condition_variable stateCond;
    size_t writerWaitingCount = 0;
    size_t readerWaitingCount = 0;
    static constexpr size_t readerCountForWriterLocked = ~(size_t)0;
    size_t readerCount = 0;
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
    class reader_view final
    {
        friend class rw_lock;
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
    class writer_view final
    {
        friend class rw_lock;
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
    writer_view writer()
    {
        return writer_view(*this);
    }
};
#if 1
#warning finish
#else
#error fix change cmp-xchg first 2 args to correct order
template <>
struct [[deprecated("untested")]] rw_lock<true>
{
    static constexpr bool is_spinlock = true;
private:
    static constexpr size_t readerCountForWriterLocked = ~(size_t)0;
    std::atomic_size_t readerCount; // if a writer has this locked then readerCount == readerCountForWriterLocked
    std::atomic_size_t writerCount; // number of writers waiting + the writer that owns this lock
public:
    constexpr rw_lock()
        : readerCount(0), writerCount(0)
    {
    }
    void reader_lock()
    {
        size_t value = readerCount.load(std::memory_order_relaxed);
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
        size_t oldValue = value;
        value++;
        while(!readerCount.compare_exchange_weak(value, oldValue, std::memory_order_acquire, std::memory_order_relaxed))
        {
            cpu_relax();
            while(value == readerCountForWriterLocked)
            {
                cpu_relax();
                value = readerCount.load(std::memory_order_relaxed);
            }
            oldValue = value;
            value++;
        }
    }
    bool reader_try_lock()
    {
        size_t value = readerCount.load(std::memory_order_relaxed);
        if(value == readerCountForWriterLocked || writerCount.load(std::memory_order_relaxed) > 0)
        {
            return false;
        }
        size_t oldValue = value;
        value++;
        if(!readerCount.compare_exchange_strong(value, oldValue, std::memory_order_acquire, std::memory_order_relaxed))
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
        size_t value = readerCountForWriterLocked;
        while(!readerCount.compare_exchange_weak(value, 0, std::memory_order_acquire, std::memory_order_relaxed))
        {
            cpu_relax();
            value = readerCountForWriterLocked;
        }
    }
    bool writer_try_lock()
    {
        size_t value = readerCountForWriterLocked;
        if(!readerCount.compare_exchange_weak(value, 0, std::memory_order_acquire, std::memory_order_relaxed))
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
        friend class rw_lock;
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
    class writer_view final
    {
        friend class rw_lock;
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
    writer_view writer()
    {
        return writer_view(*this);
    }
};

typedef rw_lock<true> rw_spinlock;
#endif
typedef rw_lock<false> rw_blocking_lock;

#endif // RW_LOCK_H_INCLUDED
