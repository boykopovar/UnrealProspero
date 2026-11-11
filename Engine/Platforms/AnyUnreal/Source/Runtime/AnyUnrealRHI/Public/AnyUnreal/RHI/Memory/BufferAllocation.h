#ifndef ANYUNREAL_RHI_MEMORY_BUFFER_ALLOCATION_H
#define ANYUNREAL_RHI_MEMORY_BUFFER_ALLOCATION_H

#include "AnyUnreal/Kernel/DirectAllocation.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API BufferAllocation final
    {
    public:
        BufferAllocation(std::size_t size, std::uint32_t addressHigh);
        BufferAllocation(const BufferAllocation&) = delete;
        BufferAllocation& operator=(const BufferAllocation&) = delete;
        void* Data() const;
        std::size_t Size() const;

    private:
        Kernel::DirectAllocation allocation;
        std::size_t size;
    };
}

#endif
