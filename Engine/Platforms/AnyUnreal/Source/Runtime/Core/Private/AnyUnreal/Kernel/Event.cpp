#include "AnyUnreal/Kernel/Event.h"
#include "AnyUnreal/Kernel/Clock.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include <algorithm>

namespace AnyUnreal::Kernel
{
    namespace
    {
        constexpr std::uint32_t matchAll = 0x01;
        constexpr std::uint32_t clearMatched = 0x20;
    }

    Event::Event(bool manualReset) : manualReset(manualReset), waitMode(matchAll | (manualReset ? 0 : clearMatched))
    {
        constexpr std::uint32_t multipleWaitersFifo = 0x21;
        CheckResult(sceKernelCreateEventFlag(&handle, "AnyUnrealEvent", multipleWaitersFifo, 0, nullptr), __func__);
    }

    Event::~Event() noexcept(false)
    {
        CheckResult(sceKernelDeleteEventFlag(handle), __func__);
    }

    bool Event::IsManualReset() const
    {
        return manualReset;
    }

    void Event::Trigger()
    {
        CheckResult(sceKernelSetEventFlag(handle, 1), __func__);
    }

    void Event::Reset()
    {
        CheckResult(sceKernelClearEventFlag(handle, 0), __func__);
    }

    bool Event::Wait(std::uint32_t milliseconds)
    {
        if (milliseconds == 0)
        {
            int result = sceKernelPollEventFlag(handle, 1, waitMode, nullptr);
            if (result == SCE_KERNEL_ERROR_EBUSY) return false;
            CheckResult(result, __func__);
            return true;
        }
        if (milliseconds == Infinite)
        {
            CheckResult(sceKernelWaitEventFlag(handle, 1, waitMode, nullptr, nullptr), __func__);
            return true;
        }
        const std::uint64_t duration = static_cast<std::uint64_t>(milliseconds) * 1000;
        const std::uint64_t start = Clock::MonotonicNanoseconds() / 1000;
        std::uint64_t remaining = duration;
        for (;;)
        {
            KernelUseconds timeout = static_cast<KernelUseconds>(std::min<std::uint64_t>(remaining, std::numeric_limits<KernelUseconds>::max()));
            int result = sceKernelWaitEventFlag(handle, 1, waitMode, nullptr, &timeout);
            if (result == 0) return true;
            if (result != SCE_KERNEL_ERROR_ETIMEDOUT) CheckResult(result, __func__);
            const std::uint64_t now = Clock::MonotonicNanoseconds() / 1000;
            if (now < start) throw std::runtime_error("Monotonic clock moved backwards");
            const std::uint64_t elapsed = now - start;
            if (elapsed >= duration) return false;
            remaining = duration - elapsed;
        }
    }
}
