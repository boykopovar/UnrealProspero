#include "UnrealProspero/UnrealProsperoPlatformGuid.h"
#include "UnrealProspero/UnrealProsperoPlatformMisc.h"
#include "UnrealProspero/Kernel/Clock.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

FGuid FUnrealProsperoPlatformGuid::CreateGuid()
{
    return CreateGuidFromTimeAndRandom();
}

FGuid FUnrealProsperoPlatformGuid::CreateGuidFromTimeAndRandom()
{
    const auto time = UnrealProspero::Kernel::Clock::UnixTime();
    constexpr std::uint64_t maximumMilliseconds = (std::uint64_t{1} << 48) - 1;
    if (time.Seconds < 0 || static_cast<std::uint64_t>(time.Seconds) > maximumMilliseconds / 1000) throw std::out_of_range("UUID timestamp is outside the supported range");
    const auto milliseconds = static_cast<std::uint64_t>(time.Seconds) * 1000 + time.Nanoseconds / 1000000;
    if (milliseconds > maximumMilliseconds) throw std::out_of_range("UUID timestamp is outside the supported range");
    FGuid guid = CreateGuidFromRandomOnly();
    guid.A = static_cast<uint32>(milliseconds >> 16);
    guid.B = (static_cast<uint32>(milliseconds) << 16) | 0x7000u | (guid.B & 0xfffu);
    return guid;
}

FGuid FUnrealProsperoPlatformGuid::CreateGuidFromRandomOnly()
{
    std::array<std::uint8_t, 16> bytes{};
    UnrealProspero::Kernel::CheckResult(sceKernelCreateUuidV4(bytes.data()), "sceKernelCreateUuidV4");
    const auto readWord = [&bytes](std::size_t offset)
    {
        return (static_cast<uint32>(bytes[offset]) << 24) | (static_cast<uint32>(bytes[offset + 1]) << 16) | (static_cast<uint32>(bytes[offset + 2]) << 8) | bytes[offset + 3];
    };
    return FGuid(readWord(0), readWord(4), readWord(8), readWord(12));
}

void FUnrealProsperoPlatformMisc::CreateGuid(FGuid& result)
{
    result = FUnrealProsperoPlatformGuid::CreateGuid();
}
