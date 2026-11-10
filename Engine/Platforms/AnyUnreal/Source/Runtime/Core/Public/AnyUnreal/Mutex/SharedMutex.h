#ifndef ANYUNREAL_MUTEX_SHARED_MUTEX_H
#define ANYUNREAL_MUTEX_SHARED_MUTEX_H

#include "CoreTypes.h"
#include "SceTypes.hpp"

namespace UE
{
    class FAnyUnrealSharedMutex final
    {
    public:
        CORE_API FAnyUnrealSharedMutex();
        CORE_API ~FAnyUnrealSharedMutex() noexcept;
        FAnyUnrealSharedMutex(const FAnyUnrealSharedMutex&) = delete;
        FAnyUnrealSharedMutex& operator=(const FAnyUnrealSharedMutex&) = delete;
        CORE_API bool TryLock();
        CORE_API void Lock();
        CORE_API void Unlock();
        CORE_API bool TryLockShared();
        CORE_API void LockShared();
        CORE_API void UnlockShared();

    private:
        PthreadRwlock mutex = nullptr;
    };
}

#endif
