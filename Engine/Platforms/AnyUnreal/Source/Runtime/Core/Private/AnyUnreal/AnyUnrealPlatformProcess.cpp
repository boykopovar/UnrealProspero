#include "AnyUnreal/AnyUnrealPlatformProcess.h"
#include "AnyUnreal/Kernel/Clock.h"
#include "AnyUnreal/Kernel/Event.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <cmath>
#include <stdexcept>

void FAnyUnrealPlatformProcess::SleepNoStats(float seconds)
{
    if (!std::isfinite(seconds) || seconds < 0) throw std::invalid_argument("Invalid sleep duration");
    if (seconds == 0)
    {
        YieldThread();
        return;
    }
    using AnyUnreal::Kernel::Clock;
    const long double nanoseconds = std::ceil(static_cast<long double>(seconds) * Clock::Frequency);
    if (nanoseconds >= 18446744073709551616.0L) throw std::overflow_error("Sleep duration exceeds the clock range");
    const auto duration = static_cast<std::uint64_t>(nanoseconds);
    const auto start = Clock::MonotonicNanoseconds();
    std::uint64_t remaining = duration;
    for (;;)
    {
        KernelTimespec request{};
        request.tv_sec = static_cast<decltype(request.tv_sec)>(remaining / Clock::Frequency);
        request.tv_nsec = static_cast<decltype(request.tv_nsec)>(remaining % Clock::Frequency);
        const int result = sceKernelNanosleep(&request, nullptr);
        if (result != 0 && result != SCE_KERNEL_ERROR_EINTR) AnyUnreal::Kernel::CheckResult(result, "sceKernelNanosleep");
        const auto now = Clock::MonotonicNanoseconds();
        if (now < start) throw std::runtime_error("Monotonic clock moved backwards");
        const auto elapsed = now - start;
        if (elapsed >= duration) return;
        remaining = duration - elapsed;
    }
}

void FAnyUnrealPlatformProcess::YieldThread()
{
    scePthreadYield();
}

void FAnyUnrealPlatformProcess::SleepInfinite()
{
    AnyUnreal::Kernel::Event event(false);
    event.Wait();
    throw std::runtime_error("Infinite sleep unexpectedly completed");
}
