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
#include "util/date_time.h"
#include "util/util.h"
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cwctype>
#include <vector>
#include "util/string_cast.h"
#include "util/checked_array.h"

#if _WIN64 || _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif __ANDROID || __APPLE__ || __linux || __unix || __posix
#include <time.h>
#else
#error unknown platform
#endif

namespace programmerjake
{
namespace voxels
{
std::chrono::system_clock::time_point DateTime::makeEpoch()
{
#if _WIN64 || _WIN32 || __ANDROID__ || __APPLE__ || __linux || __unix || __posix
    return std::chrono::system_clock::from_time_t(0);
#else
#error unknown platform
#endif
}

std::chrono::seconds DateTime::getTimeZoneOffset(DateTime time)
{
#if _WIN64 || _WIN32
    TIME_ZONE_INFORMATION tzInfo;
    if(GetTimeZoneInformation(&tzInfo) == TIME_ZONE_ID_INVALID)
    {
        // GetTimeZoneInformation failed: use UTC
        return std::chrono::seconds(0);
    }
    if(isDaylightSavingsTime(time))
    {
        return std::chrono::seconds((std::int64_t)-60 * (tzInfo.DaylightBias + tzInfo.Bias));
    }
    return std::chrono::seconds((std::int64_t)-60 * (tzInfo.StandardBias + tzInfo.Bias));
#elif __ANDROID__ || __APPLE__ || __linux || __unix || __posix
    std::tm tmStruct;
    std::time_t timeTValue = time.asTimeT();
    gmtime_r(&timeTValue, &tmStruct);
    tmStruct.tm_isdst = isDaylightSavingsTime(time) ? 1 : 0;
    return std::chrono::seconds(timeTValue - mktime(&tmStruct));
#else
#error unknown platform
#endif
}

bool DateTime::isDaylightSavingsTime(DateTime time)
{
#if _WIN64 || _WIN32
    TIME_ZONE_INFORMATION tzInfo;
    DWORD gtziRetVal = GetTimeZoneInformation(&tzInfo);
    if(gtziRetVal == TIME_ZONE_ID_INVALID)
    {
        // GetTimeZoneInformation failed: use UTC
        return false;
    }
    FIXME_MESSAGE(using current dst setting instead of the dst setting for the passed in time)
    if(gtziRetVal == TIME_ZONE_ID_DAYLIGHT)
        return true;
    return false;
#elif __ANDROID__ || __APPLE__ || __linux || __unix || __posix
    std::tm tmStruct;
    std::time_t timeTValue = time.asTimeT();
    localtime_r(&timeTValue, &tmStruct);
    return tmStruct.tm_isdst;
#else
#error unknown platform
#endif
}

DateTime::DateTime(std::tm time, bool isLocalTime)
    : millisecondsSincePosixTimeEpoch()
{
    // normalize
    time.tm_isdst = 0;
    std::tm time2 = time;
    std::mktime(&time2);
    time.tm_isdst = time2.tm_isdst; // so the hour is not adjusted
    std::mktime(&time);
    // derived from The Open Group Base Specifications Issue 7 4.15 http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
    std::int64_t seconds = time.tm_sec;
    seconds += (std::int64_t)time.tm_min * 60;
    seconds += (std::int64_t)time.tm_hour * 3600;
    seconds += (std::int64_t)time.tm_yday * 86400;
    std::int64_t year = time.tm_year;
    seconds += (year - 70) * 31536000;
    // using floorDivide makes this valid for dates before January 1 of 1970
    seconds += floorDivide(year - 69, 4) * 86400;
    seconds -= floorDivide(year - 1, 100) * 86400;
    seconds += floorDivide(year + 299, 400) * 86400;
    millisecondsSincePosixTimeEpoch = 1000 * seconds;
    if(isLocalTime)
        millisecondsSincePosixTimeEpoch -= getTimeZoneOffset(*this).count() * (std::uint64_t)1000;
}

namespace
{
int compareCaseInsensitive(std::size_t aStart, std::size_t aLength, const std::wstring &a, std::size_t bStart, std::size_t bLength, const wchar_t *const b)
{
    if(aStart >= a.size())
        aLength = 0;
    else if(aLength > a.size() - aStart)
        aLength = a.size() - aStart;
    std::size_t bSize = std::wcslen(b);
    if(bStart >= bSize)
        bLength = 0;
    else if(bLength > bSize - bStart)
        bLength = bSize - bStart;
    for(std::size_t i = 0; i < aLength && i < bLength; i++)
    {
        wchar_t aCh = a[i + aStart];
        wchar_t bCh = b[i + bStart];
        if(aCh == bCh)
            continue;
        if(std::towlower(aCh) == std::towlower(bCh))
            continue;
        if(aCh < bCh)
            return -1;
        return 1;
    }
    if(aLength < bLength)
        return -1;
    if(aLength > bLength)
        return 1;
    return 0;
}

int compareCaseInsensitive(std::size_t aStart, std::size_t aLength, const wchar_t *const a, std::size_t bStart, std::size_t bLength, const std::wstring &b)
{
    return -compareCaseInsensitive(bStart, bLength, b, aStart, aLength, a);
}

int compareCaseInsensitive(const wchar_t *const a, const std::wstring &b)
{
    return compareCaseInsensitive(0, std::wcslen(a), a, 0, b.size(), b);
}

const wchar_t *const weekNamesLong[7] =
{
    L"Sunday",
    L"Monday",
    L"Tuesday",
    L"Wednesday",
    L"Thursday",
    L"Friday",
    L"Saturday"
};

const wchar_t *const weekNamesShort[7] =
{
    L"Sun",
    L"Mon",
    L"Tue",
    L"Wed",
    L"Thu",
    L"Fri",
    L"Sat"
};

const wchar_t *const monthNamesLong[12] =
{
    L"January",
    L"February",
    L"March",
    L"April",
    L"May",
    L"June",
    L"July",
    L"August",
    L"September",
    L"October",
    L"November",
    L"December"
};

const wchar_t *const monthNamesShort[12] =
{
    L"Jan",
    L"Feb",
    L"Mar",
    L"Apr",
    L"May",
    L"Jun",
    L"Jul",
    L"Aug",
    L"Sep",
    L"Oct",
    L"Nov",
    L"Dec"
};

enum class DateTokenType
{
    Eof,
    Whitespace,
    Number,
    WeekName,
    MonthName,
    Colon,
    Slash,
    Hyphen,
    Plus,
    Identifier,
    AmPm,
};

class DateTokenizer final
{
    DateTokenizer(const DateTokenizer &) = delete;
    DateTokenizer &operator =(const DateTokenizer &) = delete;
private:
    const std::wstring &input;
public:
    explicit DateTokenizer(const std::wstring &input)
        : input(input)
    {
        next();
    }
private:
    std::size_t currentInputLocation = 0;
    std::wint_t peekCh()
    {
        if(currentInputLocation >= input.size())
            return WEOF;
        return input[currentInputLocation];
    }
    std::wint_t getCh()
    {
        std::wint_t retval = peekCh();
        currentInputLocation++;
        return retval;
    }
public:
    DateTokenType type = DateTokenType::Eof;
    std::wstring value = L"";
    std::int64_t intValue = -1;
    void next()
    {
        type = DateTokenType::Eof;
        value = L"";
        intValue = -1;
        if(peekCh() == WEOF)
            return;
        if(std::iswspace(peekCh()))
        {
            type = DateTokenType::Whitespace;
            while(peekCh() != WEOF && std::iswspace(peekCh()))
            {
                value += (wchar_t)getCh();
            }
            return;
        }
        if(std::iswdigit(peekCh()))
        {
            type = DateTokenType::Number;
            while(peekCh() != WEOF && std::iswdigit(peekCh()))
            {
                value += (wchar_t)getCh();
            }
            std::wistringstream ss(value);
            ss >> intValue;
            return;
        }
        if(std::iswalpha(peekCh()))
        {
            type = DateTokenType::Identifier;
            while(peekCh() != WEOF && std::iswalpha(peekCh()))
            {
                value += (wchar_t)getCh();
            }
            intValue = 0;
            for(const wchar_t *weekName : weekNamesLong)
            {
                if(compareCaseInsensitive(weekName, value) == 0)
                {
                    type = DateTokenType::WeekName;
                    return;
                }
                intValue++;
            }
            intValue = 0;
            for(const wchar_t *weekName : weekNamesShort)
            {
                if(compareCaseInsensitive(weekName, value) == 0)
                {
                    type = DateTokenType::WeekName;
                    return;
                }
                intValue++;
            }
            intValue = 0;
            for(const wchar_t *monthName : monthNamesLong)
            {
                if(compareCaseInsensitive(monthName, value) == 0)
                {
                    type = DateTokenType::MonthName;
                    return;
                }
                intValue++;
            }
            intValue = 0;
            for(const wchar_t *monthName : monthNamesShort)
            {
                if(compareCaseInsensitive(monthName, value) == 0)
                {
                    type = DateTokenType::MonthName;
                    return;
                }
                intValue++;
            }
            if(compareCaseInsensitive(L"AM", value) == 0)
            {
                type = DateTokenType::AmPm;
                intValue = 0;
                return;
            }
            if(compareCaseInsensitive(L"PM", value) == 0)
            {
                type = DateTokenType::AmPm;
                intValue = 12;
                return;
            }
            intValue = -1;
            return;
        }
        switch(peekCh())
        {
        case L':':
            type = DateTokenType::Colon;
            value += (wchar_t)getCh();
            return;
        case L'/':
            type = DateTokenType::Slash;
            value += (wchar_t)getCh();
            return;
        case L'-':
            type = DateTokenType::Hyphen;
            value += (wchar_t)getCh();
            intValue = -1; // sign
            return;
        case L'+':
            type = DateTokenType::Plus;
            value += (wchar_t)getCh();
            intValue = 1; // sign
            return;
        default:
            throw DateTime::FormatException("invalid character : '" + string_cast<std::string>(std::wstring(1, (wchar_t)peekCh())) + "'");
        }
    }
};

enum class FormatTokenType
{
    Eof,
    OptionalWhitespace, // ' '
    RequiredWhitespace, // '_'
    WeekName, // 'W'
    DayOfMonth, // 'D'
    MonthNumber, // 'M'
    MonthName, // 'N'
    ShortYear, // 'y'
    FullYear, // 'Y'
    YearWithOptionalSign, // '+Y'
    Slash, // '/'
    Colon, // ':'
    Hyphen, // '-'
    Hour, // 'h'
    AmPm, // 'p'
    Minute, // 'm'
    Second, // 's'
};

static const wchar_t *const dateFormats[] =
{
    L" +Y-M-D ",
    L" +Y-M-D_h:m:s ",
    L" +Y-M-D_h:m:s_p ",
    L" +Y-M-D_h:m ",
    L" +Y-M-D_h:m_p ",
    L" +Y-M-D_h_p ",
    L" W_N_D_Y ",
    L" N_D_Y ",
    L" N_D ",
    L" D_N ",
    L" N_D_h:m:s ",
    L" D_N_h:m:s ",
    L" N_D_h:m ",
    L" D_N_h:m ",
    L" N_D_h:m:s_p ",
    L" D_N_h:m:s_p ",
    L" N_D_h:m_p ",
    L" D_N_h:m_p ",
    L" N_D_h_p ",
    L" D_N_h_p ",
    L" W_N_D_Y_h:m:s ",
    L" W_N_D_Y_h:m:s_p ",
    L" W_N_D_Y_h:m ",
    L" W_N_D_Y_h:m_p ",
    L" W_N_D_Y_h_p ",
    L" N_D_Y_h:m:s ",
    L" N_D_Y_h:m:s_p ",
    L" N_D_Y_h:m ",
    L" N_D_Y_h:m_p ",
    L" N_D_Y_h_p ",
    L" W_D_N_Y ",
    L" D_N_Y ",
    L" W_D_N_Y_h:m:s ",
    L" W_D_N_Y_h:m:s_p ",
    L" W_D_N_Y_h:m ",
    L" W_D_N_Y_h:m_p ",
    L" W_D_N_Y_h_p ",
    L" D_N_Y_h:m:s ",
    L" D_N_Y_h:m:s_p ",
    L" D_N_Y_h:m ",
    L" D_N_Y_h:m_p ",
    L" D_N_Y_h_p ",
    L" M/D/Y_h:m:s ",
    L" M/D/y_h:m:s ",
    L" M/D/Y_h:m:s_p ",
    L" M/D/y_h:m:s_p ",
    L" M/D/Y_h:m ",
    L" M/D/y_h:m ",
    L" M/D/Y_h:m_p ",
    L" M/D/y_h:m_p ",
    L" M/D/Y_h_p ",
    L" M/D/y_h_p ",
    L" M/D/Y ",
    L" M/D/y ",
    L" h:m:s ",
    L" h:m:s_p ",
    L" h:m ",
    L" h:m_p ",
    L" h_p ",
};

typedef checked_array<std::wstring, sizeof(dateFormats) / sizeof(dateFormats[0])> DateFormatsArrayType;

DateFormatsArrayType makeDateFormatsArray()
{
    DateFormatsArrayType retval;
    for(std::size_t i = 0; i < retval.size(); i++)
        retval[i] = dateFormats[i];
    return std::move(retval);
}

const DateFormatsArrayType &getDateFormats()
{
    static DateFormatsArrayType retval = makeDateFormatsArray();
    return retval;
}

struct DateFormatTokenizer final
{
private:
    const std::wstring &format;
public:
    explicit DateFormatTokenizer(const std::wstring &format)
        : format(format)
    {
        next();
    }
private:
    std::size_t currentPosition = 0;
    wint_t peekCh()
    {
        if(currentPosition >= format.size())
            return WEOF;
        return format[currentPosition];
    }
    wint_t getCh()
    {
        wint_t retval = peekCh();
        currentPosition++;
        return retval;
    }
#ifndef NDEBUG
    bool gotHour = false;
#endif
public:
    FormatTokenType token = FormatTokenType::Eof;
    void next()
    {
        token = FormatTokenType::Eof;
        if(peekCh() == WEOF)
            return;
        switch(peekCh())
        {
        case L' ':
            token = FormatTokenType::OptionalWhitespace;
            getCh();
            return;
        case L'_':
            token = FormatTokenType::RequiredWhitespace;
            getCh();
            return;
        case L'+':
            token = FormatTokenType::YearWithOptionalSign;
            getCh();
            assert(peekCh() == L'Y');
            getCh();
            return;
        case L'W':
            token = FormatTokenType::WeekName;
            getCh();
            return;
        case L'D':
            token = FormatTokenType::DayOfMonth;
            getCh();
            return;
        case L'M':
            token = FormatTokenType::MonthNumber;
            getCh();
            return;
        case L'N':
            token = FormatTokenType::MonthName;
            getCh();
            return;
        case L'y':
            token = FormatTokenType::ShortYear;
            getCh();
            return;
        case L'Y':
            token = FormatTokenType::FullYear;
            getCh();
            return;
        case L'/':
            token = FormatTokenType::Slash;
            getCh();
            return;
        case L':':
            token = FormatTokenType::Colon;
            getCh();
            return;
        case L'-':
            token = FormatTokenType::Hyphen;
            getCh();
            return;
        case L'h':
            token = FormatTokenType::Hour;
#ifndef NDEBUG
            gotHour = true;
#endif
            getCh();
            return;
        case L'p':
            token = FormatTokenType::AmPm;
#ifndef NDEBUG
            assert(gotHour);
#endif
            getCh();
            return;
        case L'm':
            token = FormatTokenType::Minute;
            getCh();
            return;
        case L's':
            token = FormatTokenType::Second;
            getCh();
            return;
        default:
            UNREACHABLE();
        }
    }
};

struct FormatParser final
{
    std::tm structTm;
    bool setWeekDay = false;
    DateFormatTokenizer tokenizer;
    int signValue = 0;
    std::unique_ptr<std::string> errorMessage = nullptr;
    std::size_t tokensAfterError = 0;
    explicit FormatParser(std::tm structTm, const std::wstring &format)
        : structTm(structTm), tokenizer(format)
    {
    }
    void setError(std::string msg)
    {
        errorMessage = std::unique_ptr<std::string>(new std::string(std::move(msg)));
    }
    void parseNext(DateTokenType type, const std::wstring value, std::int64_t intValue)
    {
        if(errorMessage)
        {
            tokensAfterError++;
            return;
        }
        while(tokenizer.token == FormatTokenType::OptionalWhitespace)
        {
            tokenizer.next();
            if(type == DateTokenType::Whitespace)
                return;
        }
        switch(tokenizer.token)
        {
        case FormatTokenType::Eof:
            if(type != DateTokenType::Eof)
            {
                setError("extra tokens");
                return;
            }
            return;
        case FormatTokenType::OptionalWhitespace:
            return; // handled before switch
        case FormatTokenType::RequiredWhitespace:
            if(type != DateTokenType::Whitespace)
            {
                setError("expected space");
                return;
            }
            tokenizer.next();
            return;
        case FormatTokenType::WeekName:
            if(type != DateTokenType::WeekName)
            {
                setError("expected week name");
                return;
            }
            structTm.tm_wday = (int)intValue;
            setWeekDay = true;
            tokenizer.next();
            return;
        case FormatTokenType::DayOfMonth:
            if(type != DateTokenType::Number)
            {
                setError("expected day-of-month");
                return;
            }
            if(intValue < 1 || intValue > 31)
            {
                setError("day-of-month out of range");
                return;
            }
            structTm.tm_mday = (int)intValue;
            tokenizer.next();
            return;
        case FormatTokenType::MonthNumber:
            if(type != DateTokenType::Number)
            {
                setError("expected month number");
                return;
            }
            if(intValue < 1 || intValue > 12)
            {
                setError("month out of range");
                return;
            }
            structTm.tm_mon = (int)intValue - 1;
            tokenizer.next();
            return;
        case FormatTokenType::MonthName:
            if(type != DateTokenType::MonthName)
            {
                setError("expected month name");
                return;
            }
            structTm.tm_mon = (int)intValue;
            tokenizer.next();
            return;
        case FormatTokenType::ShortYear:
            if(type != DateTokenType::Number || value.size() != 2)
            {
                setError("expected short year");
                return;
            }
            if(intValue < 69)
                structTm.tm_year = 2000 + (int)intValue;
            else
                structTm.tm_year = 1900 + (int)intValue;
            structTm.tm_year -= 1900;
            tokenizer.next();
            return;
        case FormatTokenType::FullYear:
            if(type != DateTokenType::Number || value.size() <= 2)
            {
                setError("expected full year");
                return;
            }
            if(intValue < 1 || intValue > 100000)
            {
                setError("year out of range");
                return;
            }
            structTm.tm_year = (int)(intValue - 1900);
            tokenizer.next();
            return;
        case FormatTokenType::YearWithOptionalSign:
            if(signValue == 0 && (type == DateTokenType::Plus || type == DateTokenType::Hyphen))
            {
                signValue = (int)intValue;
                return;
            }
            else if(signValue == 0)
                signValue = 1;
            if(type != DateTokenType::Number)
            {
                setError("expected year");
                return;
            }
            intValue *= signValue;
            signValue = 0;
            if(intValue < -100000 || intValue > 100000)
            {
                setError("year out of range");
                return;
            }
            structTm.tm_year = (int)(intValue - 1900);
            tokenizer.next();
            return;
        case FormatTokenType::Slash:
            if(type != DateTokenType::Slash)
            {
                setError("expected '/'");
                return;
            }
            tokenizer.next();
            return;
        case FormatTokenType::Colon:
            if(type != DateTokenType::Colon)
            {
                setError("expected ':'");
                return;
            }
            tokenizer.next();
            return;
        case FormatTokenType::Hyphen:
            if(type != DateTokenType::Hyphen)
            {
                setError("expected '-'");
                return;
            }
            tokenizer.next();
            return;
        case FormatTokenType::Hour:
            if(type != DateTokenType::Number)
            {
                setError("expected hour");
                return;
            }
            if(intValue < 0 || intValue > 23)
            {
                setError("hour out of range");
                return;
            }
            structTm.tm_hour = (int)intValue;
            tokenizer.next();
            return;
        case FormatTokenType::AmPm:
            if(type != DateTokenType::AmPm)
            {
                setError("expected am/pm");
                return;
            }
            if(structTm.tm_hour < 1 || structTm.tm_hour > 12)
            {
                setError("hour out of range");
                return;
            }
            structTm.tm_hour = (structTm.tm_hour % 12) + (int)intValue;
            tokenizer.next();
            return;
        case FormatTokenType::Minute:
            if(type != DateTokenType::Number)
            {
                setError("expected minutes");
                return;
            }
            if(intValue < 0 || intValue >= 60)
            {
                setError("minutes out of range");
                return;
            }
            structTm.tm_min = (int)intValue;
            tokenizer.next();
            return;
        case FormatTokenType::Second:
            if(type != DateTokenType::Number)
            {
                setError("expected seconds");
                return;
            }
            if(intValue < 0 || intValue > 60)
            {
                setError("seconds out of range");
                return;
            }
            if(intValue == 60)
            {
                setError("leap seconds are not supported");
                return;
            }
            structTm.tm_sec = (int)intValue;
            tokenizer.next();
            return;
        }
        UNREACHABLE();
    }
};
}

DateTime DateTime::fromString(std::wstring time)
{
    std::tm structTm = now().asStructTm();
    structTm.tm_sec = 0;
    structTm.tm_min = 0;
    structTm.tm_hour = 12; // noon
    structTm.tm_isdst = -1;
    DateTokenizer t(time);
    std::vector<FormatParser> formatParsers;
    formatParsers.reserve(getDateFormats().size());
    for(const std::wstring &format : getDateFormats())
    {
        formatParsers.emplace_back(structTm, format);
    }
    while(true)
    {
        for(FormatParser &fp : formatParsers)
        {
            fp.parseNext(t.type, t.value, t.intValue);
        }
        if(t.type == DateTokenType::Eof)
            break;
        t.next();
    }
    std::size_t smallestTokensAfterError = 0;
    std::unique_ptr<std::string> errorMessage = nullptr;
    for(FormatParser &fp : formatParsers)
    {
        if(fp.errorMessage == nullptr)
        {
            structTm = fp.structTm;
            if(structTm.tm_isdst < 0)
            {
                std::mktime(&structTm);
                fp.structTm.tm_isdst = structTm.tm_isdst;
                structTm = fp.structTm;
            }
            std::mktime(&structTm);
            return fromStructTm(structTm);
        }
        if(errorMessage == nullptr || smallestTokensAfterError > fp.tokensAfterError)
        {
            errorMessage = std::move(fp.errorMessage);
            smallestTokensAfterError = fp.tokensAfterError;
        }
    }
    assert(errorMessage != nullptr);
    throw FormatException(*errorMessage);
}

std::tm DateTime::asStructTm(bool isLocalTime) const
{
    std::tm retval = std::tm();
    if(isLocalTime)
    {
        retval = DateTime(millisecondsSincePosixTimeEpoch + (std::int64_t)getTimeZoneOffset(*this).count() * 1000).asStructTm(false);
        retval.tm_isdst = isDaylightSavingsTime(*this) ? 1 : 0;
        return retval;
    }
    retval.tm_isdst = 0;
    std::int64_t seconds = floorDivide(millisecondsSincePosixTimeEpoch, 1000);
    std::int64_t minutes = seconds / 60;
    seconds %= 60;
    if(seconds < 0)
    {
        seconds += 60;
        minutes--;
    }
    retval.tm_sec = (int)seconds;
    std::int64_t hours = minutes / 60;
    minutes %= 60;
    if(minutes < 0)
    {
        minutes += 60;
        hours--;
    }
    retval.tm_min = (int)minutes;
    std::int64_t days = hours / 24;
    hours %= 24;
    if(hours < 0)
    {
        hours += 24;
        days--;
    }
    retval.tm_hour = (int)hours;
    std::int64_t weekDay = (days - 3) % 7; // day 0 is Thursday
    if(weekDay < 0)
        weekDay += 7;
    retval.tm_wday = (int)weekDay;
    const int dayOffsetIn400YearCycle = -10957;
    days += dayOffsetIn400YearCycle;
    std::int64_t countOf400YearCycles = days / daysIn400Years;
    int dayIn400YearCycle = days % daysIn400Years;
    if(dayIn400YearCycle < 0)
    {
        dayIn400YearCycle += daysIn400Years;
        countOf400YearCycles--;
    }
    int yearIn400YearCycle = 0;
    int dayInYear = dayIn400YearCycle;
    int currentDaysInYear = daysInYear(yearIn400YearCycle);
    while(dayInYear >= currentDaysInYear)
    {
        dayInYear -= currentDaysInYear;
        yearIn400YearCycle++;
        currentDaysInYear = daysInYear(yearIn400YearCycle);
    }
    retval.tm_yday = dayInYear;
    int startYearOf400YearCycle = 2000 + 400 * (int)countOf400YearCycles;
    retval.tm_year = startYearOf400YearCycle + yearIn400YearCycle - 1900;
    bool leapYear = isLeapYear(yearIn400YearCycle);
    int dayInMonth = dayInYear;
    int month = 0;
    int currentDaysInMonth = daysInMonth(month, leapYear);
    while(dayInMonth >= currentDaysInMonth)
    {
        dayInMonth -= currentDaysInMonth;
        month++;
        currentDaysInMonth = daysInMonth(month, leapYear);
    }
    retval.tm_mon = month;
    retval.tm_mday = (dayInMonth + 1);
    return retval;
}

std::wstring DateTime::asString(bool isLocalTime) const
{
    std::wostringstream ss;
    std::tm time = asStructTm(isLocalTime);
    ss << std::setfill(L'0');
    ss << std::setw(4) << (time.tm_year + 1900);
    ss << L"-";
    ss << std::setw(2) << (1 + time.tm_mon);
    ss << L"-";
    ss << std::setw(2) << time.tm_mday;
    ss << L" ";
    ss << std::setw(2) << time.tm_hour;
    ss << L":";
    ss << std::setw(2) << time.tm_min;
    ss << L":";
    ss << std::setw(2) << time.tm_sec;
    return ss.str();
}

#if 0
namespace
{
initializer init1([]()
{
    std::tm structTm1 = std::tm();
    structTm1.tm_sec = 0;
    structTm1.tm_min = 0;
    structTm1.tm_hour = 0;
    structTm1.tm_mday = 1;
    structTm1.tm_mon = 4;
    structTm1.tm_year = 70;
    std::cout << string_cast<std::string>(DateTime::fromStructTm(structTm1, false).asString(false)) << std::endl;
    std::cout << string_cast<std::string>(DateTime::now().asString(false)) << std::endl;
    std::cout << string_cast<std::string>(DateTime::now().asString(true)) << std::endl;
    std::vector<std::wstring> strings =
    {
        L"-100-1-1",
        L"Monday September 14 2015 13:11:59",
        L"September 14 2015 13:11:59",
        L"14 September 2015 13:11:59",
        L"Sep 14 2015 1:11:59 pm",
        L"4/3/5",
        L"4/3/2005",
        L"2 pm",
        L"1/1/68",
        L"1/1/69",
        L"  Sep 22  ",
        L"22 Sep  ",
        L"100000-1-1",
        L"-100000-1-1",
    };
    for(const std::wstring &str : strings)
    {
        std::wstring result;
        try
        {
            result = DateTime::fromString(str).asString();
        }
        catch(DateTime::FormatException &e)
        {
            result = L"FormatException: " + string_cast<std::wstring>(e.what());
        }
        std::cout << "\"" << string_cast<std::string>(str) << "\" -> " << string_cast<std::string>(result) << std::endl;
    }
    std::exit(0);
});
}
#endif

}
}
