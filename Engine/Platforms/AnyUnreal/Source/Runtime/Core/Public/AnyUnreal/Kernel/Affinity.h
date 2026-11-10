#ifndef ANYUNREAL_KERNEL_AFFINITY_H
#define ANYUNREAL_KERNEL_AFFINITY_H

#include "SceTypes.hpp"

namespace AnyUnreal::Kernel
{
    class Affinity final
    {
    public:
        static KernelCpumask Available();
        static KernelCpumask Get(Pthread thread);
        static void Set(Pthread thread, KernelCpumask mask);
    };
}

#endif
