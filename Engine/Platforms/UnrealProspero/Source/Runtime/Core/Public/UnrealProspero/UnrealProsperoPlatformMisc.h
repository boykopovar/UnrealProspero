#ifndef UNREALPROSPERO_PLATFORM_MISC_H
#define UNREALPROSPERO_PLATFORM_MISC_H

#include "GenericPlatform/GenericPlatformMisc.h"

#define UE_DEBUG_BREAK_IMPL() FUnrealProsperoPlatformMisc::DebugBreak()

struct FUnrealProsperoPlatformMisc : FGenericPlatformMisc
{
#if UNREALPROSPERO_TARGET_PS5
    static CORE_API const TCHAR* RootDir();
    static CORE_API const TCHAR* EngineDir();
#endif
    static const TCHAR* GetPlatformFeaturesModuleName() { return TEXT("UnrealProsperoPlatformFeatures"); }
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

using FPlatformMisc = FUnrealProsperoPlatformMisc;

#endif
