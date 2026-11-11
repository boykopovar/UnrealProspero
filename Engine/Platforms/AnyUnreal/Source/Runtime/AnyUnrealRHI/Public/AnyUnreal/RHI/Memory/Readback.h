#ifndef ANYUNREAL_RHI_READBACK_H
#define ANYUNREAL_RHI_READBACK_H

#include "AnyUnreal/RHI/Command/Completion.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API Readback final
    {
    public:
        Readback(std::size_t size, std::uint32_t addressHigh);
        void Record(CommandBuffer& commands, std::span<const std::byte> source);
        std::span<std::byte> GetRange(std::size_t offset, std::size_t size) const;
        std::size_t Size() const;

    private:
        BufferAllocation storage;
        Completion completion;
        bool recorded = false;
    };
}

#endif
