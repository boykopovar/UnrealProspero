#ifndef UNREALPROSPERO_MUTEX_SHARED_MUTEX_H
#define UNREALPROSPERO_MUTEX_SHARED_MUTEX_H

#include "CoreTypes.h"
#include "SceTypes.hpp"

namespace UE
{
    class FUnrealProsperoSharedMutex final
    {
    public:
        CORE_API FUnrealProsperoSharedMutex();
        CORE_API ~FUnrealProsperoSharedMutex() noexcept;
        FUnrealProsperoSharedMutex(const FUnrealProsperoSharedMutex&) = delete;
        FUnrealProsperoSharedMutex& operator=(const FUnrealProsperoSharedMutex&) = delete;
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
