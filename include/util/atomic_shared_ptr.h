/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef ATOMIC_SHARED_PTR_H_INCLUDED
#define ATOMIC_SHARED_PTR_H_INCLUDED

#include <atomic>
#include <memory>
#include <utility>

namespace programmerjake
{
namespace voxels
{
#ifndef DOXYGEN
template <typename T = void>
struct has_atomic_load_shared_ptr final
{
private:
    typedef char yes;
    struct no
    {
        char v[2];
    };
#ifndef __CDT_PARSER__
    template <std::shared_ptr<T>(*)(const std::shared_ptr<T> *) = &std::atomic_load>
    static yes fn(const T *);
#endif
    static no fn(...);

public:
    static constexpr bool value = sizeof(yes) == sizeof(fn(static_cast<const T *>(nullptr)));
};
#endif // DOXYGEN
/** @brief replacement for <code>std::atomic<std::shared_ptr<T>></code>
 */
#ifdef DOXYGEN
template <typename T>
class atomic_shared_ptr
{
public:
    constexpr atomic_shared_ptr()
    {
    }
    constexpr atomic_shared_ptr(std::shared_ptr<T> value) : value(std::move(value))
    {
    }
    const atomic_shared_ptr &operator=(const atomic_shared_ptr &) = delete;
    void operator=(std::shared_ptr<T> value);
    void store(std::shared_ptr<T> value);
    void store(std::shared_ptr<T> value, std::memory_order mo);
    bool is_lock_free() const;
    std::shared_ptr<T> load() const;
    std::shared_ptr<T> load(std::memory_order mo) const;
    operator std::shared_ptr<T>() const;
    std::shared_ptr<T> exchange(std::shared_ptr<T> desired);
    std::shared_ptr<T> exchange(std::shared_ptr<T> desired, std::memory_order mo);
    bool compare_exchange_weak(std::shared_ptr<T> &expected,
                               std::shared_ptr<T> desired,
                               std::memory_order success,
                               std::memory_order failure);
    bool compare_exchange_weak(std::shared_ptr<T> &expected,
                               std::shared_ptr<T> desired,
                               std::memory_order mo = std::memory_order::memory_order_seq_cst);
    bool compare_exchange_strong(std::shared_ptr<T> &expected,
                                 std::shared_ptr<T> desired,
                                 std::memory_order success,
                                 std::memory_order failure);
    bool compare_exchange_strong(std::shared_ptr<T> &expected,
                                 std::shared_ptr<T> desired,
                                 std::memory_order mo = std::memory_order::memory_order_seq_cst);
};
#else // DOXYGEN
template <typename T, bool = has_atomic_load_shared_ptr<>::value>
class atomic_shared_ptr;

template <typename T>
class atomic_shared_ptr<T, true> final
{
    std::shared_ptr<T> value;

public:
    atomic_shared_ptr() = default;
    constexpr atomic_shared_ptr(std::shared_ptr<T> value) : value(std::move(value))
    {
    }
    atomic_shared_ptr(const atomic_shared_ptr &) = delete;
    const atomic_shared_ptr &operator=(const atomic_shared_ptr &) = delete;
    atomic_shared_ptr &operator=(std::shared_ptr<T> value)
    {
        store(std::move(value));
        return *this;
    }
    void store(std::shared_ptr<T> value)
    {
        std::atomic_store(&this->value, std::move(value));
    }
    void store(std::shared_ptr<T> value, std::memory_order mo)
    {
        std::atomic_store_explicit(&this->value, std::move(value), mo);
    }
    bool is_lock_free() const
    {
        return std::atomic_is_lock_free(&value);
    }
    std::shared_ptr<T> load() const
    {
        return std::atomic_load(&value);
    }
    std::shared_ptr<T> load(std::memory_order mo) const
    {
        return std::atomic_load_explicit(&value, mo);
    }
    operator std::shared_ptr<T>() const
    {
        return load();
    }
    std::shared_ptr<T> exchange(std::shared_ptr<T> desired)
    {
        return std::atomic_exchange(&value, std::move(desired));
    }
    std::shared_ptr<T> exchange(std::shared_ptr<T> desired, std::memory_order mo)
    {
        return std::atomic_exchange_explicit(&value, std::move(desired), mo);
    }
    bool compare_exchange_weak(std::shared_ptr<T> &expected,
                               std::shared_ptr<T> desired,
                               std::memory_order success,
                               std::memory_order failure)
    {
        return std::atomic_compare_exchange_weak_explicit(
            &value, &expected, std::move(desired), success, failure);
    }
    bool compare_exchange_weak(std::shared_ptr<T> &expected,
                               std::shared_ptr<T> desired,
                               std::memory_order mo = std::memory_order::memory_order_seq_cst)
    {
        std::memory_order success = mo, failure = mo;
        if(mo == std::memory_order::memory_order_acq_rel)
            failure = std::memory_order::memory_order_acquire;
        else if(mo == std::memory_order::memory_order_release)
            failure = std::memory_order::memory_order_relaxed;
        return std::atomic_compare_exchange_weak_explicit(
            &value, &expected, std::move(desired), success, failure);
    }
    bool compare_exchange_strong(std::shared_ptr<T> &expected,
                                 std::shared_ptr<T> desired,
                                 std::memory_order success,
                                 std::memory_order failure)
    {
        return std::atomic_compare_exchange_strong_explicit(
            &value, &expected, std::move(desired), success, failure);
    }
    bool compare_exchange_strong(std::shared_ptr<T> &expected,
                                 std::shared_ptr<T> desired,
                                 std::memory_order mo = std::memory_order::memory_order_seq_cst)
    {
        std::memory_order success = mo, failure = mo;
        if(mo == std::memory_order::memory_order_acq_rel)
            failure = std::memory_order::memory_order_acquire;
        else if(mo == std::memory_order::memory_order_release)
            failure = std::memory_order::memory_order_relaxed;
        return std::atomic_compare_exchange_strong_explicit(
            &value, &expected, std::move(desired), success, failure);
    }
};

class atomic_shared_ptr_mutex_holder final
{
private:
    void *theLock;

public:
    atomic_shared_ptr_mutex_holder(void *theAtomicSharedPtr) noexcept;
    void lock();
    bool try_lock();
    void unlock();
};

template <typename T>
class atomic_shared_ptr<T, false> final
{
    std::shared_ptr<T> value;

public:
    atomic_shared_ptr() = default;
    constexpr atomic_shared_ptr(std::shared_ptr<T> value) : value(std::move(value))
    {
    }
    atomic_shared_ptr(const atomic_shared_ptr &) = delete;
    const atomic_shared_ptr &operator=(const atomic_shared_ptr &) = delete;
    atomic_shared_ptr &operator=(std::shared_ptr<T> value)
    {
        store(std::move(value));
        return *this;
    }
    void store(std::shared_ptr<T> value)
    {
        atomic_shared_ptr_mutex_holder mutex_holder(static_cast<void *>(this));
        mutex_holder.lock();
        this->value = std::move(value);
        mutex_holder.unlock();
    }
    void store(std::shared_ptr<T> value, std::memory_order)
    {
        store(std::move(value));
    }
    bool is_lock_free() const
    {
        return false;
    }
    std::shared_ptr<T> load() const
    {
        atomic_shared_ptr_mutex_holder mutex_holder(
            const_cast<void *>(reinterpret_cast<const void *>(this)));
        mutex_holder.lock();
        std::shared_ptr<T> retval = value;
        mutex_holder.unlock();
        return std::move(retval);
    }
    std::shared_ptr<T> load(std::memory_order) const
    {
        return load();
    }
    operator std::shared_ptr<T>() const
    {
        return load();
    }
    std::shared_ptr<T> exchange(std::shared_ptr<T> desired)
    {
        atomic_shared_ptr_mutex_holder mutex_holder(reinterpret_cast<void *>(this));
        mutex_holder.lock();
        std::shared_ptr<T> retval = value;
        value = std::move(desired);
        mutex_holder.unlock();
        return std::move(retval);
    }
    std::shared_ptr<T> exchange(std::shared_ptr<T> desired, std::memory_order)
    {
        return exchange(std::move(desired));
    }

private:
    static bool owner_equal(const std::shared_ptr<T> &a, const std::shared_ptr<T> &b)
    {
        return !a.owner_before(b) && !b.owner_before(a);
    }
    static bool total_equal(const std::shared_ptr<T> &a, const std::shared_ptr<T> &b)
    {
        return a == b && owner_equal(a, b);
    }

public:
    bool compare_exchange_weak(std::shared_ptr<T> &expected,
                               std::shared_ptr<T> desired,
                               std::memory_order,
                               std::memory_order)
    {
        return compare_exchange_weak(expected, std::move(desired));
    }
    bool compare_exchange_weak(std::shared_ptr<T> &expected,
                               std::shared_ptr<T> desired,
                               std::memory_order = std::memory_order::memory_order_seq_cst)
    {
        atomic_shared_ptr_mutex_holder mutex_holder(static_cast<void *>(this));
        if(!mutex_holder.try_lock())
            return false;
        bool retval = total_equal(value, expected);
        if(retval)
        {
            value = std::move(desired);
        }
        else
        {
            expected = value;
        }
        mutex_holder.unlock();
        return retval;
    }
    bool compare_exchange_strong(std::shared_ptr<T> &expected,
                                 std::shared_ptr<T> desired,
                                 std::memory_order success,
                                 std::memory_order failure)
    {
        return compare_exchange_strong(expected, std::move(desired));
    }
    bool compare_exchange_strong(std::shared_ptr<T> &expected,
                                 std::shared_ptr<T> desired,
                                 std::memory_order = std::memory_order::memory_order_seq_cst)
    {
        atomic_shared_ptr_mutex_holder mutex_holder(static_cast<void *>(this));
        mutex_holder.lock();
        bool retval = total_equal(value, expected);
        if(retval)
        {
            value = std::move(desired);
        }
        else
        {
            expected = value;
        }
        mutex_holder.unlock();
        return retval;
    }
};
#endif // DOXYGEN
}
}

#endif // ATOMIC_SHARED_PTR_H_INCLUDED
