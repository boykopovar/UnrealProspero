#ifndef UNREALPROSPERO_RHI_MEMORY_GPU_MEMORY_BLOCK_H
#define UNREALPROSPERO_RHI_MEMORY_GPU_MEMORY_BLOCK_H

#include "UnrealProspero/RHI/Memory/GpuAddressSpace.h"
#include "UnrealProspero/Kernel/DirectAllocation.h"
#include <memory>

namespace UnrealProspero::RHI
{
    class GpuMemoryBlock final
    {
    public:
        GpuMemoryBlock(std::shared_ptr<GpuAddressSpace> addressSpace, std::size_t size, std::size_t alignment);
        ~GpuMemoryBlock() noexcept(false);
        GpuMemoryBlock(const GpuMemoryBlock&) = delete;
        GpuMemoryBlock& operator=(const GpuMemoryBlock&) = delete;
        void* Data() const;
        std::size_t Size() const;

    private:
        std::shared_ptr<GpuAddressSpace> addressSpace;
        std::unique_ptr<Kernel::DirectAllocation> allocation;
    };
}

#endif
