#ifndef ANYUNREAL_PLATFORM_MISC_H
#define ANYUNREAL_PLATFORM_MISC_H

#include "GenericPlatform/GenericPlatformMisc.h"

#define UE_DEBUG_BREAK_IMPL() FAnyUnrealPlatformMisc::DebugBreak()

struct FAnyUnrealPlatformMisc : FGenericPlatformMisc
{
    static CORE_API int32 NumberOfCoresIncludingHyperthreads();
    static CORE_API bool IsDebuggerPresent();
    static CORE_API void DebugBreak();
    static CORE_API void PromptForRemoteDebugging(bool isEnsure);
    static CORE_API void MemoryBarrier();
    static CORE_API void LowLevelOutputDebugString(const TCHAR* message);
};

using FPlatformMisc = FAnyUnrealPlatformMisc;

#endif
