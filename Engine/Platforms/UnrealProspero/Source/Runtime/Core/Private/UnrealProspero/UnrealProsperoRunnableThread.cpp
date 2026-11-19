#include "UnrealProspero/UnrealProsperoRunnableThread.h"
#include "UnrealProspero/Kernel/Affinity.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Runnable/Priority.h"
#include "UnrealProspero/Text/Encoding.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadManager.h"
#include "prx/libkernel/include/Api.hpp"
#include "Stats/StatsSystemTypes.h"
#include <stdexcept>

class FUnrealProsperoRunnableThread::Cleanup final
{
public:
    explicit Cleanup(FUnrealProsperoRunnableThread& owner) : owner(owner) {}
    ~Cleanup() noexcept(false)
    {
        if (tls) owner.releaseTls();
        owner.completed.store(true, std::memory_order_release);
    }
    void EnableTls() noexcept { tls = true; }

private:
    FUnrealProsperoRunnableThread& owner;
    bool tls = false;
};

FUnrealProsperoRunnableThread::~FUnrealProsperoRunnableThread() noexcept
{
    Kill(true);
}

bool FUnrealProsperoRunnableThread::CreateInternal(FRunnable* runnable, const TCHAR* name, uint32 stackSize, EThreadPriority priority, uint64 affinityMask, EThreadCreateFlags flags)
{
    if (thread) throw std::logic_error("Runnable thread has already been created");
    if (!runnable || !name) throw std::invalid_argument("Runnable thread requires a runnable and a name");
    if (flags != EThreadCreateFlags::None && flags != EThreadCreateFlags::SMTExclusive) throw std::invalid_argument("Unknown thread creation flags");
    UnrealProspero::Kernel::ThreadAttributes attributes;
    attributes.SetPriority(UnrealProspero::Runnable::GetKernelPriority(priority));
    attributes.SetExclusiveScheduling(flags == EThreadCreateFlags::SMTExclusive);
    constexpr uint32 defaultStackSize = 2 * 1024 * 1024;
    attributes.SetStackSize(stackSize == 0 ? defaultStackSize : stackSize);
    const auto mask = affinityMask == FPlatformAffinity::GetNoAffinityMask() ? UnrealProspero::Kernel::Affinity::Available() : affinityMask;
    attributes.SetAffinity(mask);
    auto kernelName = UnrealProspero::Text::EncodeUtf8(name);
    constexpr std::size_t maxKernelNameBytes = 31;
    if (kernelName.size() > maxKernelNameBytes)
    {
        auto end = maxKernelNameBytes;
        while ((static_cast<unsigned char>(kernelName[end]) & 0xc0) == 0x80) --end;
        kernelName.resize(end);
    }
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

void FUnrealProsperoRunnableThread::run()
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

void FUnrealProsperoRunnableThread::releaseTls()
{
#if STATS
    FThreadStats::Shutdown();
#endif
    FreeTls();
}

void FUnrealProsperoRunnableThread::SetThreadPriority(EThreadPriority priority)
{
    const int kernelPriority = UnrealProspero::Runnable::GetKernelPriority(priority);
    if (!thread) throw std::logic_error("Runnable thread has not been created");
    UnrealProspero::Kernel::CheckResult(scePthreadSetprio(thread->Get(), kernelPriority), "scePthreadSetprio");
    ThreadPriority = priority;
}

bool FUnrealProsperoRunnableThread::SetThreadAffinity(const FThreadAffinity& affinity)
{
    if (!thread) throw std::logic_error("Runnable thread has not been created");
    if (affinity.ProcessorGroup != 0) throw std::invalid_argument("libkernel does not expose processor groups");
    if (affinity.ThreadAffinityMask == 0) return false;
    const auto mask = affinity.ThreadAffinityMask == FPlatformAffinity::GetNoAffinityMask() ? UnrealProspero::Kernel::Affinity::Available() : affinity.ThreadAffinityMask;
    if (mask == ThreadAffinityMask) return false;
    UnrealProspero::Kernel::Affinity::Set(thread->Get(), mask);
    ThreadAffinityMask = mask;
    return true;
}

void FUnrealProsperoRunnableThread::Suspend(bool)
{
    throw std::runtime_error("libkernel does not expose thread suspension and resumption");
}

bool FUnrealProsperoRunnableThread::Kill(bool shouldWait)
{
    if (!thread || !thread->IsJoinable()) return true;
    if (!completed.load(std::memory_order_acquire)) Runnable->Stop();
    if (shouldWait) thread->Join();
    return true;
}

void FUnrealProsperoRunnableThread::WaitForCompletion()
{
    if (!thread) throw std::logic_error("Runnable thread has not been created");
    thread->Join();
}
