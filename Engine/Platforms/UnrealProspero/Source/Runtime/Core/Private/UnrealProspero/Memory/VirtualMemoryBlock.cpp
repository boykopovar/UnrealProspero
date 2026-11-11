#include "UnrealProspero/Memory/VirtualMemoryBlock.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Memory
{
    VirtualMemoryBlock::VirtualMemoryBlock(void* address, uint32 pageCount) : FBasicVirtualMemoryBlock(address, pageCount)
    {
        const auto pointer = reinterpret_cast<std::uintptr_t>(address);
        if (pointer == 0 || pageCount == 0 || pointer % GetVirtualSizeAlignment() != 0 || GetActualSize() > std::numeric_limits<std::uintptr_t>::max() - pointer) throw std::invalid_argument("Invalid virtual memory block extent");
    }

    VirtualMemoryBlock VirtualMemoryBlock::AllocateVirtual(std::size_t size, std::size_t alignment)
    {
        constexpr auto page = GetVirtualSizeAlignment();
        if (size == 0 || size > std::numeric_limits<std::size_t>::max() - (page - 1)) throw std::invalid_argument("Invalid virtual memory size");
        if (!std::has_single_bit(alignment)) throw std::invalid_argument("Invalid virtual memory alignment");
        alignment = std::max(page, alignment);
        const auto rounded = (size + page - 1) & ~(page - 1);
        if (rounded / page > std::numeric_limits<uint32>::max()) throw std::overflow_error("Virtual memory block exceeds the page count limit");
        VirtualMemoryBlock result;
        Kernel::CheckResult(sceKernelReserveVirtualRange(&result.Ptr, rounded, 0, alignment), "sceKernelReserveVirtualRange");
        const auto address = reinterpret_cast<std::uintptr_t>(result.Ptr);
        if (address == 0 || address % alignment != 0 || rounded > std::numeric_limits<std::uintptr_t>::max() - address)
        {
            Kernel::CheckResult(sceKernelMunmap(address, rounded), "sceKernelMunmap");
            throw std::runtime_error("Kernel returned an invalid virtual memory reservation");
        }
        result.VMSizeDivVirtualSizeAlignment = static_cast<uint32>(rounded / page);
        return result;
    }

    std::size_t VirtualMemoryBlock::GetActualSize() const
    {
        return static_cast<std::size_t>(VMSizeDivVirtualSizeAlignment) * GetVirtualSizeAlignment();
    }

    void VirtualMemoryBlock::validateRange(std::size_t offset, std::size_t size) const
    {
        if (!Ptr) throw std::logic_error("Virtual memory block is not allocated");
        if (offset % GetCommitAlignment() != 0 || size % GetCommitAlignment() != 0 || offset > GetActualSize() || size > GetActualSize() - offset) throw std::invalid_argument("Invalid virtual memory range");
    }

    std::size_t VirtualMemoryBlock::offsetOf(void* address) const
    {
        const auto base = reinterpret_cast<std::uintptr_t>(Ptr);
        const auto pointer = reinterpret_cast<std::uintptr_t>(address);
        if (!Ptr || pointer < base || pointer - base > GetActualSize()) throw std::invalid_argument("Address is outside the virtual memory block");
        return pointer - base;
    }

    void VirtualMemoryBlock::Commit(std::size_t offset, std::size_t size)
    {
        validateRange(offset, size);
        auto current = reinterpret_cast<std::uintptr_t>(Ptr) + offset;
        const auto end = current + size;
        while (current != end)
        {
            VirtualQueryInfo info{};
            Kernel::CheckResult(sceKernelVirtualQuery(reinterpret_cast<void*>(current), 0, &info, sizeof(info)), "sceKernelVirtualQuery");
            if (info.start > current || info.end <= current || info.end % GetCommitAlignment() != 0) throw std::runtime_error("Invalid virtual memory query extent");
            const auto next = std::min(end, info.end);
            if (info.is_committed)
            {
                Kernel::CheckResult(sceKernelMprotect(reinterpret_cast<void*>(current), next - current, 3), "sceKernelMprotect");
            }
            else
            {
                constexpr int privateAnonymousFixed = 0x1012;
                void* mapped = nullptr;
                Kernel::CheckResult(sceKernelMmap(reinterpret_cast<void*>(current), next - current, 3, privateAnonymousFixed, -1, 0, &mapped), "sceKernelMmap");
                if (reinterpret_cast<std::uintptr_t>(mapped) != current) throw std::runtime_error("Fixed virtual memory mapping moved");
            }
            current = next;
        }
    }

    void VirtualMemoryBlock::Decommit(std::size_t offset, std::size_t size)
    {
        validateRange(offset, size);
        if (size == 0) return;
        const auto expected = reinterpret_cast<std::uintptr_t>(Ptr) + offset;
        void* address = reinterpret_cast<void*>(expected);
        constexpr int fixed = 0x10;
        Kernel::CheckResult(sceKernelReserveVirtualRange(&address, size, fixed, GetCommitAlignment()), "sceKernelReserveVirtualRange");
        if (reinterpret_cast<std::uintptr_t>(address) != expected) throw std::runtime_error("Fixed virtual memory reservation moved");
    }

    void VirtualMemoryBlock::Commit() { Commit(0, GetActualSize()); }
    void VirtualMemoryBlock::Decommit() { Decommit(0, GetActualSize()); }
    void VirtualMemoryBlock::CommitByPtr(void* address, std::size_t size) { Commit(offsetOf(address), size); }
    void VirtualMemoryBlock::DecommitByPtr(void* address, std::size_t size) { Decommit(offsetOf(address), size); }

    void VirtualMemoryBlock::FreeVirtual()
    {
        if (!Ptr) return;
        Kernel::CheckResult(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(Ptr), GetActualSize()), "sceKernelMunmap");
        Ptr = nullptr;
        VMSizeDivVirtualSizeAlignment = 0;
    }
}
