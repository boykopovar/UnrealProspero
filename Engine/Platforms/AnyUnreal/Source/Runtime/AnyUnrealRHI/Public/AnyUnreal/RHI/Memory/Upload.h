#ifndef ANYUNREAL_RHI_UPLOAD_H
#define ANYUNREAL_RHI_UPLOAD_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "AnyUnreal/RHI/Command/MemoryBarrier.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API Upload final
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
