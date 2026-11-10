#include "AnyUnreal/Mutex/RecursiveMutex.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/Pthread/include/Mutex.hpp"

namespace UE
{
    FAnyUnrealRecursiveMutex::FAnyUnrealRecursiveMutex()
    {
        constexpr int recursiveType = 2;
        PthreadMutexattr attributes = nullptr;
        AnyUnreal::Kernel::CheckResult(scePthreadMutexattrInit(&attributes), __func__);
        try
        {
            AnyUnreal::Kernel::CheckResult(scePthreadMutexattrSettype(&attributes, recursiveType), __func__);
            AnyUnreal::Kernel::CheckResult(scePthreadMutexInit(&mutex, &attributes, nullptr), __func__);
        }
        catch (...)
        {
            AnyUnreal::Kernel::CheckResult(scePthreadMutexattrDestroy(&attributes), __func__);
            if (mutex != nullptr) AnyUnreal::Kernel::CheckResult(scePthreadMutexDestroy(&mutex), __func__);
            throw;
        }
        int destroyResult = scePthreadMutexattrDestroy(&attributes);
        if (destroyResult != 0 && mutex != nullptr) AnyUnreal::Kernel::CheckResult(scePthreadMutexDestroy(&mutex), __func__);
        AnyUnreal::Kernel::CheckResult(destroyResult, __func__);
    }

    FAnyUnrealRecursiveMutex::~FAnyUnrealRecursiveMutex() noexcept(false)
    {
        AnyUnreal::Kernel::CheckResult(scePthreadMutexDestroy(&mutex), __func__);
    }

    bool FAnyUnrealRecursiveMutex::TryLock()
    {
        return AnyUnreal::Kernel::CheckLockResult(scePthreadMutexTrylock(&mutex), __func__);
    }

    void FAnyUnrealRecursiveMutex::Lock()
    {
        AnyUnreal::Kernel::CheckResult(scePthreadMutexLock(&mutex), __func__);
    }

    void FAnyUnrealRecursiveMutex::Unlock()
    {
        AnyUnreal::Kernel::CheckResult(scePthreadMutexUnlock(&mutex), __func__);
    }
}
