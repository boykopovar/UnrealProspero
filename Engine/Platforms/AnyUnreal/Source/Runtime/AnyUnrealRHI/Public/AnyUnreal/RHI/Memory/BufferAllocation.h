#ifndef ANYUNREAL_RHI_MEMORY_BUFFER_ALLOCATION_H
#define ANYUNREAL_RHI_MEMORY_BUFFER_ALLOCATION_H

#include "AnyUnreal/Kernel/DirectAllocation.h"
#include <span>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API BufferAllocation final
    {
    public:
        static std::size_t CalculateAllocationSize(std::size_t size);
        BufferAllocation(std::size_t size, std::uint32_t addressHigh);
        BufferAllocation(const BufferAllocation&) = delete;
        BufferAllocation& operator=(const BufferAllocation&) = delete;
        void* Data() const;
        std::size_t Size() const;
        std::size_t AllocationSize() const;
        std::span<std::byte> GetRange(std::size_t offset, std::size_t length) const;
        void Upload(std::size_t offset, std::span<const std::byte> source);

    private:
        Kernel::DirectAllocation allocation;
        std::size_t size;
    };
}

#endif
