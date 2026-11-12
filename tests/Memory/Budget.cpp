#include "UnrealProspero/Kernel/DirectAllocation.h"
#include "UnrealProspero/Kernel/MemoryBudget.h"
#include <cstdio>
#include <optional>
#include <stdexcept>

int main()
{
    using UnrealProspero::Kernel::DirectAllocation;
    using UnrealProspero::Kernel::MemoryBudget;
    constexpr auto page = DirectAllocation::PageSize;
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
    if (std::puts("Memory budget allocation, fragmentation and release checks passed") < 0) throw std::runtime_error("Cannot write memory results");
    return 0;
}
