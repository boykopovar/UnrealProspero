#ifndef ANYUNREAL_RHI_COMPLETION_H
#define ANYUNREAL_RHI_COMPLETION_H

#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API Completion final
    {
    public:
        explicit Completion(std::uint32_t addressHigh);
        void Write(CommandBuffer& commands);
        bool IsComplete() const;
        void Wait(std::uint64_t timeoutMicroseconds) const;

    private:
        BufferAllocation storage;
        bool written = false;
    };
}

#endif
