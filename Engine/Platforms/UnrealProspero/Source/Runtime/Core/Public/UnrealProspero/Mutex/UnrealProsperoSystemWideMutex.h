#ifndef UNREALPROSPERO_SYSTEM_WIDE_MUTEX_H
#define UNREALPROSPERO_SYSTEM_WIDE_MUTEX_H

#include "CoreTypes.h"
#include "Misc/Timespan.h"
#include "UnrealProspero/Kernel/FileLock.h"

class FString;

namespace UE
{
    class FUnrealProsperoSystemWideMutex final
    {
    public:
        CORE_API explicit FUnrealProsperoSystemWideMutex(const FString& name, FTimespan timeout = FTimespan::Zero());
        ~FUnrealProsperoSystemWideMutex() noexcept(false) = default;
        FUnrealProsperoSystemWideMutex(const FUnrealProsperoSystemWideMutex&) = delete;
        FUnrealProsperoSystemWideMutex& operator=(const FUnrealProsperoSystemWideMutex&) = delete;
        bool IsValid() const noexcept { return lock.IsOwned(); }
        CORE_API void Release();

    private:
        UnrealProspero::Kernel::FileLock lock;
    };
}

#endif
