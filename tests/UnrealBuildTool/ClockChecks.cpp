#include "UnrealProspero/Kernel/Clock.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <atomic>

bool CheckClocks()
{
    using UnrealProspero::Kernel::Clock;
    const auto start = Clock::MonotonicNanoseconds();
    const auto threadStart = Clock::ThreadNanoseconds();
    const auto processStart = Clock::ProcessNanoseconds();
    std::atomic<std::uint64_t> iterations{0};
    while (Clock::MonotonicNanoseconds() - start < Clock::Frequency / 20) iterations.fetch_add(1, std::memory_order_relaxed);
    const auto threadEnd = Clock::ThreadNanoseconds();
    const auto processEnd = Clock::ProcessNanoseconds();
    if (iterations.load(std::memory_order_relaxed) == 0) return false;
    if (threadEnd <= threadStart || processEnd <= processStart || processEnd < threadEnd) return false;
    const auto sleepStart = Clock::MonotonicNanoseconds();
    UnrealProspero::Kernel::CheckResult(sceKernelUsleep_nid_postfix(20000), "sceKernelUsleep");
    const auto sleepEnd = Clock::MonotonicNanoseconds();
    if (sleepEnd < sleepStart || sleepEnd - sleepStart < Clock::Frequency / 50) return false;
    const auto wall = Clock::UnixTime();
    if (wall.Nanoseconds >= Clock::Frequency) return false;
    KernelTimeval reference{};
    UnrealProspero::Kernel::CheckResult(sceKernelGettimeofday(&reference), "sceKernelGettimeofday");
    if (reference.tv_sec < wall.Seconds || reference.tv_sec - wall.Seconds > 10) return false;
    return true;
}
