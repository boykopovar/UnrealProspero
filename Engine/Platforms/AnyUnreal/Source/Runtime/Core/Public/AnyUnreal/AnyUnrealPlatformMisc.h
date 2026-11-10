#ifndef ANYUNREAL_PLATFORM_MISC_H
#define ANYUNREAL_PLATFORM_MISC_H

#include "GenericPlatform/GenericPlatformMisc.h"

#define UE_DEBUG_BREAK_IMPL() FAnyUnrealPlatformMisc::DebugBreak()

struct FAnyUnrealPlatformMisc : FGenericPlatformMisc
{
    static CORE_API void RequestExit(bool force, const TCHAR* callSite = nullptr);
    static CORE_API void RequestExitWithStatus(bool force, uint8 status, const TCHAR* callSite = nullptr);
    static CORE_API bool HasOverriddenReturnCode(uint8* status);
    static CORE_API void CreateGuid(FGuid& result);
    static CORE_API int32 NumberOfCoresIncludingHyperthreads();
    static CORE_API bool IsDebuggerPresent();
    static CORE_API void DebugBreak();
    static CORE_API void PromptForRemoteDebugging(bool isEnsure);
    static CORE_API void MemoryBarrier();
    static CORE_API void LowLevelOutputDebugString(const TCHAR* message);
};

using FPlatformMisc = FAnyUnrealPlatformMisc;

#endif
