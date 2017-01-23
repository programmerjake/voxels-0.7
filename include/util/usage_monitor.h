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
#include <chrono>
#include <utility>
#include <cassert>
#include "logging.h"
#include <cmath>

namespace programmerjake
{
namespace voxels
{
class ThreadUsageMonitor final
{
    typedef std::chrono::steady_clock clock;
    typedef clock::time_point time_point;
    typedef clock::duration duration;
    const std::wstring name;
    struct ConstantState final
    {
        float usageDecaySpeed;
        explicit ConstantState(float usageDecaySpeed = -1) : usageDecaySpeed(usageDecaySpeed)
        {
        }
    };
    const ConstantState constantState;
    struct State final
    {
        time_point lastTime;
        duration usedDuration;
        duration unusedDuration;
        float averageUsage;
        bool isUsed;
        State()
            : lastTime(clock::now()),
              usedDuration(0),
              unusedDuration(0),
              averageUsage(1),
              isUsed(true)
        {
        }
        void updateUsage(const ConstantState &constantState, time_point currentTime = clock::now())
        {
            duration deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            if(constantState.usageDecaySpeed >= 0)
            {
                float factor = std::exp(-std::chrono::duration_cast<std::chrono::duration<double>>(
                                             deltaTime).count() * constantState.usageDecaySpeed);
                averageUsage *= factor;
                if(isUsed)
                    averageUsage += 1 - factor;
            }
            else
            {
                if(isUsed)
                    usedDuration += deltaTime;
                else
                    unusedDuration += deltaTime;
                if(usedDuration.count() != 0 || unusedDuration.count() != 0)
                {
                    auto usedDurationF = std::chrono::duration_cast<std::chrono::duration<double>>(
                                             usedDuration).count();
                    auto unusedDurationF =
                        std::chrono::duration_cast<std::chrono::duration<double>>(unusedDuration)
                            .count();
                    averageUsage =
                        static_cast<float>(usedDurationF / (usedDurationF + unusedDurationF));
                }
            }
        }
        void startUsage(const ConstantState &constantState, time_point currentTime = clock::now())
        {
            assert(!isUsed);
            updateUsage(constantState, currentTime);
            isUsed = true;
        }
        void stopUsage(const ConstantState &constantState, time_point currentTime = clock::now())
        {
            assert(isUsed);
            updateUsage(constantState, currentTime);
            isUsed = false;
        }
    };
    State state;

public:
    explicit ThreadUsageMonitor(std::wstring name) // reports the overall usage
        : name(std::move(name)),
          constantState(),
          state()
    {
    }
    explicit ThreadUsageMonitor(std::wstring name,
                                float usageDecaySpeed) // reports the moving average usage
        : name(std::move(name)),
          constantState(usageDecaySpeed),
          state()
    {
    }
    template <typename Fn>
    void unusedCall(Fn fn)
    {
        state.stopUsage(constantState);
        try
        {
            fn();
        }
        catch(...)
        {
            state.startUsage(constantState);
            throw;
        }
        state.startUsage(constantState);
    }
    template <typename LockType, typename ConditionVariableType>
    void unusedWait(ConditionVariableType &condition, LockType &lock)
    {
        state.stopUsage(constantState);
        try
        {
            condition.wait(lock);
        }
        catch(...)
        {
            state.startUsage(constantState);
            throw;
        }
        state.startUsage(constantState);
    }
    void dump(TLS &tls) const
    {
        State state = this->state;
        state.updateUsage(constantState);
        getDebugLog(tls) << L"ThreadUsageMonitor(" << name << L"): useFraction = ";
        if(state.averageUsage < 0)
            getDebugLog(tls) << L"-%" << postnl;
        else
            getDebugLog(tls) << 100 * state.averageUsage << L"%" << postnl;
    }
};
}
}
