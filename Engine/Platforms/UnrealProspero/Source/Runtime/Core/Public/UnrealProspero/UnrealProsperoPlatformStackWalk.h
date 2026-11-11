#ifndef UNREALPROSPERO_PLATFORM_STACK_WALK_H
#define UNREALPROSPERO_PLATFORM_STACK_WALK_H

#include "GenericPlatform/GenericPlatformStackWalk.h"

struct FUnrealProsperoPlatformStackWalk : FGenericPlatformStackWalk
{
    [[noreturn]] static CORE_API bool InitStackWalking();
    [[noreturn]] static CORE_API bool InitStackWalkingForProcess(const FProcHandle& process);
    [[noreturn]] static CORE_API void ProgramCounterToSymbolInfo(uint64 programCounter, FProgramCounterSymbolInfo& info);
    [[noreturn]] static CORE_API void ProgramCounterToSymbolInfoEx(uint64 programCounter, FProgramCounterSymbolInfoEx& info);
    [[noreturn]] static CORE_API uint32 CaptureStackBackTrace(uint64* backTrace, uint32 maxDepth, void* context = nullptr);
    [[noreturn]] static CORE_API uint32 CaptureThreadStackBackTrace(uint64 threadId, uint64* backTrace, uint32 maxDepth, void* context = nullptr);
    [[noreturn]] static CORE_API void ThreadStackWalkAndDump(ANSICHAR* output, SIZE_T outputSize, int32 ignoreCount, uint32 threadId);
    [[noreturn]] static CORE_API int32 GetProcessModuleCount();
    [[noreturn]] static CORE_API int32 GetProcessModuleSignatures(FStackWalkModuleInfo* signatures, int32 capacity);
};

using FPlatformStackWalk = FUnrealProsperoPlatformStackWalk;

#endif
