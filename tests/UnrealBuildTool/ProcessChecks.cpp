#include "HAL/PlatformProcess.h"
#include "AnyUnreal/Kernel/Clock.h"
#include <limits>
#include <stdexcept>

namespace
{
    template<typename TException>
    bool rejects(float seconds)
    {
        try
        {
            FPlatformProcess::SleepNoStats(seconds);
        }
        catch (const TException&)
        {
            return true;
        }
        return false;
    }
}

bool CheckProcess()
{
    if (!rejects<std::invalid_argument>(-1)) return false;
    if (!rejects<std::invalid_argument>(std::numeric_limits<float>::infinity())) return false;
    if (!rejects<std::invalid_argument>(std::numeric_limits<float>::quiet_NaN())) return false;
    if (!rejects<std::overflow_error>(std::numeric_limits<float>::max())) return false;
    FPlatformProcess::YieldThread();
    FPlatformProcess::SleepNoStats(0);
    using AnyUnreal::Kernel::Clock;
    const auto start = Clock::MonotonicNanoseconds();
    FPlatformProcess::SleepNoStats(0.02f);
    const auto end = Clock::MonotonicNanoseconds();
    if (end < start || end - start < 20000000) return false;
    FPlatformProcess::SleepNoStats(std::numeric_limits<float>::denorm_min());
    FProcHandle empty;
    if (empty.IsValid()) return false;
    FProcHandle handle(42);
    if (!handle.IsValid() || handle.Get() != 42) return false;
    handle.Reset();
    return !handle.IsValid();
}
