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
#include "util/atomic_shared_ptr.h"
#include <utility>
#include "util/cpu_relax.h"

namespace programmerjake
{
namespace voxels
{
namespace
{
constexpr size_t atomic_shared_ptr_locks_count = 32; // must be a power of 2
constexpr size_t atomic_shared_ptr_locks_mask = atomic_shared_ptr_locks_count - 1;
std::atomic_flag atomic_shared_ptr_lock01 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock02 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock03 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock04 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock05 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock06 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock07 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock08 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock09 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock10 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock11 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock12 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock13 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock14 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock15 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock16 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock17 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock18 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock19 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock20 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock21 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock22 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock23 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock24 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock25 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock26 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock27 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock28 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock29 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock30 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock31 = ATOMIC_FLAG_INIT;
std::atomic_flag atomic_shared_ptr_lock32 = ATOMIC_FLAG_INIT;
std::atomic_flag *atomic_shared_ptr_locks[atomic_shared_ptr_locks_count] =
{
    &atomic_shared_ptr_lock01,
    &atomic_shared_ptr_lock02,
    &atomic_shared_ptr_lock03,
    &atomic_shared_ptr_lock04,
    &atomic_shared_ptr_lock05,
    &atomic_shared_ptr_lock06,
    &atomic_shared_ptr_lock07,
    &atomic_shared_ptr_lock08,
    &atomic_shared_ptr_lock09,
    &atomic_shared_ptr_lock10,
    &atomic_shared_ptr_lock11,
    &atomic_shared_ptr_lock12,
    &atomic_shared_ptr_lock13,
    &atomic_shared_ptr_lock14,
    &atomic_shared_ptr_lock15,
    &atomic_shared_ptr_lock16,
    &atomic_shared_ptr_lock17,
    &atomic_shared_ptr_lock18,
    &atomic_shared_ptr_lock19,
    &atomic_shared_ptr_lock20,
    &atomic_shared_ptr_lock21,
    &atomic_shared_ptr_lock22,
    &atomic_shared_ptr_lock23,
    &atomic_shared_ptr_lock24,
    &atomic_shared_ptr_lock25,
    &atomic_shared_ptr_lock26,
    &atomic_shared_ptr_lock27,
    &atomic_shared_ptr_lock28,
    &atomic_shared_ptr_lock29,
    &atomic_shared_ptr_lock30,
    &atomic_shared_ptr_lock31,
    &atomic_shared_ptr_lock32,
};
}

atomic_shared_ptr_mutex_holder::atomic_shared_ptr_mutex_holder(void *theAtomicSharedPtr) noexcept
    : theLock((void *)atomic_shared_ptr_locks[std::hash<void *>()(theAtomicSharedPtr) & atomic_shared_ptr_locks_mask])
{
}

void atomic_shared_ptr_mutex_holder::lock()
{
    std::atomic_flag *theFlag = (std::atomic_flag *)theLock;
    while(theFlag->test_and_set(std::memory_order_acquire))
        cpu_relax();
}

bool atomic_shared_ptr_mutex_holder::try_lock()
{
    std::atomic_flag *theFlag = (std::atomic_flag *)theLock;
    if(theFlag->test_and_set(std::memory_order_acquire))
        return false;
    return true;
}

void atomic_shared_ptr_mutex_holder::unlock()
{
    std::atomic_flag *theFlag = (std::atomic_flag *)theLock;
    theFlag->clear(std::memory_order_release);
}
}
}
