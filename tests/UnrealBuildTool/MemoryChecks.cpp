#include "AnyUnreal/Kernel/DirectAllocation.h"
#include "HAL/PlatformMemory.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace
{
    void* APS5_VABI checkConcurrentPages(void* argument)
    {
        auto& passed = *static_cast<bool*>(argument);
        try
        {
            for (int index = 0; index < 8; ++index)
            {
                constexpr auto size = AnyUnreal::Memory::PageAllocator::Alignment;
                void* allocation = FPlatformMemory::BinnedAllocFromOS(size);
                auto* bytes = static_cast<unsigned char*>(allocation);
                bytes[0] = static_cast<unsigned char>(index);
                if (!FPlatformMemory::PageProtect(allocation, size, true, false) || bytes[0] != index) return nullptr;
                FPlatformMemory::BinnedFreeToOS(allocation, size);
            }
            passed = true;
        }
        catch (...)
        {
            passed = false;
        }
        return nullptr;
    }

    bool checkTrackerAllocator()
    {
        void* (*allocate)(std::size_t) = nullptr;
        void (*free)(void*, std::size_t) = nullptr;
        int32 alignment = 0;
        if (!FPlatformMemory::GetLLMAllocFunctions(allocate, free, alignment)) return false;
        if (!allocate || !free || alignment != AnyUnreal::Kernel::DirectAllocation::PageSize) return false;
        struct Allocation
        {
            void* Address;
            std::size_t Size;
            void (*Free)(void*, std::size_t);
            ~Allocation() noexcept(false) { Free(Address, Size); }
        };
        const auto size = static_cast<std::size_t>(alignment);
        Allocation first{allocate(size * 3), size * 3, free};
        Allocation second{allocate(size), size, free};
        if (!first.Address || !second.Address || first.Address == second.Address) return false;
        if (reinterpret_cast<std::uintptr_t>(first.Address) % size != 0 || reinterpret_cast<std::uintptr_t>(second.Address) % size != 0) return false;
        std::memset(first.Address, 0x5a, first.Size);
        std::memset(second.Address, 0xa5, second.Size);
        const auto* firstBytes = static_cast<const unsigned char*>(first.Address);
        const auto* secondBytes = static_cast<const unsigned char*>(second.Address);
        for (std::size_t offset = 0; offset < first.Size; ++offset) if (firstBytes[offset] != 0x5a) return false;
        for (std::size_t offset = 0; offset < second.Size; ++offset) if (secondBytes[offset] != 0xa5) return false;
        return true;
    }

    bool checkProtection(void* address, int expected)
    {
        void* start = nullptr;
        void* end = nullptr;
        int protection = 0;
        AnyUnreal::Kernel::CheckResult(sceKernelQueryMemoryProtection(address, &start, &end, &protection), "sceKernelQueryMemoryProtection");
        const auto location = reinterpret_cast<std::uintptr_t>(address);
        return protection == expected && reinterpret_cast<std::uintptr_t>(start) <= location && location < reinterpret_cast<std::uintptr_t>(end);
    }
}

bool CheckMemory()
{
    if (!checkTrackerAllocator()) return false;
    using AnyUnreal::Kernel::DirectAllocation;
    constexpr auto page = DirectAllocation::PageSize;
    {
        using AnyUnreal::Kernel::DirectMemoryAccess;
        DirectAllocation allocation(page * 3, 65536, DirectMemoryAccess::CpuGpu);
        auto* bytes = static_cast<unsigned char*>(allocation.Data());
        if (!checkProtection(bytes, 0x33)) return false;
        allocation.ProtectCpu(page, page, true, false);
        if (!checkProtection(bytes, 0x33) || !checkProtection(bytes + page, 0x31) || !checkProtection(bytes + page * 2, 0x33)) return false;
        allocation.ProtectCpu(page, page, false, false);
        if (!checkProtection(bytes + page, 0x30)) return false;
        allocation.ProtectCpu(page, page, true, true);
        bytes[page] = 43;
        if (!checkProtection(bytes + page, 0x33) || bytes[page] != 43) return false;
        try
        {
            DirectAllocation invalid(page, page, static_cast<DirectMemoryAccess>(-1));
            return false;
        }
        catch (const std::invalid_argument&)
        {
        }
    }
    const std::size_t alignments[]{page, 65536, 2097152};
    for (const auto alignment : alignments)
    {
        for (int repetition = 0; repetition < 2; ++repetition)
        {
            DirectAllocation allocation(page * 3, alignment);
            if (allocation.Size() != page * 3 || reinterpret_cast<std::uintptr_t>(allocation.Data()) % alignment != 0) return false;
            auto* bytes = static_cast<unsigned char*>(allocation.Data());
            for (std::size_t i = 0; i < allocation.Size(); ++i)
            {
                if (bytes[i] != 0) return false;
                bytes[i] = static_cast<unsigned char>(i % 251);
            }
            allocation.ProtectCpu(page, page, true, false);
            if (!checkProtection(bytes, 3) || !checkProtection(bytes + page, 1) || !checkProtection(bytes + page * 2, 3)) return false;
            for (std::size_t i = 0; i < allocation.Size(); ++i)
            {
                if (bytes[i] != static_cast<unsigned char>(i % 251)) return false;
            }
            allocation.ProtectCpu(page, page, false, false);
            if (!checkProtection(bytes + page, 0)) return false;
            allocation.ProtectCpu(page, page, true, true);
            bytes[page] = 42;
            if (!checkProtection(bytes + page, 3) || bytes[page] != 42) return false;
            try
            {
                allocation.ProtectCpu(page * 2, page * 2, true, true);
                return false;
            }
            catch (const std::invalid_argument&)
            {
            }
        }
    }
    try
    {
        DirectAllocation invalid(page, page * 3);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    try
    {
        DirectAllocation invalid(page - 1);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    void* allocation = FPlatformMemory::BinnedAllocFromOS(page * 4);
    if (reinterpret_cast<std::uintptr_t>(allocation) % AnyUnreal::Memory::PageAllocator::Alignment != 0) return false;
    auto* bytes = static_cast<unsigned char*>(allocation);
    bytes[page] = 71;
    if (!FPlatformMemory::PageProtect(bytes + page, page, true, false)) return false;
    if (!checkProtection(bytes + page, 1) || bytes[page] != 71) return false;
    try
    {
        FPlatformMemory::BinnedFreeToOS(allocation, page);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    if (bytes[page] != 71) return false;
    FPlatformMemory::BinnedFreeToOS(allocation, page * 4);
    try
    {
        FPlatformMemory::BinnedFreeToOS(allocation, page * 4);
        return false;
    }
    catch (const std::invalid_argument&)
    {
    }
    try
    {
        FPlatformMemory::GetStats();
        return false;
    }
    catch (const std::runtime_error&)
    {
    }
    Pthread threads[4]{};
    bool passed[4]{};
    for (int index = 0; index < 4; ++index) AnyUnreal::Kernel::CheckResult(scePthreadCreate(&threads[index], nullptr, checkConcurrentPages, &passed[index], "AnyUnrealPages"), "scePthreadCreate");
    for (auto& thread : threads) AnyUnreal::Kernel::CheckResult(scePthreadJoin(thread, nullptr), "scePthreadJoin");
    for (const bool result : passed) if (!result) return false;
    return true;
}
