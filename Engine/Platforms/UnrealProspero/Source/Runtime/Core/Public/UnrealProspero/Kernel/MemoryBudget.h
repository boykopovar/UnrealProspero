#ifndef UNREALPROSPERO_KERNEL_MEMORY_BUDGET_H
#define UNREALPROSPERO_KERNEL_MEMORY_BUDGET_H

#include <cstdint>

namespace UnrealProspero::Kernel
{
    struct MemoryPoolUsage
    {
        std::uint64_t Capacity;
        std::uint64_t Allocated;
    };

    struct MemoryBudget
    {
        MemoryPoolUsage Direct;
        MemoryPoolUsage Flexible;

        static MemoryBudget Query();
    };
}

#endif
