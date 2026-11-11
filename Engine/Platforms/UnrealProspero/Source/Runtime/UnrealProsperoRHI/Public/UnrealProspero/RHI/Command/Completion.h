#ifndef UNREALPROSPERO_RHI_COMPLETION_H
#define UNREALPROSPERO_RHI_COMPLETION_H

#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API Completion final
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
