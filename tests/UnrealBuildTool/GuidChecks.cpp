#include "UnrealProspero/UnrealProsperoPlatformGuid.h"
#include "UnrealProspero/Kernel/Clock.h"
#include "HAL/PlatformMisc.h"
#include <array>
#include <cstdint>
#include <set>

namespace
{
    std::uint64_t milliseconds()
    {
        const auto time = UnrealProspero::Kernel::Clock::UnixTime();
        return static_cast<std::uint64_t>(time.Seconds) * 1000 + time.Nanoseconds / 1000000;
    }

    bool hasType(const FGuid& guid, uint32 version)
    {
        return guid.IsValid() && ((guid.B >> 12) & 15) == version && (guid.C >> 30) == 2;
    }
}

bool CheckGuids()
{
    std::set<std::array<uint32, 4>> generated;
    for (int index = 0; index != 256; ++index)
    {
        const auto random = FUnrealProsperoPlatformGuid::CreateGuidFromRandomOnly();
        if (!hasType(random, 4)) return false;
        if (!generated.insert({random.A, random.B, random.C, random.D}).second) return false;
        const auto before = milliseconds();
        FGuid timed;
        FPlatformMisc::CreateGuid(timed);
        const auto after = milliseconds();
        if (!hasType(timed, 7)) return false;
        const auto timestamp = (static_cast<std::uint64_t>(timed.A) << 16) | (timed.B >> 16);
        if (timestamp < before || timestamp > after) return false;
        if (!generated.insert({timed.A, timed.B, timed.C, timed.D}).second) return false;
    }
    return true;
}
