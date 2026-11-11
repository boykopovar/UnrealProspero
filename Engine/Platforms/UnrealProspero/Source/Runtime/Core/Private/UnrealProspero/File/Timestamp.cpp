#include "UnrealProspero/File/Timestamp.h"
#include <chrono>
#include <stdexcept>

namespace UnrealProspero::File
{
    namespace
    {
        constexpr auto epochDays = (std::chrono::sys_days{std::chrono::year{1970} / 1 / 1} - std::chrono::sys_days{std::chrono::year{1} / 1 / 1}).count();
        constexpr int64 epochTicks = epochDays * ETimespan::TicksPerDay;
        constexpr int64 nanosecondsPerTick = 1000000000 / ETimespan::TicksPerSecond;
        constexpr int64 ticksPerMicrosecond = ETimespan::TicksPerSecond / 1000000;
        static_assert(1000000000 % ETimespan::TicksPerSecond == 0);
        static_assert(ETimespan::TicksPerSecond % 1000000 == 0);
    }

    FDateTime Timestamp::ToDateTime(const KernelTimespec& time)
    {
        if (time.tv_nsec < 0 || time.tv_nsec >= 1000000000) throw std::invalid_argument("Invalid timestamp nanoseconds");
        const int64 minimumSeconds = -epochTicks / ETimespan::TicksPerSecond;
        const int64 maximumSeconds = (FDateTime::MaxValue().GetTicks() - epochTicks) / ETimespan::TicksPerSecond;
        if (time.tv_sec < minimumSeconds || time.tv_sec > maximumSeconds) throw std::out_of_range("Timestamp is outside the Unreal date range");
        const int64 ticks = epochTicks + time.tv_sec * ETimespan::TicksPerSecond + time.tv_nsec / nanosecondsPerTick;
        if (ticks > FDateTime::MaxValue().GetTicks()) throw std::out_of_range("Timestamp is outside the Unreal date range");
        return FDateTime(ticks);
    }

    KernelTimeval Timestamp::ToTimeval(FDateTime time)
    {
        const int64 ticks = time.GetTicks();
        if (ticks < FDateTime::MinValue().GetTicks() || ticks > FDateTime::MaxValue().GetTicks()) throw std::out_of_range("Invalid Unreal date");
        const int64 sinceEpoch = ticks - epochTicks;
        int64 seconds = sinceEpoch / ETimespan::TicksPerSecond;
        int64 remainder = sinceEpoch % ETimespan::TicksPerSecond;
        if (remainder < 0)
        {
            --seconds;
            remainder += ETimespan::TicksPerSecond;
        }
        return KernelTimeval{seconds, remainder / ticksPerMicrosecond};
    }
}
