#ifndef UNREALPROSPERO_KERNEL_AFFINITY_H
#define UNREALPROSPERO_KERNEL_AFFINITY_H

#include "SceTypes.hpp"

namespace UnrealProspero::Kernel
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
