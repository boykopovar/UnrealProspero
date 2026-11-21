#ifndef UNREALPROSPERO_RHI_MEMORY_GPU_ADDRESS_SPACE_H
#define UNREALPROSPERO_RHI_MEMORY_GPU_ADDRESS_SPACE_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>

namespace UnrealProspero::RHI
{
    class GpuAddressSpace final
    {
    public:
        explicit GpuAddressSpace(std::uint32_t addressHigh);
        ~GpuAddressSpace() noexcept(false);
        GpuAddressSpace(const GpuAddressSpace&) = delete;
        GpuAddressSpace& operator=(const GpuAddressSpace&) = delete;
        void* Allocate(std::size_t size, std::size_t alignment);
        void Release(void* address, std::size_t size);

    private:
        std::uintptr_t base;
        std::size_t size;
        std::map<std::size_t, std::size_t> freeRanges;
        std::map<std::size_t, std::size_t> allocations;
        std::mutex mutex;
    };
}

#endif
