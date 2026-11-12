#ifndef UNREALPROSPERO_PLATFORM_MUTEX_H
#define UNREALPROSPERO_PLATFORM_MUTEX_H

#include "UnrealProspero/Mutex/UnrealProsperoRecursiveMutex.h"
#include "UnrealProspero/Mutex/UnrealProsperoSharedMutex.h"
#include "UnrealProspero/Mutex/UnrealProsperoSystemWideMutex.h"

namespace UE
{
    using FPlatformRecursiveMutex = FUnrealProsperoRecursiveMutex;
    using FPlatformSharedMutex = FUnrealProsperoSharedMutex;
    using FPlatformSystemWideMutex = FUnrealProsperoSystemWideMutex;
}

#endif
