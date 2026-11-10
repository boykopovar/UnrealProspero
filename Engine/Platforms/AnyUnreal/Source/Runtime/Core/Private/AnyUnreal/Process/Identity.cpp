#include "AnyUnreal/AnyUnrealPlatformProcess.h"
#include "AnyUnreal/Kernel/Affinity.h"
#include "AnyUnreal/Kernel/Error.h"
#include "HAL/PlatformAffinity.h"
#include "prx/libkernel/include/Api.hpp"

uint32 FAnyUnrealPlatformProcess::GetCurrentProcessId()
{
    const int process = getpid_nid_postfix();
    if (process <= 0) throw std::runtime_error("Invalid current process identifier");
    return static_cast<uint32>(process);
}

uint32 FAnyUnrealPlatformProcess::GetCurrentCoreNumber()
{
    const int processor = sceKernelGetCurrentCpu();
    if (processor < 0) throw AnyUnreal::Kernel::Error(processor, "sceKernelGetCurrentCpu");
    return static_cast<uint32>(processor);
}

void FAnyUnrealPlatformProcess::SetThreadAffinityMask(uint64 mask)
{
    if (mask == FPlatformAffinity::GetNoAffinityMask()) mask = AnyUnreal::Kernel::Affinity::Available();
    AnyUnreal::Kernel::Affinity::Set(scePthreadSelf(), mask);
}
