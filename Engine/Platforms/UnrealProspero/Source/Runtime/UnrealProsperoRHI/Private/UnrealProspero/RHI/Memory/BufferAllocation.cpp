#include "UnrealProspero/RHI/Memory/BufferAllocation.h"
#include <limits>
#include <cstring>
#include <stdexcept>
#include <atomic>
#if UNREALPROSPERO_WITH_UE_TRACKING
#include "HAL/LowLevelMemTracker.h"
#include "ProfilingDebugging/MemoryTrace.h"
#endif

namespace UnrealProspero::RHI
{
    namespace
    {
        std::atomic<std::uint64_t> textureAllocationBytes{0};
        std::atomic<std::uint64_t> streamingTextureAllocationBytes{0};
    }

    std::size_t BufferAllocation::CalculateAllocationSize(std::size_t size)
    {
        constexpr auto pageSize = Kernel::DirectAllocation::PageSize;
        if (size == 0 || size > std::numeric_limits<std::uint32_t>::max() - pageSize + 1) throw std::invalid_argument("GPU buffer exceeds the shader address aperture");
        return (size + pageSize - 1) & ~(pageSize - 1);
    }

    BufferAllocation::BufferAllocation(std::size_t size, std::uint32_t addressHigh, std::size_t alignment, AllocationUsage usage) : allocation(CalculateAllocationSize(size), addressHigh, alignment, (size + 255) & ~std::size_t{255}), size(size), usage(usage)
    {
        if (usage != AllocationUsage::Buffer && usage != AllocationUsage::Texture && usage != AllocationUsage::StreamingTexture) throw std::invalid_argument("Invalid GPU allocation usage");
        const auto address = reinterpret_cast<std::uintptr_t>(allocation.Data());
        if ((address >> 32) != addressHigh || ((address + allocation.Size() - 1) >> 32) != addressHigh) throw std::runtime_error("Kernel mapped GPU buffer outside the shader address aperture");
#if UNREALPROSPERO_WITH_UE_TRACKING
        if (alignment > std::numeric_limits<uint32>::max()) throw std::overflow_error("GPU allocation alignment exceeds memory trace range");
        LLM_IF_ENABLED(FLowLevelMemTracker::Get().OnLowLevelAlloc(ELLMTracker::Default, allocation.Data(), allocation.Size(), ELLMTag::Untagged, ELLMAllocType::System));
        MemoryTrace_Alloc(address, allocation.Size(), static_cast<uint32>(alignment));
#endif
        if (usage == AllocationUsage::Texture) textureAllocationBytes.fetch_add(allocation.Size(), std::memory_order_relaxed);
        if (usage == AllocationUsage::StreamingTexture) streamingTextureAllocationBytes.fetch_add(allocation.Size(), std::memory_order_relaxed);
    }

    BufferAllocation::~BufferAllocation() noexcept(false)
    {
#if UNREALPROSPERO_WITH_UE_TRACKING
        MemoryTrace_Free(reinterpret_cast<std::uintptr_t>(allocation.Data()));
        LLM_IF_ENABLED(FLowLevelMemTracker::Get().OnLowLevelFree(ELLMTracker::Default, allocation.Data(), ELLMAllocType::System));
#endif
        if (usage == AllocationUsage::Texture) textureAllocationBytes.fetch_sub(allocation.Size(), std::memory_order_relaxed);
        if (usage == AllocationUsage::StreamingTexture) streamingTextureAllocationBytes.fetch_sub(allocation.Size(), std::memory_order_relaxed);
    }

#if UNREALPROSPERO_WITH_UE_TRACKING
    void BufferAllocation::UpdateAllocationTags() const
    {
        LLM_IF_ENABLED(FLowLevelMemTracker::Get().OnLowLevelFree(ELLMTracker::Default, allocation.Data(), ELLMAllocType::System));
        LLM_IF_ENABLED(FLowLevelMemTracker::Get().OnLowLevelAlloc(ELLMTracker::Default, allocation.Data(), allocation.Size(), ELLMTag::Untagged, ELLMAllocType::System));
        MemoryTrace_UpdateAlloc(reinterpret_cast<std::uintptr_t>(allocation.Data()));
    }
#endif

    std::uint64_t BufferAllocation::GetTextureAllocationBytes()
    {
        return GetNonStreamingTextureAllocationBytes() + streamingTextureAllocationBytes.load(std::memory_order_relaxed);
    }

    std::uint64_t BufferAllocation::GetNonStreamingTextureAllocationBytes()
    {
        return textureAllocationBytes.load(std::memory_order_relaxed);
    }

    std::span<std::byte> BufferAllocation::GetRange(std::size_t offset, std::size_t length) const
    {
        if (length == 0 || offset > size || length > size - offset) throw std::out_of_range("GPU buffer range exceeds its allocation");
        return {static_cast<std::byte*>(allocation.Data()) + offset, length};
    }

    void BufferAllocation::Upload(std::size_t offset, std::span<const std::byte> source)
    {
        const auto destination = GetRange(offset, source.size());
        std::memmove(destination.data(), source.data(), source.size());
    }

    void* BufferAllocation::Data() const { return allocation.Data(); }
    std::size_t BufferAllocation::Size() const { return size; }
    std::size_t BufferAllocation::AllocationSize() const { return allocation.Size(); }
}
