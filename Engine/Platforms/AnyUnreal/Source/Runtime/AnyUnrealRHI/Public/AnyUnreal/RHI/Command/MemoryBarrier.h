#ifndef ANYUNREAL_RHI_MEMORY_BARRIER_H
#define ANYUNREAL_RHI_MEMORY_BARRIER_H

#include "AnyUnreal/RHI/Command/MemoryAcquire.h"
#include "AnyUnreal/RHI/Memory/BufferAllocation.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API MemoryBarrier final
    {
    public:
        explicit MemoryBarrier(std::uint32_t addressHigh);
        void Release(CommandBuffer& commands);
        void Acquire(CommandBuffer& commands, std::span<const GpuMemoryRange> ranges);

    private:
        BufferAllocation completion;
        bool released = false;
        bool acquired = false;
    };
}

#endif
