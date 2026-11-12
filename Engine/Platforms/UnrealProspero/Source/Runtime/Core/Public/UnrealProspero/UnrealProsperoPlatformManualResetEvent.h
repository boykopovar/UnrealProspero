#ifndef UNREALPROSPERO_PLATFORM_MANUAL_RESET_EVENT_H
#define UNREALPROSPERO_PLATFORM_MANUAL_RESET_EVENT_H

#include "UnrealProspero/Kernel/Event.h"
#include "Misc/MonotonicTime.h"

namespace UE::HAL::Private
{
    class FUnrealProsperoManualResetEvent final
    {
    public:
        FUnrealProsperoManualResetEvent() = default;
        FUnrealProsperoManualResetEvent(const FUnrealProsperoManualResetEvent&) = delete;
        FUnrealProsperoManualResetEvent& operator=(const FUnrealProsperoManualResetEvent&) = delete;
        CORE_API void Reset();
        CORE_API bool Poll();
        CORE_API void Wait();
        CORE_API bool WaitFor(FMonotonicTimeSpan waitTime);
        CORE_API bool WaitUntil(FMonotonicTimePoint waitTime);
        CORE_API void Notify();

    private:
        UnrealProspero::Kernel::Event event{true};
    };
}

namespace UE
{
    using FPlatformManualResetEvent = HAL::Private::FUnrealProsperoManualResetEvent;
}

#endif
