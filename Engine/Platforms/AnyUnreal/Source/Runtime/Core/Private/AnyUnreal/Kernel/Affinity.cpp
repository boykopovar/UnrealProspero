#include "AnyUnreal/Kernel/Affinity.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"

namespace AnyUnreal::Kernel
{
    KernelCpumask Affinity::Available()
    {
        const auto mask = sceKernelGetAvailableCpumask();
        if (mask == 0) throw std::runtime_error("No processors are available");
        return mask;
    }

    KernelCpumask Affinity::Get(Pthread thread)
    {
        KernelCpumask mask = 0;
        CheckResult(scePthreadGetaffinity(thread, &mask), "scePthreadGetaffinity");
        if (mask == 0 || (mask & ~Available()) != 0) throw std::runtime_error("Invalid thread affinity returned by libkernel");
        return mask;
    }

    void Affinity::Set(Pthread thread, KernelCpumask mask)
    {
        if (mask == 0 || (mask & ~Available()) != 0) throw std::invalid_argument("Thread affinity contains unavailable processors");
        CheckResult(scePthreadSetaffinity(thread, mask), "scePthreadSetaffinity");
    }
}
