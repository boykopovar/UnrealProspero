#ifndef UNREALPROSPERO_KERNEL_PROCESS_MEMORY_H
#define UNREALPROSPERO_KERNEL_PROCESS_MEMORY_H

#include <cstdint>

namespace UnrealProspero::Kernel
{
    struct ProcessMemory
    {
        std::uint64_t VirtualBytes;
        std::uint64_t ResidentBytes;
        std::uint64_t PeakResidentBytes;

        static ProcessMemory Query();
    };
}

#endif
