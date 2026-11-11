#include "UnrealProspero/Kernel/Clock.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Kernel
{
    namespace
    {
        KernelTimespec readClock(KernelClockid clockId)
        {
            KernelTimespec time{};
            CheckResult(sceKernelClockGettime(clockId, &time), "sceKernelClockGettime");
            if (time.tv_nsec < 0 || time.tv_nsec >= static_cast<std::int64_t>(Clock::Frequency)) throw std::runtime_error("Invalid kernel clock nanoseconds");
            return time;
        }

        std::uint64_t readNanoseconds(KernelClockid clockId)
        {
            const KernelTimespec time = readClock(clockId);
            if (time.tv_sec < 0) throw std::runtime_error("Negative kernel elapsed time");
            const auto seconds = static_cast<std::uint64_t>(time.tv_sec);
            const auto nanoseconds = static_cast<std::uint64_t>(time.tv_nsec);
            if (seconds > (std::numeric_limits<std::uint64_t>::max() - nanoseconds) / Clock::Frequency) throw std::overflow_error("Kernel elapsed time exceeds nanosecond counter range");
            return seconds * Clock::Frequency + nanoseconds;
        }
    }

    std::uint64_t Clock::MonotonicNanoseconds()
    {
        constexpr KernelClockid monotonicClock = 4;
        return readNanoseconds(monotonicClock);
    }

    std::uint64_t Clock::ProcessNanoseconds()
    {
        constexpr KernelClockid processClock = 15;
        return readNanoseconds(processClock);
    }

    std::uint64_t Clock::ThreadNanoseconds()
    {
        constexpr KernelClockid threadClock = 14;
        return readNanoseconds(threadClock);
    }

    WallTime Clock::UnixTime()
    {
        constexpr KernelClockid realtimeClock = 0;
        const KernelTimespec time = readClock(realtimeClock);
        return {time.tv_sec, static_cast<std::uint32_t>(time.tv_nsec)};
    }
}
