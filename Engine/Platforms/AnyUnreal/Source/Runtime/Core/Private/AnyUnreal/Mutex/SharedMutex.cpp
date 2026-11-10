#include "AnyUnreal/Mutex/SharedMutex.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/Pthread/include/Rwlock.hpp"

namespace UE
{
    FAnyUnrealSharedMutex::FAnyUnrealSharedMutex()
    {
        AnyUnreal::Kernel::CheckResult(scePthreadRwlockInit(&mutex, nullptr, nullptr), __func__);
    }

    FAnyUnrealSharedMutex::~FAnyUnrealSharedMutex() noexcept
    {
        AnyUnreal::Kernel::CheckResult(scePthreadRwlockDestroy(&mutex), __func__);
    }

    bool FAnyUnrealSharedMutex::TryLock()
    {
        return AnyUnreal::Kernel::CheckLockResult(scePthreadRwlockTrywrlock(&mutex), __func__);
    }

    void FAnyUnrealSharedMutex::Lock()
    {
        AnyUnreal::Kernel::CheckResult(scePthreadRwlockWrlock(&mutex), __func__);
    }

    void FAnyUnrealSharedMutex::Unlock()
    {
        AnyUnreal::Kernel::CheckResult(scePthreadRwlockUnlock(&mutex), __func__);
    }

    bool FAnyUnrealSharedMutex::TryLockShared()
    {
        return AnyUnreal::Kernel::CheckLockResult(scePthreadRwlockTryrdlock(&mutex), __func__);
    }

    void FAnyUnrealSharedMutex::LockShared()
    {
        AnyUnreal::Kernel::CheckResult(scePthreadRwlockRdlock(&mutex), __func__);
    }

    void FAnyUnrealSharedMutex::UnlockShared()
    {
        AnyUnreal::Kernel::CheckResult(scePthreadRwlockUnlock(&mutex), __func__);
    }
}
