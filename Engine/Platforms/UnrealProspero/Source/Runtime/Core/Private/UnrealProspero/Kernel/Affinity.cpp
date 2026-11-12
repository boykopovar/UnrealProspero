#include "UnrealProspero/Kernel/Affinity.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Kernel/PosixResult.h"
#include "prx/libkernel/include/Api.hpp"

namespace UnrealProspero::Kernel
{
    KernelCpumask Affinity::Available()
    {
        static const KernelCpumask available = []
        {
            constexpr int processAffinity = 3;
            constexpr int process = 2;
            KernelCpumask mask = 0;
            CheckPosixResult(cpuset_getaffinity_nid_postfix(processAffinity, process, -1, sizeof(mask), &mask), "cpuset_getaffinity");
            if (mask == 0) throw std::runtime_error("No processors are available");
            return mask;
        }();
        return available;
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
