#ifndef ANYUNREAL_PLATFORM_PROCESS_H
#define ANYUNREAL_PLATFORM_PROCESS_H

#include "GenericPlatform/GenericPlatformProcess.h"

struct FProcHandle : TProcHandle<int32, -1>
{
    using TProcHandle::TProcHandle;
};

struct FAnyUnrealPlatformProcess : FGenericPlatformProcess
{
    static CORE_API uint32 GetCurrentProcessId();
    static CORE_API uint32 GetCurrentCoreNumber();
    static CORE_API void SetThreadAffinityMask(uint64 mask);
    static CORE_API void Sleep(float seconds);
    static CORE_API void SleepNoStats(float seconds);
    [[noreturn]] static CORE_API void SleepInfinite();
    static CORE_API void YieldThread();
    static CORE_API FEvent* CreateSynchEvent(bool manualReset = false);
};

using FPlatformProcess = FAnyUnrealPlatformProcess;

#endif
