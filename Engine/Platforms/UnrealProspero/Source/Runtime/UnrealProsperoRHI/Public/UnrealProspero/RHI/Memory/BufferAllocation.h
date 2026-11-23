#ifndef UNREALPROSPERO_RHI_MEMORY_BUFFER_ALLOCATION_H
#define UNREALPROSPERO_RHI_MEMORY_BUFFER_ALLOCATION_H

#include "UnrealProspero/RHI/Memory/ReusableAllocation.h"
#include <span>

namespace UnrealProspero::RHI
{
    enum class AllocationUsage
    {
        Buffer,
        Texture,
        StreamingTexture
    };

    class UNREALPROSPERORHI_API BufferAllocation final
    {
    public:
        static std::uint64_t GetTextureAllocationBytes();
        static std::uint64_t GetNonStreamingTextureAllocationBytes();
        static std::size_t CalculateAllocationSize(std::size_t size);
        BufferAllocation(std::size_t size, std::uint32_t addressHigh, std::size_t alignment = Kernel::DirectAllocation::PageSize, AllocationUsage usage = AllocationUsage::Buffer, AllocationInitialization initialization = AllocationInitialization::Zeroed);
        ~BufferAllocation() noexcept(false);
        BufferAllocation(const BufferAllocation&) = delete;
        BufferAllocation& operator=(const BufferAllocation&) = delete;
        void* Data() const;
        std::size_t Size() const;
        std::size_t AllocationSize() const;
        std::span<std::byte> GetRange(std::size_t offset, std::size_t length) const;
        void Upload(std::size_t offset, std::span<const std::byte> source);
#if UNREALPROSPERO_WITH_UE_TRACKING
        void UpdateAllocationTags() const;
#endif

    private:
        ReusableAllocation allocation;
        std::size_t size;
        AllocationUsage usage;
    };
}

#endif
