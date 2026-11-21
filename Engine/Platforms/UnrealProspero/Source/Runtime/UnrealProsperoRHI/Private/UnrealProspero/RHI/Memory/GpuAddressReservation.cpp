#include "UnrealProspero/RHI/Memory/GpuAddressReservation.h"
#include "UnrealProspero/Kernel/MemoryConstants.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    GpuAddressReservation::GpuAddressReservation(void* address, std::size_t size) : address(address), size(size)
    {
        const auto pointer = reinterpret_cast<std::uintptr_t>(address);
        if (!address || pointer % Kernel::MemoryPageSize != 0 || size == 0 || size % Kernel::MemoryPageSize != 0 || size > std::numeric_limits<std::uintptr_t>::max() - pointer) throw std::invalid_argument("Invalid GPU virtual address reservation");
        Kernel::CheckResult(sceKernelReserveVirtualRange(&this->address, size, 0x90, Kernel::MemoryPageSize), "sceKernelReserveVirtualRange");
        if (this->address != address)
        {
            Kernel::CheckResult(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(this->address), size), "sceKernelMunmap");
            throw std::runtime_error("Fixed GPU address reservation moved");
        }
    }

    GpuAddressReservation::~GpuAddressReservation() noexcept(false)
    {
        Kernel::CheckResult(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(address), size), "sceKernelMunmap");
    }
}
