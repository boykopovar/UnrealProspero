#include "UnrealProspero/UnrealProsperoPlatformManualResetEvent.h"
#include "Async/Fundamental/Oversubscription.h"
#include <cmath>
#include <stdexcept>

namespace UE::HAL::Private
{
    void FUnrealProsperoManualResetEvent::Reset()
    {
        event.Reset();
    }

    bool FUnrealProsperoManualResetEvent::Poll()
    {
        return event.Wait(0);
    }

    void FUnrealProsperoManualResetEvent::Wait()
    {
        LowLevelTasks::FOversubscriptionScope scope;
        event.Wait();
    }

    bool FUnrealProsperoManualResetEvent::WaitFor(FMonotonicTimeSpan waitTime)
    {
        if (std::isnan(waitTime.ToSeconds())) throw std::invalid_argument("Manual reset event timeout is NaN");
        if (waitTime <= FMonotonicTimeSpan::Zero()) return Poll();
        if (waitTime.IsInfinity())
        {
            Wait();
            return true;
        }
        LowLevelTasks::FOversubscriptionScope scope;
        const auto start = FMonotonicTimePoint::Now();
        auto remaining = waitTime;
        for (;;)
        {
            constexpr auto maximum = UnrealProspero::Kernel::Event::Infinite - 1;
            const double milliseconds = std::ceil(remaining.ToMilliseconds());
            const auto timeout = milliseconds >= maximum ? maximum : static_cast<std::uint32_t>(milliseconds);
            if (event.Wait(timeout)) return true;
            const auto elapsed = FMonotonicTimePoint::Now() - start;
            if (elapsed < FMonotonicTimeSpan::Zero()) throw std::runtime_error("Monotonic clock moved backwards");
            if (elapsed >= waitTime) return false;
            remaining = waitTime - elapsed;
        }
    }

    bool FUnrealProsperoManualResetEvent::WaitUntil(FMonotonicTimePoint waitTime)
    {
        return WaitFor(waitTime - FMonotonicTimePoint::Now());
    }

    void FUnrealProsperoManualResetEvent::Notify()
    {
        event.Trigger();
    }
}
