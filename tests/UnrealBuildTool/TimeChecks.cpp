#include "HAL/PlatformTime.h"
#include "AnyUnreal/Kernel/Calendar.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
    bool matches(const AnyUnreal::Kernel::CalendarTime& time, int year, int month, int day, int weekday, int hour, int minute, int second, int millisecond)
    {
        return time.Year == year && time.Month == month && time.Day == day && time.DayOfWeek == weekday && time.Hour == hour && time.Minute == minute && time.Second == second && time.Millisecond == millisecond;
    }

    template<typename TException, typename TOperation>
    bool rejects(TOperation operation)
    {
        try
        {
            operation();
        }
        catch (const TException&)
        {
            return true;
        }
        return false;
    }

    bool valid(const AnyUnreal::Kernel::CalendarTime& time)
    {
        return time.Month >= 1 && time.Month <= 12 && time.Day >= 1 && time.Day <= 31 && time.DayOfWeek >= 0 && time.DayOfWeek <= 6 && time.Hour >= 0 && time.Hour < 24 && time.Minute >= 0 && time.Minute < 60 && time.Second >= 0 && time.Second < 60 && time.Millisecond >= 0 && time.Millisecond < 1000;
    }
}

bool CheckTime()
{
    using AnyUnreal::Kernel::Calendar;
    if (!matches(Calendar::Utc({0, 0}), 1970, 1, 1, 4, 0, 0, 0, 0)) return false;
    if (!matches(Calendar::Utc({-1, 999999999}), 1969, 12, 31, 3, 23, 59, 59, 999)) return false;
    if (!matches(Calendar::Utc({951782400, 123456789}), 2000, 2, 29, 2, 0, 0, 0, 123)) return false;
    if (!matches(Calendar::Utc({2147483648, 0}), 2038, 1, 19, 2, 3, 14, 8, 0)) return false;
    if (!rejects<std::invalid_argument>([] { Calendar::Utc({0, 1000000000}); })) return false;
    if (!rejects<std::out_of_range>([] { Calendar::Utc({std::numeric_limits<std::int64_t>::min(), 0}); })) return false;
    if (!rejects<std::out_of_range>([] { Calendar::Utc({std::numeric_limits<std::int64_t>::max(), 0}); })) return false;
    if (FPlatformTime::SecondsToCycles64(1) != AnyUnreal::Kernel::Clock::Frequency) return false;
    if (FPlatformTime::SecondsToCycles64(0) != 0 || FPlatformTime::SecondsToCycles64(0.5) != 500000000) return false;
    if (!rejects<std::invalid_argument>([] { FPlatformTime::SecondsToCycles64(-1); })) return false;
    if (!rejects<std::invalid_argument>([] { FPlatformTime::SecondsToCycles64(std::numeric_limits<double>::infinity()); })) return false;
    if (!rejects<std::invalid_argument>([] { FPlatformTime::SecondsToCycles64(std::numeric_limits<double>::quiet_NaN()); })) return false;
    if (!rejects<std::overflow_error>([] { FPlatformTime::SecondsToCycles64(std::ldexp(1.0, 64)); })) return false;
    static_assert(FPlatformTime::ToSeconds(1000000) == 1);
    static_assert(FPlatformTime::ToMilliseconds(1000) == 1);
    static_assert(FPlatformTime::ToSeconds64(1000000000) == 1);
    static_assert(FPlatformTime::ToMilliseconds64(1000000000) == 1000);
    const auto start = FPlatformTime::InitTiming();
    const auto before = FPlatformTime::Cycles64();
    const auto shortCounter = FPlatformTime::Cycles();
    const auto after = FPlatformTime::Cycles64();
    if (after < before || FPlatformTime::Seconds() < start) return false;
    if (static_cast<uint32>(shortCounter - static_cast<uint32>(before / 1000)) > (after - before) / 1000 + 1) return false;
    AnyUnreal::Kernel::CalendarTime time{};
    FPlatformTime::UtcTime(time.Year, time.Month, time.DayOfWeek, time.Day, time.Hour, time.Minute, time.Second, time.Millisecond);
    if (!valid(time)) return false;
    FPlatformTime::SystemTime(time.Year, time.Month, time.DayOfWeek, time.Day, time.Hour, time.Minute, time.Second, time.Millisecond);
    return valid(time);
}
