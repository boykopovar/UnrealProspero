#ifndef UNREALPROSPERO_PLATFORM_PROCESS_H
#define UNREALPROSPERO_PLATFORM_PROCESS_H

#include "GenericPlatform/GenericPlatformProcess.h"

struct FProcHandle : TProcHandle<int32, -1>
{
    using TProcHandle::TProcHandle;
};

struct FUnrealProsperoPlatformProcess : FGenericPlatformProcess
{
    static CORE_API const TCHAR* BaseDir();
    static CORE_API const TCHAR* UserTempDir();
    static CORE_API const TCHAR* ExecutableName(bool removeExtension = true);
    static CORE_API void* GetDllHandle(const TCHAR* filename);
    static CORE_API void FreeDllHandle(void* handle);
    static CORE_API void* GetDllExport(void* handle, const TCHAR* name);
    static CORE_API uint32 GetCurrentProcessId();
    static CORE_API uint32 GetCurrentCoreNumber();
    static CORE_API void SetThreadAffinityMask(uint64 mask);
    static CORE_API void Sleep(float seconds);
    static CORE_API void SleepNoStats(float seconds);
    [[noreturn]] static CORE_API void SleepInfinite();
    static CORE_API void YieldThread();
    static CORE_API FRunnableThread* CreateRunnableThread();
    static CORE_API FEvent* CreateSynchEvent(bool manualReset = false);
};

using FPlatformProcess = FUnrealProsperoPlatformProcess;

#endif
