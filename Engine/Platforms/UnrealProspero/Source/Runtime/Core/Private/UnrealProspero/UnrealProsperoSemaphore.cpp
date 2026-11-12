#include "UnrealProspero/UnrealProsperoSemaphore.h"
#include "UnrealProspero/Kernel/Clock.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/Semaphore/include/Semaphore.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

FUnrealProsperoSemaphore::FUnrealProsperoSemaphore(int32 initialCount, int32 maxCount)
{
    if (initialCount < 0 || maxCount <= 0 || initialCount > maxCount) throw std::invalid_argument("Invalid semaphore counts");
    UnrealProspero::Kernel::CheckResult(sceKernelCreateSema(&handle, "UnrealProsperoSemaphore", 1, initialCount, maxCount, nullptr), __func__);
}

FUnrealProsperoSemaphore::~FUnrealProsperoSemaphore() noexcept(false)
{
    UnrealProspero::Kernel::CheckResult(sceKernelDeleteSema(handle), __func__);
}

void FUnrealProsperoSemaphore::Acquire()
{
    UnrealProspero::Kernel::CheckResult(sceKernelWaitSema(handle, 1, nullptr), __func__);
}

bool FUnrealProsperoSemaphore::TryAcquire(FTimespan timeout)
{
    if (timeout.GetTicks() < 0) throw std::invalid_argument("Negative semaphore timeout");
    if (timeout.GetTicks() == 0)
    {
        const int result = sceKernelPollSema(handle, 1);
        if (result == SCE_KERNEL_ERROR_EBUSY) return false;
        UnrealProspero::Kernel::CheckResult(result, __func__);
        return true;
    }
    const std::uint64_t ticks = static_cast<std::uint64_t>(timeout.GetTicks());
    const std::uint64_t duration = ticks / 10 + (ticks % 10 != 0);
    const std::uint64_t start = UnrealProspero::Kernel::Clock::MonotonicNanoseconds() / 1000;
    std::uint64_t remaining = duration;
    for (;;)
    {
        KernelUseconds wait = static_cast<KernelUseconds>(std::min<std::uint64_t>(remaining, std::numeric_limits<KernelUseconds>::max()));
        const int result = sceKernelWaitSema(handle, 1, &wait);
        if (result == 0) return true;
        if (result != SCE_KERNEL_ERROR_ETIMEDOUT) UnrealProspero::Kernel::CheckResult(result, __func__);
        const std::uint64_t now = UnrealProspero::Kernel::Clock::MonotonicNanoseconds() / 1000;
        if (now < start) throw std::runtime_error("Monotonic clock moved backwards");
        const std::uint64_t elapsed = now - start;
        if (elapsed >= duration) return false;
        remaining = duration - elapsed;
    }
}

void FUnrealProsperoSemaphore::Release(int32 count)
{
    if (count <= 0) throw std::invalid_argument("Invalid semaphore release count");
    UnrealProspero::Kernel::CheckResult(sceKernelSignalSema(handle, count), __func__);
}
