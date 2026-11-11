#ifndef UNREALPROSPERO_KERNEL_CALENDAR_H
#define UNREALPROSPERO_KERNEL_CALENDAR_H

#include "UnrealProspero/Kernel/Clock.h"
#include <cstdint>

namespace UnrealProspero::Kernel
{
    struct CalendarTime
    {
        std::int32_t Year;
        std::int32_t Month;
        std::int32_t DayOfWeek;
        std::int32_t Day;
        std::int32_t Hour;
        std::int32_t Minute;
        std::int32_t Second;
        std::int32_t Millisecond;
    };

    class Calendar
    {
    public:
        static CalendarTime Utc(WallTime time);
        static CalendarTime Local(WallTime time);
    };
}

#endif
