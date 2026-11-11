#include "UnrealProspero/Mutex/SharedMutex.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/Pthread/include/Rwlock.hpp"

namespace UE
{
    FUnrealProsperoSharedMutex::FUnrealProsperoSharedMutex()
    {
        UnrealProspero::Kernel::CheckResult(scePthreadRwlockInit(&mutex, nullptr, nullptr), __func__);
    }

    FUnrealProsperoSharedMutex::~FUnrealProsperoSharedMutex() noexcept
    {
        UnrealProspero::Kernel::CheckResult(scePthreadRwlockDestroy(&mutex), __func__);
    }

    bool FUnrealProsperoSharedMutex::TryLock()
    {
        return UnrealProspero::Kernel::CheckLockResult(scePthreadRwlockTrywrlock(&mutex), __func__);
    }

    void FUnrealProsperoSharedMutex::Lock()
    {
        UnrealProspero::Kernel::CheckResult(scePthreadRwlockWrlock(&mutex), __func__);
    }

    void FUnrealProsperoSharedMutex::Unlock()
    {
        UnrealProspero::Kernel::CheckResult(scePthreadRwlockUnlock(&mutex), __func__);
    }

    bool FUnrealProsperoSharedMutex::TryLockShared()
    {
        return UnrealProspero::Kernel::CheckLockResult(scePthreadRwlockTryrdlock(&mutex), __func__);
    }

    void FUnrealProsperoSharedMutex::LockShared()
    {
        UnrealProspero::Kernel::CheckResult(scePthreadRwlockRdlock(&mutex), __func__);
    }

    void FUnrealProsperoSharedMutex::UnlockShared()
    {
        UnrealProspero::Kernel::CheckResult(scePthreadRwlockUnlock(&mutex), __func__);
    }
}
