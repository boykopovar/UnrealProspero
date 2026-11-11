#include "UnrealProspero/Kernel/DirectAllocation.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Kernel
{
    DirectAllocation::DirectAllocation(std::size_t size, std::size_t alignment, DirectMemoryAccess access, void* addressHint) : address(addressHint), size(size), gpuProtection(access == DirectMemoryAccess::CpuGpu ? 0x30 : 0)
    {
        if (access != DirectMemoryAccess::Cpu && access != DirectMemoryAccess::CpuGpu) throw std::invalid_argument("Invalid direct memory access mode");
        if (size == 0 || size % PageSize != 0) throw std::invalid_argument("Direct allocation size must be a positive multiple of the page size");
        if (alignment < PageSize || !std::has_single_bit(alignment)) throw std::invalid_argument("Invalid direct allocation alignment");
        if (reinterpret_cast<std::uintptr_t>(addressHint) % alignment != 0) throw std::invalid_argument("Unaligned direct memory address hint");
        const auto capacity = sceKernelGetDirectMemorySize();
        if (capacity == 0 || capacity > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) throw std::runtime_error("Invalid direct memory capacity");
        constexpr int directMemoryType = 12;
        CheckResult(sceKernelAllocateDirectMemory(0, static_cast<std::int64_t>(capacity), size, alignment, directMemoryType, &physicalAddress), "sceKernelAllocateDirectMemory");
        try
        {
            CheckResult(sceKernelMapDirectMemory(&address, size, 3 | gpuProtection, 0, physicalAddress, alignment), "sceKernelMapDirectMemory");
        }
        catch (...)
        {
            CheckResult(sceKernelReleaseDirectMemory(physicalAddress, size), "sceKernelReleaseDirectMemory");
            throw;
        }
        if (address == nullptr || reinterpret_cast<std::uintptr_t>(address) % alignment != 0)
        {
            CheckResult(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(address), size), "sceKernelMunmap");
            CheckResult(sceKernelReleaseDirectMemory(physicalAddress, size), "sceKernelReleaseDirectMemory");
            throw std::runtime_error("Kernel returned an invalid direct memory address");
        }
        std::memset(address, 0, size);
    }

    DirectAllocation::~DirectAllocation() noexcept(false)
    {
        CheckResult(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(address), size), "sceKernelMunmap");
        CheckResult(sceKernelReleaseDirectMemory(physicalAddress, size), "sceKernelReleaseDirectMemory");
    }

    void* DirectAllocation::Data() const
    {
        return address;
    }

    std::size_t DirectAllocation::Size() const
    {
        return size;
    }

    void DirectAllocation::ProtectCpu(std::size_t offset, std::size_t length, bool readable, bool writable)
    {
        if (offset % PageSize != 0 || length == 0 || length % PageSize != 0 || offset > size || length > size - offset) throw std::invalid_argument("Invalid direct memory protection range");
        if (writable && !readable) throw std::invalid_argument("Write-only page protection is not supported on x86-64");
        const int protection = (readable ? 1 : 0) | (writable ? 2 : 0) | gpuProtection;
        CheckResult(sceKernelMprotect(static_cast<std::byte*>(address) + offset, length, protection), "sceKernelMprotect");
    }
}
