#ifndef UNREALPROSPERO_MUTEX_RECURSIVE_MUTEX_H
#define UNREALPROSPERO_MUTEX_RECURSIVE_MUTEX_H

#include "CoreTypes.h"
#include "SceTypes.hpp"

namespace UE
{
    class FUnrealProsperoRecursiveMutex final
    {
    public:
        CORE_API FUnrealProsperoRecursiveMutex();
        CORE_API ~FUnrealProsperoRecursiveMutex() noexcept;
        FUnrealProsperoRecursiveMutex(const FUnrealProsperoRecursiveMutex&) = delete;
        FUnrealProsperoRecursiveMutex& operator=(const FUnrealProsperoRecursiveMutex&) = delete;
        CORE_API bool TryLock();
        CORE_API void Lock();
        CORE_API void Unlock();

    private:
        PthreadMutex mutex = nullptr;
    };
}

#endif
