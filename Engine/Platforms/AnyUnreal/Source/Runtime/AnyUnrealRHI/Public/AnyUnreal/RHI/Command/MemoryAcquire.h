#ifndef ANYUNREAL_RHI_MEMORY_ACQUIRE_H
#define ANYUNREAL_RHI_MEMORY_ACQUIRE_H

#include "SceTypes.hpp"
#include <cstddef>
#include <span>

namespace AnyUnreal::RHI
{
    struct GpuMemoryRange
    {
        const volatile void* Address;
        std::size_t Size;
    };

    ANYUNREALRHI_API void WriteMemoryAcquire(CommandBuffer& commands, std::span<const GpuMemoryRange> ranges);
}

#endif
