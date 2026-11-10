#ifndef ANYUNREAL_PLATFORM_MUTEX_H
#define ANYUNREAL_PLATFORM_MUTEX_H

#include "AnyUnreal/Mutex/RecursiveMutex.h"
#include "AnyUnreal/Mutex/SharedMutex.h"

namespace UE
{
    class FAnyUnrealSystemWideMutex;
    using FPlatformRecursiveMutex = FAnyUnrealRecursiveMutex;
    using FPlatformSharedMutex = FAnyUnrealSharedMutex;
    using FPlatformSystemWideMutex = FAnyUnrealSystemWideMutex;
}

#endif
