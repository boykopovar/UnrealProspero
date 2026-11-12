#ifndef UNREALPROSPERO_KERNEL_MEMORY_BUDGET_H
#define UNREALPROSPERO_KERNEL_MEMORY_BUDGET_H

#include <cstdint>

namespace UnrealProspero::Kernel
{
    struct MemoryPoolBudget
    {
        std::uint64_t Capacity;
        std::uint64_t Available;
    };

    struct MemoryBudget
    {
        MemoryPoolBudget Direct;
        MemoryPoolBudget Flexible;

        static MemoryBudget Query();
    };
}

#endif
