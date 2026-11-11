#ifndef UNREALPROSPERO_PLATFORM_MUTEX_H
#define UNREALPROSPERO_PLATFORM_MUTEX_H

#include "UnrealProspero/Mutex/RecursiveMutex.h"
#include "UnrealProspero/Mutex/SharedMutex.h"

namespace UE
{
    class FUnrealProsperoSystemWideMutex;
    using FPlatformRecursiveMutex = FUnrealProsperoRecursiveMutex;
    using FPlatformSharedMutex = FUnrealProsperoSharedMutex;
    using FPlatformSystemWideMutex = FUnrealProsperoSystemWideMutex;
}

#endif
