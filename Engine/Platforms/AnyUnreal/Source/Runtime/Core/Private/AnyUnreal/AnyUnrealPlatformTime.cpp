#include "AnyUnreal/AnyUnrealPlatformTime.h"
#include "AnyUnreal/Kernel/Calendar.h"

namespace
{
    void assignTime(const AnyUnreal::Kernel::CalendarTime& time, int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
    {
        year = time.Year;
        month = time.Month;
        dayOfWeek = time.DayOfWeek;
        day = time.Day;
        hour = time.Hour;
        minute = time.Minute;
        second = time.Second;
        millisecond = time.Millisecond;
    }
}

void FAnyUnrealPlatformTime::SystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    assignTime(AnyUnreal::Kernel::Calendar::Local(AnyUnreal::Kernel::Clock::UnixTime()), year, month, dayOfWeek, day, hour, minute, second, millisecond);
}

void FAnyUnrealPlatformTime::UtcTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    assignTime(AnyUnreal::Kernel::Calendar::Utc(AnyUnreal::Kernel::Clock::UnixTime()), year, month, dayOfWeek, day, hour, minute, second, millisecond);
}
