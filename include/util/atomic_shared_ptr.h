#ifndef ATOMIC_SHARED_PTR_H_INCLUDED
#define ATOMIC_SHARED_PTR_H_INCLUDED

#include <atomic>
#include <memory>
#include <utility>

namespace programmerjake
{
namespace voxels
{
/** @brief replacement for <code>std::atomic<std::shared_ptr<T>></code>
 */
template <typename T>
class atomic_shared_ptr final
{
    std::shared_ptr<T> value;
public:
    atomic_shared_ptr() = default;
    constexpr atomic_shared_ptr(std::shared_ptr<T> value)
        : value(std::move(value))
    {
    }
    atomic_shared_ptr(const atomic_shared_ptr &) = delete;
    const atomic_shared_ptr &operator =(const atomic_shared_ptr &) = delete;
    void operator =(std::shared_ptr<T> value)
    {
        store(std::move(value));
    }
    void store(std::shared_ptr<T> value)
    {
        std::atomic_store(&value, std::move(value));
    }
    void store(std::shared_ptr<T> value, std::memory_order mo)
    {
        std::atomic_store_explicit(&value, std::move(value), mo);
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
    bool atomic_compare_exchange_weak(std::shared_ptr<T> &expected, std::shared_ptr<T> desired, std::memory_order success, std::memory_order failure)
    {
        return std::atomic_compare_exchange_weak_explicit(&value, &expected, std::move(desired), success, failure);
    }
    bool atomic_compare_exchange_weak(std::shared_ptr<T> &expected, std::shared_ptr<T> desired, std::memory_order mo = std::memory_order::memory_order_seq_cst)
    {
        std::memory_order success = mo, failure = mo;
        if(mo == std::memory_order::memory_order_acq_rel)
            failure = std::memory_order::memory_order_acquire;
        else if(mo == std::memory_order::memory_order_release)
            failure = std::memory_order::memory_order_relaxed;
        return std::atomic_compare_exchange_weak_explicit(&value, &expected, std::move(desired), success, failure);
    }
    bool atomic_compare_exchange_strong(std::shared_ptr<T> &expected, std::shared_ptr<T> desired, std::memory_order success, std::memory_order failure)
    {
        return std::atomic_compare_exchange_strong_explicit(&value, &expected, std::move(desired), success, failure);
    }
    bool atomic_compare_exchange_strong(std::shared_ptr<T> &expected, std::shared_ptr<T> desired, std::memory_order mo = std::memory_order::memory_order_seq_cst)
    {
        std::memory_order success = mo, failure = mo;
        if(mo == std::memory_order::memory_order_acq_rel)
            failure = std::memory_order::memory_order_acquire;
        else if(mo == std::memory_order::memory_order_release)
            failure = std::memory_order::memory_order_relaxed;
        return std::atomic_compare_exchange_strong_explicit(&value, &expected, std::move(desired), success, failure);
    }
};
}
}

#endif // ATOMIC_SHARED_PTR_H_INCLUDED
