#include "UnrealProspero/UnrealProsperoPlatformMemory.h"
#include "UnrealProspero/Kernel/MemoryBudget.h"
#include "UnrealProspero/Kernel/ProcessMemory.h"
#include "UnrealProspero/Kernel/SystemConfiguration.h"
#include <algorithm>
#include <atomic>
#include <limits>

namespace
{
    std::atomic<std::uint64_t> peakPhysical{0};
    std::atomic<std::uint64_t> peakVirtual{0};
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

    std::uint64_t sum(std::uint64_t first, std::uint64_t second)
    {
        if (second > std::numeric_limits<std::uint64_t>::max() - first) throw std::overflow_error("Memory budget exceeds the byte counter range");
        return first + second;
    }

    std::uint64_t updatePeak(std::atomic<std::uint64_t>& peak, std::uint64_t value)
    {
        auto previous = peak.load(std::memory_order_relaxed);
        while (previous < value && !peak.compare_exchange_weak(previous, value, std::memory_order_relaxed)) {}
        return std::max(previous, value);
    }
}

const FPlatformMemoryConstants& FUnrealProsperoPlatformMemory::GetConstants()
{
    static const FPlatformMemoryConstants constants = []
    {
        const auto budget = UnrealProspero::Kernel::MemoryBudget::Query();
        FPlatformMemoryConstants result{};
        result.TotalPhysical = sum(budget.Direct.Capacity, budget.Flexible.Capacity);
        result.TotalVirtual = result.TotalPhysical;
        result.PageSize = UnrealProspero::Kernel::SystemConfiguration::PageSize();
        result.OsAllocationGranularity = UnrealProspero::Kernel::DirectAllocation::PageSize;
        result.BinnedPageSize = UnrealProspero::Memory::PageAllocator::Alignment;
        result.BinnedAllocationGranularity = result.OsAllocationGranularity;
        const auto gigabytes = result.TotalPhysical / (1ull << 30) + (result.TotalPhysical % (1ull << 30) != 0);
        if (gigabytes == 0 || gigabytes > std::numeric_limits<uint32>::max()) throw std::runtime_error("Invalid application memory capacity");
        result.TotalPhysicalGB = static_cast<uint32>(gigabytes);
        return result;
    }();
    return constants;
}

FPlatformMemoryStats FUnrealProsperoPlatformMemory::GetStats()
{
    FPlatformMemoryStats result;
    const auto budget = UnrealProspero::Kernel::MemoryBudget::Query();
    const auto process = UnrealProspero::Kernel::ProcessMemory::Query();
    result.AvailablePhysical = sum(budget.Direct.Available, budget.Flexible.Available);
    result.AvailableVirtual = result.AvailablePhysical;
    result.UsedPhysical = process.ResidentBytes;
    result.UsedVirtual = process.VirtualBytes;
    result.PeakUsedPhysical = updatePeak(peakPhysical, std::max(process.ResidentBytes, process.PeakResidentBytes));
    result.PeakUsedVirtual = updatePeak(peakVirtual, process.VirtualBytes);
    return result;
}
