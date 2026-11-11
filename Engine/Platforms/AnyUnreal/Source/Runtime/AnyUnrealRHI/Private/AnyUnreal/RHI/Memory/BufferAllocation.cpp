#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include <limits>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <atomic>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::atomic<std::uint64_t> textureAllocationBytes{0};
        void* addressHint(std::uint32_t high, std::size_t alignment)
        {
            if (high > 0x7fff) throw std::invalid_argument("GPU buffer aperture exceeds user address space");
            const auto base = static_cast<std::uint64_t>(high) << 32;
            return reinterpret_cast<void*>(static_cast<std::uintptr_t>(base == 0 ? std::max<std::uint64_t>(65536, alignment) : base));
        }
    }

    std::size_t BufferAllocation::CalculateAllocationSize(std::size_t size)
    {
        constexpr auto pageSize = Kernel::DirectAllocation::PageSize;
        if (size == 0 || size > std::numeric_limits<std::uint32_t>::max() - pageSize + 1) throw std::invalid_argument("GPU buffer exceeds the shader address aperture");
        return (size + pageSize - 1) & ~(pageSize - 1);
    }

    BufferAllocation::BufferAllocation(std::size_t size, std::uint32_t addressHigh, std::size_t alignment, AllocationUsage usage) : allocation(CalculateAllocationSize(size), alignment, Kernel::DirectMemoryAccess::CpuGpu, addressHint(addressHigh, alignment)), size(size), usage(usage)
    {
        if (usage != AllocationUsage::Buffer && usage != AllocationUsage::Texture) throw std::invalid_argument("Invalid GPU allocation usage");
        const auto address = reinterpret_cast<std::uintptr_t>(allocation.Data());
        if ((address >> 32) != addressHigh || ((address + allocation.Size() - 1) >> 32) != addressHigh) throw std::runtime_error("Kernel mapped GPU buffer outside the shader address aperture");
        if (usage == AllocationUsage::Texture) textureAllocationBytes.fetch_add(allocation.Size(), std::memory_order_relaxed);
    }

    BufferAllocation::~BufferAllocation() noexcept(false)
    {
        if (usage == AllocationUsage::Texture) textureAllocationBytes.fetch_sub(allocation.Size(), std::memory_order_relaxed);
    }

    std::uint64_t BufferAllocation::GetTextureAllocationBytes()
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
