#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include "prx/libSceAgc/DcbFlow/include/Memory.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WriteMemoryAcquire(CommandBuffer& commands, std::span<const GpuMemoryRange> ranges)
    {
        for (const auto& range : ranges)
        {
            if (!range.Address || range.Size == 0) throw std::invalid_argument("GPU memory range must not be empty");
            const auto address = reinterpret_cast<std::uintptr_t>(range.Address);
            if (range.Size - 1 > std::numeric_limits<std::uintptr_t>::max() - address) throw std::invalid_argument("GPU memory range overflows its address");
        }
        if (ranges.empty()) return;
        constexpr std::uint32_t globalCacheControl = 0x4381;
        const auto& range = ranges.front();
        sceAgcDcbAcquireMem(&commands, 1, 0, globalCacheControl, range.Address, range.Size, 40);
    }
}
