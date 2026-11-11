#ifndef UNREALPROSPERO_RHI_UPLOAD_H
#define UNREALPROSPERO_RHI_UPLOAD_H

#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include "UnrealProspero/RHI/Command/MemoryBarrier.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API Upload final
    {
    public:
        Upload(std::span<const std::byte> source, std::uint32_t addressHigh);
        void Record(CommandBuffer& commands, std::span<std::byte> destination);

    private:
        BufferAllocation storage;
        MemoryBarrier barrier;
        bool recorded = false;
    };
}

#endif
