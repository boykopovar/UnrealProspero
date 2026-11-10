#ifndef ANYUNREAL_MUTEX_RECURSIVE_MUTEX_H
#define ANYUNREAL_MUTEX_RECURSIVE_MUTEX_H

#include "CoreTypes.h"
#include "SceTypes.hpp"

namespace UE
{
    class FAnyUnrealRecursiveMutex final
    {
    public:
        CORE_API FAnyUnrealRecursiveMutex();
        CORE_API ~FAnyUnrealRecursiveMutex() noexcept(false);
        FAnyUnrealRecursiveMutex(const FAnyUnrealRecursiveMutex&) = delete;
        FAnyUnrealRecursiveMutex& operator=(const FAnyUnrealRecursiveMutex&) = delete;
        CORE_API bool TryLock();
        CORE_API void Lock();
        CORE_API void Unlock();

    private:
        PthreadMutex mutex = nullptr;
    };
}

#endif
