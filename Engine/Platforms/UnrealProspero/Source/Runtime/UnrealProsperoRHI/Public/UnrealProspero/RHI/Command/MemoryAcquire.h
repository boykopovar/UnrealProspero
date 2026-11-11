#ifndef UNREALPROSPERO_RHI_MEMORY_ACQUIRE_H
#define UNREALPROSPERO_RHI_MEMORY_ACQUIRE_H

#include "SceTypes.hpp"
#include <cstddef>
#include <span>

namespace UnrealProspero::RHI
{
    struct GpuMemoryRange
    {
        const volatile void* Address;
        std::size_t Size;
    };

    UNREALPROSPERORHI_API void WriteMemoryAcquire(CommandBuffer& commands, std::span<const GpuMemoryRange> ranges);
}

#endif
