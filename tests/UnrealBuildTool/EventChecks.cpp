#include "UnrealProspero/Kernel/Event.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <array>
#include <atomic>

namespace
{
    using UnrealProspero::Kernel::Event;

    struct Waiters
    {
        Event event;
        Event ready{true};
        const unsigned count;
        const std::uint32_t timeout;
        std::atomic<unsigned> entered{0};
        std::atomic<unsigned> awakened{0};

        Waiters(bool manualReset, unsigned count, std::uint32_t timeout) : event(manualReset), count(count), timeout(timeout) {}
    };

    void* APS5_VABI waitEvent(void* argument)
    {
        auto& waiters = *static_cast<Waiters*>(argument);
        if (waiters.entered.fetch_add(1) + 1 == waiters.count) waiters.ready.Trigger();
        if (waiters.event.Wait(waiters.timeout)) waiters.awakened.fetch_add(1);
        return nullptr;
    }

    bool checkWaiters(bool manualReset, unsigned count, std::uint32_t timeout)
    {
        Waiters waiters(manualReset, count, timeout);
        std::array<Pthread, 4> threads{};
        for (unsigned index = 0; index < count; ++index) UnrealProspero::Kernel::CheckResult(scePthreadCreate(&threads[index], nullptr, waitEvent, &waiters, "UnrealProsperoEventCheck"), __func__);
        bool ready = waiters.ready.Wait(5000);
        waiters.event.Trigger();
        for (unsigned index = 0; index < count; ++index) UnrealProspero::Kernel::CheckResult(scePthreadJoin(threads[index], nullptr), __func__);
        return ready && waiters.awakened.load() == count;
    }
}

bool CheckEvents()
{
    Event automatic(false);
    Event manual(true);
    if (automatic.IsManualReset() || !manual.IsManualReset()) return false;
    if (automatic.Wait(0) || manual.Wait(0)) return false;
    automatic.Trigger();
    automatic.Trigger();
    if (!automatic.Wait(0) || automatic.Wait(0)) return false;
    manual.Trigger();
    if (!manual.Wait(0) || !manual.Wait(0)) return false;
    manual.Reset();
    if (manual.Wait(0)) return false;
    automatic.Trigger();
    automatic.Reset();
    if (automatic.Wait(0)) return false;
    std::uint64_t start = sceKernelGetProcessTime();
    if (automatic.Wait(2)) return false;
    if (sceKernelGetProcessTime() - start < 1000) return false;
    if (!checkWaiters(true, 4, 5000)) return false;
    if (!checkWaiters(false, 1, Event::Infinite - 1)) return false;
    return checkWaiters(false, 1, Event::Infinite);
}
