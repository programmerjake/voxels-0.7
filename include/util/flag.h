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
#ifndef FLAG_H_INCLUDED
#define FLAG_H_INCLUDED

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace programmerjake
{
namespace voxels
{
class flag final
{
private:
    mutable std::mutex lock;
    mutable std::condition_variable_any cond;
    std::atomic_bool value;
    mutable std::atomic_size_t waitingCount;
public:
    flag(bool value = false)
        : value(value)
    {
    }
    const flag &operator =(bool v)
    {
        if(value.exchange(v) != v)
        {
            if(waitingCount > 0)
                cond.notify_all();
        }

        return *this;
    }
    const flag &operator =(const flag &r)
    {
        return *this = (bool)r;
    }
    bool exchange(bool v)
    {
        bool retval = value.exchange(v);

        if(retval != v)
        {
            if(waitingCount > 0)
                cond.notify_all();
        }

        return retval;
    }
    operator bool() const
    {
        bool retval = value;
        return retval;
    }
    bool operator !() const
    {
        bool retval = value;
        return !retval;
    }
    void wait(bool v = true) const /// waits until value == v
    {
        if(v == value)
        {
            return;
        }

        waitingCount++;

        lock.lock();

        while(v != value)
        {
            cond.wait(lock);
        }

        lock.unlock();

        waitingCount--;
    }
    void waitThenReset(bool v = true) /// waits until value == v then set value to !v
    {
        if(v == value.exchange(!v))
        {
            if(waitingCount > 0)
                cond.notify_all();
            return;
        }

        waitingCount++;

        lock.lock();

        while(v != value.exchange(!v))
        {
            cond.wait(lock);
        }
        cond.notify_all();
        lock.unlock();
        waitingCount++;
    }
    void set()
    {
        *this = true;
    }
    void reset()
    {
        *this = false;
    }
};
}
}

#endif // FLAG_H_INCLUDED
