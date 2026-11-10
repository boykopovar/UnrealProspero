#include "AnyUnreal/AnyUnrealEvent.h"
#include "Stats/ThreadIdleStats.h"
#include <stdexcept>

bool FAnyUnrealEvent::Create(bool manualReset)
{
    if (event.has_value()) throw std::logic_error("Event is already created");
    event.emplace(manualReset);
    return true;
}

AnyUnreal::Kernel::Event& FAnyUnrealEvent::getEvent()
{
    if (!event.has_value()) throw std::logic_error("Event has not been created");
    return *event;
}

bool FAnyUnrealEvent::IsManualReset()
{
    return getEvent().IsManualReset();
}

void FAnyUnrealEvent::Trigger()
{
    auto& value = getEvent();
    TriggerForStats();
    value.Trigger();
}

void FAnyUnrealEvent::Reset()
{
    auto& value = getEvent();
    ResetForStats();
    value.Reset();
}

bool FAnyUnrealEvent::Wait(uint32 milliseconds, bool ignoreThreadIdleStats)
{
    auto& value = getEvent();
    WaitForStats();
    UE::Stats::FThreadIdleStats::FScopeIdle scope(ignoreThreadIdleStats);
    return value.Wait(milliseconds);
}
