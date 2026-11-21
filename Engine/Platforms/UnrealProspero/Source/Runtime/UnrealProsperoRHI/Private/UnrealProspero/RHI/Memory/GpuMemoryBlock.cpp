#include "UnrealProspero/RHI/Memory/GpuMemoryBlock.h"
#include <stdexcept>
#include <utility>

namespace UnrealProspero::RHI
{
    GpuMemoryBlock::GpuMemoryBlock(std::shared_ptr<GpuAddressSpace> addressSpace, std::size_t size, std::size_t alignment) : addressSpace(std::move(addressSpace))
    {
        if (!this->addressSpace) throw std::invalid_argument("GPU memory requires an address space");
        auto* address = this->addressSpace->Allocate(size, alignment);
        try
        {
            allocation = std::make_unique<Kernel::DirectAllocation>(size, alignment, Kernel::DirectMemoryAccess::CpuGpu, address, Kernel::DirectMemoryPlacement::Reserved);
        }
        catch (...)
        {
            this->addressSpace->Release(address, size);
            throw;
        }
    }

    GpuMemoryBlock::~GpuMemoryBlock() noexcept(false)
    {
        auto* address = allocation->Data();
        const auto size = allocation->Size();
        allocation.reset();
        addressSpace->Release(address, size);
    }

    void* GpuMemoryBlock::Data() const { return allocation->Data(); }
    std::size_t GpuMemoryBlock::Size() const { return allocation->Size(); }
}
