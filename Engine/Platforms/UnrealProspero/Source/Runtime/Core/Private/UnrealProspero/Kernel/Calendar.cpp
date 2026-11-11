#include "UnrealProspero/Kernel/Calendar.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <chrono>
#include <stdexcept>

namespace UnrealProspero::Kernel
{
    CalendarTime Calendar::Utc(WallTime time)
    {
        if (time.Nanoseconds >= Clock::Frequency) throw std::invalid_argument("Invalid calendar nanoseconds");
        constexpr std::int64_t secondsPerDay = 86400;
        auto days = time.Seconds / secondsPerDay;
        auto seconds = time.Seconds % secondsPerDay;
        if (seconds < 0)
        {
            --days;
            seconds += secondsPerDay;
        }
        constexpr auto firstDay = std::chrono::sys_days{std::chrono::year::min() / std::chrono::January / 1}.time_since_epoch().count();
        constexpr auto lastDay = std::chrono::sys_days{std::chrono::year::max() / std::chrono::December / 31}.time_since_epoch().count();
        if (days < firstDay || days > lastDay) throw std::out_of_range("Calendar time is outside the supported year range");
        const std::chrono::sys_days date{std::chrono::days{days}};
        const std::chrono::year_month_day calendar{date};
        return {
            static_cast<int>(calendar.year()),
            static_cast<std::int32_t>(static_cast<unsigned>(calendar.month())),
            static_cast<std::int32_t>(std::chrono::weekday{date}.c_encoding()),
            static_cast<std::int32_t>(static_cast<unsigned>(calendar.day())),
            static_cast<std::int32_t>(seconds / 3600),
            static_cast<std::int32_t>((seconds / 60) % 60),
            static_cast<std::int32_t>(seconds % 60),
            static_cast<std::int32_t>(time.Nanoseconds / 1000000)
        };
    }

    CalendarTime Calendar::Local(WallTime time)
    {
        Utc(time);
        std::int64_t localSeconds = 0;
        CheckResult(sceKernelConvertUtcToLocaltime(time.Seconds, &localSeconds, nullptr, nullptr), "sceKernelConvertUtcToLocaltime");
        return Utc({localSeconds, time.Nanoseconds});
    }
}
