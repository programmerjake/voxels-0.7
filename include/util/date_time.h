/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
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
#ifndef DATE_TIME_H_INCLUDED
#define DATE_TIME_H_INCLUDED

#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <limits>
#include "stream/stream.h"

namespace programmerjake
{
namespace voxels
{
class DateTime final
{
private:
    std::int64_t millisecondsSincePosixTimeEpoch;
    static std::chrono::system_clock::time_point makeEpoch();

    // the integer equivalent of floor((double)dividend / divisor)
    static std::int64_t floorDivide(std::int64_t dividend, std::int64_t divisor)
    {
        std::int64_t remainder = dividend % divisor;
        std::int64_t quotient = dividend / divisor;
        if(remainder < 0)
            return quotient - 1;
        return quotient;
    }
public:
    static bool isLeapYear(std::int64_t year)
    {
        if(year % 4 != 0)
            return false;
        if(year % 100 != 0)
            return true;
        if(year % 400 != 0)
            return false;
        return true;
    }
    static int daysInYear(std::int64_t year)
    {
        return isLeapYear(year) ? 366 : 365;
    }
    static constexpr int daysIn400Years = 146097;
    static int daysInMonth(int monthsSinceJanuary, bool leapYear)
    {
        monthsSinceJanuary %= 12;
        if(monthsSinceJanuary < 0)
            monthsSinceJanuary += 12;
        static const int dayCount[12] =
        {
            31, // January
            28, // February
            31, // March
            30, // April
            31, // May
            30, // June
            31, // July
            31, // August
            30, // September
            31, // October
            30, // November
            31  // December
        };
        if(monthsSinceJanuary == 1) // February
            if(leapYear)
                return 29;
        return dayCount[monthsSinceJanuary];
    }
    static std::chrono::system_clock::time_point epoch()
    {
        static std::chrono::system_clock::time_point retval = makeEpoch();
        return retval;
    }
    DateTime()
        : DateTime(0)
    {
    }
private:
    explicit DateTime(std::uint64_t millisecondsSincePosixTimeEpoch)
        : millisecondsSincePosixTimeEpoch(millisecondsSincePosixTimeEpoch)
    {
    }
    explicit DateTime(std::tm time, bool isLocalTime);
public:
    static DateTime fromMillisecondsSincePosixTimeEpoch(std::uint64_t millisecondsSincePosixTimeEpoch)
    {
        return DateTime(millisecondsSincePosixTimeEpoch);
    }
    static DateTime fromTimePoint(std::chrono::system_clock::time_point timePoint)
    {
        return DateTime(std::chrono::duration_cast<std::chrono::milliseconds>(timePoint - epoch()).count());
    }
    static DateTime fromTimeT(std::time_t timeT)
    {
        return fromTimePoint(std::chrono::system_clock::from_time_t(timeT));
    }
    static DateTime fromStructTm(std::tm time, bool isLocalTime = true)
    {
        return DateTime(time, isLocalTime);
    }
    struct FormatException : public std::runtime_error
    {
        FormatException(const std::string &message)
            : runtime_error(message)
        {
        }
    };
    /** parses a date and time
     * @param time the string to parse
     * @throw DateTime::FormatException
     */
    static DateTime fromString(std::wstring time);
    static DateTime now()
    {
        return fromTimePoint(std::chrono::system_clock::now());
    }
    std::int64_t asMllisecondsSincePosixTimeEpoch() const
    {
        return millisecondsSincePosixTimeEpoch;
    }
    std::chrono::system_clock::time_point asTimePoint() const
    {
        return std::chrono::milliseconds(millisecondsSincePosixTimeEpoch) + epoch();
    }
    std::time_t asTimeT() const
    {
        return std::chrono::system_clock::to_time_t(asTimePoint());
    }
    void write(stream::Writer &writer) const
    {
        stream::write<std::int64_t>(writer, millisecondsSincePosixTimeEpoch);
    }
    static std::chrono::seconds getTimeZoneOffset(DateTime time = now());
    static bool isDaylightSavingsTime(DateTime time = now());
    static DateTime read(stream::Reader &reader)
    {
        return DateTime(stream::read<std::int64_t>(reader));
    }
    std::tm asStructTm(bool isLocalTime = true) const;
    std::wstring asString(bool isLocalTime = true) const;
    bool operator ==(const DateTime &rt) const
    {
        return millisecondsSincePosixTimeEpoch == rt.millisecondsSincePosixTimeEpoch;
    }
    bool operator !=(const DateTime &rt) const
    {
        return millisecondsSincePosixTimeEpoch != rt.millisecondsSincePosixTimeEpoch;
    }
    bool operator <=(const DateTime &rt) const
    {
        return millisecondsSincePosixTimeEpoch <= rt.millisecondsSincePosixTimeEpoch;
    }
    bool operator >=(const DateTime &rt) const
    {
        return millisecondsSincePosixTimeEpoch >= rt.millisecondsSincePosixTimeEpoch;
    }
    bool operator <(const DateTime &rt) const
    {
        return millisecondsSincePosixTimeEpoch < rt.millisecondsSincePosixTimeEpoch;
    }
    bool operator >(const DateTime &rt) const
    {
        return millisecondsSincePosixTimeEpoch > rt.millisecondsSincePosixTimeEpoch;
    }
    friend DateTime operator +(const std::chrono::milliseconds &d, const DateTime &dt)
    {
        return DateTime(dt.millisecondsSincePosixTimeEpoch + d.count());
    }
    friend std::chrono::milliseconds operator -(const DateTime &a, const DateTime &b)
    {
        return std::chrono::milliseconds(a.millisecondsSincePosixTimeEpoch - b.millisecondsSincePosixTimeEpoch);
    }
    DateTime operator +(const std::chrono::milliseconds &d) const
    {
        return DateTime(millisecondsSincePosixTimeEpoch + d.count());
    }
    DateTime operator -(const std::chrono::milliseconds &d) const
    {
        return DateTime(millisecondsSincePosixTimeEpoch - d.count());
    }
    DateTime &operator +=(const std::chrono::milliseconds &d)
    {
        millisecondsSincePosixTimeEpoch += d.count();
        return *this;
    }
    DateTime &operator -=(const std::chrono::milliseconds &d)
    {
        millisecondsSincePosixTimeEpoch -= d.count();
        return *this;
    }
    static DateTime min()
    {
        return DateTime(std::numeric_limits<std::int64_t>::min());
    }
    static DateTime max()
    {
        return DateTime(std::numeric_limits<std::int64_t>::max());
    }
};
}
}

#endif // DATE_TIME_H_INCLUDED
