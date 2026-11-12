#include "UnrealProspero/Kernel/DirectAllocation.h"
#include "UnrealProspero/Kernel/MemoryBudget.h"
#include "UnrealProspero/Kernel/ProcessMemory.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstdio>
#include <optional>
#include <stdexcept>

int main()
{
    using UnrealProspero::Kernel::DirectAllocation;
    using UnrealProspero::Kernel::MemoryBudget;
    constexpr auto page = DirectAllocation::PageSize;
    const auto process = UnrealProspero::Kernel::ProcessMemory::Query();
    if (std::printf("Process memory: virtual=%llu resident=%llu peak=%llu\n", static_cast<unsigned long long>(process.VirtualBytes), static_cast<unsigned long long>(process.ResidentBytes), static_cast<unsigned long long>(process.PeakResidentBytes)) < 0) throw std::runtime_error("Cannot write process memory results");
    const auto initial = MemoryBudget::Query();
    const auto check = [&](std::uint64_t added)
    {
        const auto current = MemoryBudget::Query();
        if (current.Direct.Capacity != initial.Direct.Capacity || current.Direct.Allocated != initial.Direct.Allocated + added) throw std::runtime_error("Incorrect direct memory accounting");
        if (current.Flexible.Capacity != initial.Flexible.Capacity || current.Flexible.Allocated > current.Flexible.Capacity) throw std::runtime_error("Incorrect flexible memory accounting");
    };
    {
        DirectAllocation first(page);
        std::optional<DirectAllocation> middle(std::in_place, 2 * page);
        DirectAllocation last(3 * page);
        check(6 * page);
        middle.reset();
        check(4 * page);
        middle.emplace(2 * page);
        check(6 * page);
    }
    check(0);
    const auto beforeFlexible = MemoryBudget::Query();
    void* flexible = nullptr;
    UnrealProspero::Kernel::CheckResult(sceKernelMapFlexibleMemory(&flexible, 2 * page, 3, 0), "sceKernelMapFlexibleMemory");
    const auto allocatedFlexible = MemoryBudget::Query();
    UnrealProspero::Kernel::CheckResult(sceKernelReleaseFlexibleMemory(flexible, 2 * page), "sceKernelReleaseFlexibleMemory");
    const auto releasedFlexible = MemoryBudget::Query();
    if (allocatedFlexible.Flexible.Allocated != beforeFlexible.Flexible.Allocated + 2 * page || releasedFlexible.Flexible.Allocated != beforeFlexible.Flexible.Allocated) throw std::runtime_error("Incorrect flexible memory allocation accounting");
    if (allocatedFlexible.Direct.Allocated != beforeFlexible.Direct.Allocated || releasedFlexible.Direct.Allocated != beforeFlexible.Direct.Allocated) throw std::runtime_error("Flexible memory changed the direct memory budget");
    if (std::puts("Memory budget allocation, fragmentation and release checks passed") < 0) throw std::runtime_error("Cannot write memory results");
    return 0;
}
