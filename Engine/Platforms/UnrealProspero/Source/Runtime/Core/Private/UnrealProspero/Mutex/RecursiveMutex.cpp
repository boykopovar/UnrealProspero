#include "UnrealProspero/Mutex/RecursiveMutex.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/Pthread/include/Mutex.hpp"

namespace UE
{
    FUnrealProsperoRecursiveMutex::FUnrealProsperoRecursiveMutex()
    {
        constexpr int recursiveType = 2;
        PthreadMutexattr attributes = nullptr;
        UnrealProspero::Kernel::CheckResult(scePthreadMutexattrInit(&attributes), __func__);
        try
        {
            UnrealProspero::Kernel::CheckResult(scePthreadMutexattrSettype(&attributes, recursiveType), __func__);
            UnrealProspero::Kernel::CheckResult(scePthreadMutexInit(&mutex, &attributes, nullptr), __func__);
        }
        catch (...)
        {
            UnrealProspero::Kernel::CheckResult(scePthreadMutexattrDestroy(&attributes), __func__);
            if (mutex != nullptr) UnrealProspero::Kernel::CheckResult(scePthreadMutexDestroy(&mutex), __func__);
            throw;
        }
        int destroyResult = scePthreadMutexattrDestroy(&attributes);
        if (destroyResult != 0 && mutex != nullptr) UnrealProspero::Kernel::CheckResult(scePthreadMutexDestroy(&mutex), __func__);
        UnrealProspero::Kernel::CheckResult(destroyResult, __func__);
    }

    FUnrealProsperoRecursiveMutex::~FUnrealProsperoRecursiveMutex() noexcept
    {
        UnrealProspero::Kernel::CheckResult(scePthreadMutexDestroy(&mutex), __func__);
    }

    bool FUnrealProsperoRecursiveMutex::TryLock()
    {
        return UnrealProspero::Kernel::CheckLockResult(scePthreadMutexTrylock(&mutex), __func__);
    }

    void FUnrealProsperoRecursiveMutex::Lock()
    {
        UnrealProspero::Kernel::CheckResult(scePthreadMutexLock(&mutex), __func__);
    }

    void FUnrealProsperoRecursiveMutex::Unlock()
    {
        UnrealProspero::Kernel::CheckResult(scePthreadMutexUnlock(&mutex), __func__);
    }
}
