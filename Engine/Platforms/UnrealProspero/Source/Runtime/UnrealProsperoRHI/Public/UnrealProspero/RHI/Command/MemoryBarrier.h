#ifndef UNREALPROSPERO_RHI_MEMORY_BARRIER_H
#define UNREALPROSPERO_RHI_MEMORY_BARRIER_H

#include "UnrealProspero/RHI/Command/MemoryAcquire.h"
#include "UnrealProspero/RHI/Memory/BufferAllocation.h"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API MemoryBarrier final
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
