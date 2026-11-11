#include "AnyUnreal/RHI/Memory/BufferAllocation.h"
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::size_t roundedSize(std::size_t size)
        {
            constexpr auto pageSize = Kernel::DirectAllocation::PageSize;
            if (size == 0 || size > std::numeric_limits<std::uint32_t>::max() - pageSize + 1) throw std::invalid_argument("GPU buffer exceeds the shader address aperture");
            return (size + pageSize - 1) & ~(pageSize - 1);
        }

        void* addressHint(std::uint32_t high)
        {
            if (high > 0x7fff) throw std::invalid_argument("GPU buffer aperture exceeds user address space");
            const auto base = static_cast<std::uint64_t>(high) << 32;
            return reinterpret_cast<void*>(static_cast<std::uintptr_t>(base == 0 ? 65536 : base));
        }
    }

    BufferAllocation::BufferAllocation(std::size_t size, std::uint32_t addressHigh) : allocation(roundedSize(size), Kernel::DirectAllocation::PageSize, Kernel::DirectMemoryAccess::CpuGpu, addressHint(addressHigh)), size(size)
    {
        const auto address = reinterpret_cast<std::uintptr_t>(allocation.Data());
        if ((address >> 32) != addressHigh || ((address + allocation.Size() - 1) >> 32) != addressHigh) throw std::runtime_error("Kernel mapped GPU buffer outside the shader address aperture");
    }

    void* BufferAllocation::Data() const { return allocation.Data(); }
    std::size_t BufferAllocation::Size() const { return size; }
}
