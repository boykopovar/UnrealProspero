#include "AnyUnreal/AnyUnrealRunnableThread.h"
#include "AnyUnreal/Kernel/Affinity.h"
#include "AnyUnreal/Kernel/Result.h"
#include "AnyUnreal/Runnable/Priority.h"
#include "AnyUnreal/Text/Encoding.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadManager.h"
#include "prx/libkernel/include/Api.hpp"
#include "Stats/StatsSystemTypes.h"
#include <stdexcept>

class FAnyUnrealRunnableThread::Cleanup final
{
public:
    explicit Cleanup(FAnyUnrealRunnableThread& owner) : owner(owner) {}
    ~Cleanup() noexcept(false)
    {
        if (tls) owner.releaseTls();
        owner.completed.store(true, std::memory_order_release);
    }
    void EnableTls() noexcept { tls = true; }

private:
    FAnyUnrealRunnableThread& owner;
    bool tls = false;
};

FAnyUnrealRunnableThread::~FAnyUnrealRunnableThread() noexcept
{
    Kill(true);
}

bool FAnyUnrealRunnableThread::CreateInternal(FRunnable* runnable, const TCHAR* name, uint32 stackSize, EThreadPriority priority, uint64 affinityMask, EThreadCreateFlags flags)
{
    if (thread) throw std::logic_error("Runnable thread has already been created");
    if (!runnable || !name) throw std::invalid_argument("Runnable thread requires a runnable and a name");
    if (flags != EThreadCreateFlags::None && flags != EThreadCreateFlags::SMTExclusive) throw std::invalid_argument("Unknown thread creation flags");
    AnyUnreal::Kernel::ThreadAttributes attributes;
    attributes.SetPriority(AnyUnreal::Runnable::GetKernelPriority(priority));
    attributes.SetExclusiveScheduling(flags == EThreadCreateFlags::SMTExclusive);
    if (stackSize != 0) attributes.SetStackSize(stackSize);
    const auto mask = affinityMask == FPlatformAffinity::GetNoAffinityMask() ? AnyUnreal::Kernel::Affinity::Available() : affinityMask;
    attributes.SetAffinity(mask);
    const auto kernelName = AnyUnreal::Text::EncodeUtf8(name);
    Runnable = runnable;
    ThreadName = name;
    ThreadPriority = priority;
    ThreadAffinityMask = mask;
    thread.emplace([this] { run(); }, kernelName, &attributes);
    initialized.Wait();
    if (!initSucceeded || initFailure)
    {
        thread->Join();
        if (initFailure) std::rethrow_exception(initFailure);
        return false;
    }
    return true;
}

void FAnyUnrealRunnableThread::run()
{
    Cleanup cleanup(*this);
    try
    {
        ThreadID = FPlatformTLS::GetCurrentThreadId();
        FThreadManager::Get().AddThread(ThreadID, this);
        initSucceeded = Runnable->Init();
        if (initSucceeded)
        {
            SetTls();
            cleanup.EnableTls();
        }
    }
    catch (...)
    {
        initFailure = std::current_exception();
    }
    initialized.Trigger();
    if (!initSucceeded || initFailure) return;
    try
    {
        Runnable->Run();
    }
    catch (...)
    {
        Runnable->Exit();
        throw;
    }
    Runnable->Exit();
}

void FAnyUnrealRunnableThread::releaseTls()
{
#if STATS
    FThreadStats::Shutdown();
#endif
    FreeTls();
}

void FAnyUnrealRunnableThread::SetThreadPriority(EThreadPriority priority)
{
    const int kernelPriority = AnyUnreal::Runnable::GetKernelPriority(priority);
    if (!thread) throw std::logic_error("Runnable thread has not been created");
    AnyUnreal::Kernel::CheckResult(scePthreadSetprio(thread->Get(), kernelPriority), "scePthreadSetprio");
    ThreadPriority = priority;
}

bool FAnyUnrealRunnableThread::SetThreadAffinity(const FThreadAffinity& affinity)
{
    if (!thread) throw std::logic_error("Runnable thread has not been created");
    if (affinity.ProcessorGroup != 0) throw std::invalid_argument("libkernel does not expose processor groups");
    if (affinity.ThreadAffinityMask == 0) return false;
    const auto mask = affinity.ThreadAffinityMask == FPlatformAffinity::GetNoAffinityMask() ? AnyUnreal::Kernel::Affinity::Available() : affinity.ThreadAffinityMask;
    if (mask == ThreadAffinityMask) return false;
    AnyUnreal::Kernel::Affinity::Set(thread->Get(), mask);
    ThreadAffinityMask = mask;
    return true;
}

void FAnyUnrealRunnableThread::Suspend(bool)
{
    throw std::runtime_error("libkernel does not expose thread suspension and resumption");
}

bool FAnyUnrealRunnableThread::Kill(bool shouldWait)
{
    if (!thread || !thread->IsJoinable()) return true;
    if (!completed.load(std::memory_order_acquire)) Runnable->Stop();
    if (shouldWait) thread->Join();
    return true;
}

void FAnyUnrealRunnableThread::WaitForCompletion()
{
    if (!thread) throw std::logic_error("Runnable thread has not been created");
    thread->Join();
}
