#include "UnrealProspero/Kernel/MemoryBudget.h"
#include "UnrealProspero/Kernel/DirectAllocation.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace UnrealProspero::Kernel
{
    MemoryBudget MemoryBudget::Query()
    {
        struct DirectMemoryRange
        {
            std::int64_t Start;
            std::int64_t End;
            int Type;
        };
        static_assert(sizeof(DirectMemoryRange) == 24);
        static_assert(offsetof(DirectMemoryRange, End) == 8);
        static_assert(offsetof(DirectMemoryRange, Type) == 16);
        MemoryBudget budget{};
        budget.Direct.Capacity = sceKernelGetDirectMemorySize();
        if (budget.Direct.Capacity == 0 || budget.Direct.Capacity > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) || budget.Direct.Capacity % DirectAllocation::PageSize != 0) throw std::runtime_error("Invalid direct memory capacity");
        std::int64_t cursor = 0;
        while (static_cast<std::uint64_t>(cursor) < budget.Direct.Capacity)
        {
            DirectMemoryRange range{};
            const int result = sceKernelDirectMemoryQuery(cursor, 1, &range, sizeof(range));
            if (result == SCE_KERNEL_ERROR_EACCES) break;
            CheckResult(result, "sceKernelDirectMemoryQuery");
            if (range.Start < cursor || range.End <= range.Start || static_cast<std::uint64_t>(range.End) > budget.Direct.Capacity || range.Start % DirectAllocation::PageSize != 0 || range.End % DirectAllocation::PageSize != 0) throw std::runtime_error("Invalid direct memory range");
            budget.Direct.Allocated += static_cast<std::uint64_t>(range.End - range.Start);
            cursor = range.End;
        }
        std::size_t configured = 0;
        std::size_t available = 0;
        CheckResult(sceKernelConfiguredFlexibleMemorySize(&configured), "sceKernelConfiguredFlexibleMemorySize");
        CheckResult(sceKernelAvailableFlexibleMemorySize(&available), "sceKernelAvailableFlexibleMemorySize");
        if (available > configured) throw std::runtime_error("Flexible memory availability exceeds capacity");
        budget.Flexible.Capacity = configured;
        budget.Flexible.Allocated = configured - available;
        return budget;
    }
}
