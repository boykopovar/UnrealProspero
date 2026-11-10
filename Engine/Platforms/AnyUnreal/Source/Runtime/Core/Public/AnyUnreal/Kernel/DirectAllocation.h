#ifndef ANYUNREAL_KERNEL_DIRECT_ALLOCATION_H
#define ANYUNREAL_KERNEL_DIRECT_ALLOCATION_H

#include <cstddef>
#include <cstdint>

namespace AnyUnreal::Kernel
{
    enum class DirectMemoryAccess
    {
        Cpu,
        CpuGpu
    };

    class DirectAllocation
    {
    public:
        static constexpr std::size_t PageSize = 16384;
        explicit DirectAllocation(std::size_t size, std::size_t alignment = PageSize, DirectMemoryAccess access = DirectMemoryAccess::Cpu);
        ~DirectAllocation() noexcept(false);
        DirectAllocation(const DirectAllocation&) = delete;
        DirectAllocation& operator=(const DirectAllocation&) = delete;
        void* Data() const;
        std::size_t Size() const;
        void ProtectCpu(std::size_t offset, std::size_t length, bool readable, bool writable);

    private:
        void* address = nullptr;
        std::int64_t physicalAddress = 0;
        std::size_t size;
        int gpuProtection;
    };
}

#endif
