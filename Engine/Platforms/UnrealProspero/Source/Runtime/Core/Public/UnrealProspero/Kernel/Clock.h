#ifndef UNREALPROSPERO_KERNEL_CLOCK_H
#define UNREALPROSPERO_KERNEL_CLOCK_H

#include <cstdint>

namespace UnrealProspero::Kernel
{
    struct WallTime
    {
        std::int64_t Seconds;
        std::uint32_t Nanoseconds;
    };

    class Clock
    {
    public:
        static constexpr std::uint64_t Frequency = 1000000000;
        static std::uint64_t MonotonicNanoseconds();
        static std::uint64_t ProcessNanoseconds();
        static std::uint64_t ThreadNanoseconds();
        static WallTime UnixTime();
    };
}

#endif
