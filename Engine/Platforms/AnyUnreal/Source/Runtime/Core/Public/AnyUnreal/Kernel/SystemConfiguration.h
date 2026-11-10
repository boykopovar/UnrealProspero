#ifndef ANYUNREAL_KERNEL_SYSTEM_CONFIGURATION_H
#define ANYUNREAL_KERNEL_SYSTEM_CONFIGURATION_H

#include <cstddef>
#include <cstdint>

namespace AnyUnreal::Kernel
{
    class SystemConfiguration final
    {
    public:
        static int LogicalProcessorCount();
        static std::size_t PageSize();
        static std::uint64_t PhysicalMemorySize();
    };
}

#endif
