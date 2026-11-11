#ifndef UNREALPROSPERO_RUNNABLE_THREAD_H
#define UNREALPROSPERO_RUNNABLE_THREAD_H

#include "HAL/RunnableThread.h"
#include "UnrealProspero/Kernel/Event.h"
#include "UnrealProspero/Kernel/Thread.h"
#include <atomic>
#include <exception>
#include <optional>

class FUnrealProsperoRunnableThread final : public FRunnableThread
{
public:
    CORE_API ~FUnrealProsperoRunnableThread() noexcept override;
    CORE_API void SetThreadPriority(EThreadPriority priority) override;
    CORE_API bool SetThreadAffinity(const FThreadAffinity& affinity) override;
    CORE_API void Suspend(bool shouldPause = true) override;
    CORE_API bool Kill(bool shouldWait = true) override;
    CORE_API void WaitForCompletion() override;

protected:
    CORE_API bool CreateInternal(FRunnable* runnable, const TCHAR* name, uint32 stackSize = 0, EThreadPriority priority = TPri_Normal, uint64 affinityMask = 0, EThreadCreateFlags flags = EThreadCreateFlags::None) override;

private:
    class Cleanup;
    UnrealProspero::Kernel::Event initialized{true};
    std::atomic<bool> completed{false};
    bool initSucceeded = false;
    std::exception_ptr initFailure;
    std::optional<UnrealProspero::Kernel::Thread> thread;
    void run();
    void releaseTls();
};

#endif
