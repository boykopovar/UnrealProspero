#ifndef UNREALPROSPERO_KERNEL_DIRECT_ALLOCATION_H
#define UNREALPROSPERO_KERNEL_DIRECT_ALLOCATION_H

#include "UnrealProspero/Kernel/MemoryConstants.h"
#include <cstddef>
#include <cstdint>

namespace UnrealProspero::Kernel
{
    enum class DirectMemoryAccess
    {
        Cpu,
        CpuGpu
    };

    enum class DirectMemoryPlacement
    {
        Hint,
        Reserved
    };

    class DirectAllocation
    {
    public:
        static constexpr std::size_t PageSize = MemoryPageSize;
        explicit DirectAllocation(std::size_t size, std::size_t alignment = PageSize, DirectMemoryAccess access = DirectMemoryAccess::Cpu, void* addressHint = nullptr, DirectMemoryPlacement placement = DirectMemoryPlacement::Hint);
        ~DirectAllocation() noexcept(false);
        DirectAllocation(const DirectAllocation&) = delete;
        DirectAllocation& operator=(const DirectAllocation&) = delete;
        void* Data() const;
        std::size_t Size() const;
        void ProtectCpu(std::size_t offset, std::size_t length, bool readable, bool writable);

    private:
        void releaseMapping();
        void* address = nullptr;
        std::int64_t physicalAddress = 0;
        std::size_t size;
        int gpuProtection;
        DirectMemoryPlacement placement;
    };
}

#endif
