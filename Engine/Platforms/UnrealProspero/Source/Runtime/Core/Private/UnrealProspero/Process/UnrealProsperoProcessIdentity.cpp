#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/Kernel/Affinity.h"
#include "UnrealProspero/Kernel/Error.h"
#include "HAL/PlatformAffinity.h"
#include "prx/libkernel/include/Api.hpp"

uint32 FUnrealProsperoPlatformProcess::GetCurrentProcessId()
{
    const int process = getpid_nid_postfix();
    if (process <= 0) throw std::runtime_error("Invalid current process identifier");
    return static_cast<uint32>(process);
}

uint32 FUnrealProsperoPlatformProcess::GetCurrentCoreNumber()
{
    const int processor = sceKernelGetCurrentCpu();
    if (processor < 0) throw UnrealProspero::Kernel::Error(processor, "sceKernelGetCurrentCpu");
    return static_cast<uint32>(processor);
}

void FUnrealProsperoPlatformProcess::SetThreadAffinityMask(uint64 mask)
{
    if (mask == FPlatformAffinity::GetNoAffinityMask()) mask = UnrealProspero::Kernel::Affinity::Available();
    UnrealProspero::Kernel::Affinity::Set(scePthreadSelf(), mask);
}
