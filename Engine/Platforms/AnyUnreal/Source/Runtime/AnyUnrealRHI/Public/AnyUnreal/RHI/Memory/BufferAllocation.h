#ifndef ANYUNREAL_RHI_MEMORY_BUFFER_ALLOCATION_H
#define ANYUNREAL_RHI_MEMORY_BUFFER_ALLOCATION_H

#include "AnyUnreal/Kernel/DirectAllocation.h"
#include <span>

namespace AnyUnreal::RHI
{
    enum class AllocationUsage
    {
        Buffer,
        Texture
    };

    class ANYUNREALRHI_API BufferAllocation final
    {
    public:
        static std::uint64_t GetTextureAllocationBytes();
        static std::size_t CalculateAllocationSize(std::size_t size);
        BufferAllocation(std::size_t size, std::uint32_t addressHigh, std::size_t alignment = Kernel::DirectAllocation::PageSize, AllocationUsage usage = AllocationUsage::Buffer);
        ~BufferAllocation() noexcept(false);
        BufferAllocation(const BufferAllocation&) = delete;
        BufferAllocation& operator=(const BufferAllocation&) = delete;
        void* Data() const;
        std::size_t Size() const;
        std::size_t AllocationSize() const;
        std::span<std::byte> GetRange(std::size_t offset, std::size_t length) const;
        void Upload(std::size_t offset, std::span<const std::byte> source);
#if ANYUNREAL_WITH_UE_TRACKING
        void UpdateAllocationTags() const;
#endif

    private:
        Kernel::DirectAllocation allocation;
        std::size_t size;
        AllocationUsage usage;
    };
}

#endif
