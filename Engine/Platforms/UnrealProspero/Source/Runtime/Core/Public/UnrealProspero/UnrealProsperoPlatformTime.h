#ifndef UNREALPROSPERO_PLATFORM_TIME_H
#define UNREALPROSPERO_PLATFORM_TIME_H

#include "GenericPlatform/GenericPlatformTime.h"
#include "UnrealProspero/Kernel/Clock.h"
#include "UnrealProspero/Time/GameThreadCpuTicker.h"
#include <cmath>
#include <stdexcept>

struct FUnrealProsperoPlatformTime : FGenericPlatformTime
{
    static double InitTiming()
    {
        return Seconds();
    }

    static double Seconds()
    {
        return ToSeconds64(Cycles64());
    }

    static uint32 Cycles()
    {
        return static_cast<uint32>(Cycles64() / 1000);
    }

    static uint64 Cycles64()
    {
        return UnrealProspero::Kernel::Clock::MonotonicNanoseconds();
    }

    static constexpr double GetSecondsPerCycle()
    {
        return 1e-6;
    }

    static constexpr double GetSecondsPerCycle64()
    {
        return 1.0 / UnrealProspero::Kernel::Clock::Frequency;
    }

    static constexpr float ToMilliseconds(uint32 cycles)
    {
        return static_cast<float>(static_cast<double>(cycles) * 1e-3);
    }

    static constexpr float ToSeconds(uint32 cycles)
    {
        return static_cast<float>(static_cast<double>(cycles) * GetSecondsPerCycle());
    }

    static constexpr double ToMilliseconds64(uint64 cycles)
    {
        return ToSeconds64(cycles) * 1000;
    }

    static constexpr double ToSeconds64(uint64 cycles)
    {
        return static_cast<double>(cycles) * GetSecondsPerCycle64();
    }

    static uint64 SecondsToCycles64(double seconds)
    {
        if (!std::isfinite(seconds) || seconds < 0) throw std::invalid_argument("Invalid elapsed time");
        const long double cycles = static_cast<long double>(seconds) * UnrealProspero::Kernel::Clock::Frequency;
        if (cycles >= 18446744073709551616.0L) throw std::overflow_error("Elapsed time exceeds the cycle counter range");
        return static_cast<uint64>(cycles);
    }

    static CORE_API void SystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static CORE_API void UtcTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);

    static CORE_API bool UpdateCPUTime(float deltaTime);
    static CORE_API bool UpdateThreadCPUTime(float deltaTime = 0);

    static void AutoUpdateGameThreadCPUTime(double updateInterval)
    {
        UnrealProspero::Time::GameThreadCpuTicker::Enable(updateInterval);
    }

    static CORE_API FCPUTime GetCPUTime();
    static CORE_API FCPUTime GetThreadCPUTime();
    static CORE_API double GetLastIntervalCPUTimeInSeconds();
    static CORE_API double GetLastIntervalThreadCPUTimeInSeconds();
};

using FPlatformTime = FUnrealProsperoPlatformTime;

#endif
