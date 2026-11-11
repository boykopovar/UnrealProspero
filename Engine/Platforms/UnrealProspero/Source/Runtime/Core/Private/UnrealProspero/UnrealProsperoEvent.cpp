#include "UnrealProspero/UnrealProsperoEvent.h"
#include "Stats/ThreadIdleStats.h"
#include <stdexcept>

bool FUnrealProsperoEvent::Create(bool manualReset)
{
    if (event.has_value()) throw std::logic_error("Event is already created");
    event.emplace(manualReset);
    return true;
}

UnrealProspero::Kernel::Event& FUnrealProsperoEvent::getEvent()
{
    if (!event.has_value()) throw std::logic_error("Event has not been created");
    return *event;
}

bool FUnrealProsperoEvent::IsManualReset()
{
    return getEvent().IsManualReset();
}

void FUnrealProsperoEvent::Trigger()
{
    auto& value = getEvent();
    TriggerForStats();
    value.Trigger();
}

void FUnrealProsperoEvent::Reset()
{
    auto& value = getEvent();
    ResetForStats();
    value.Reset();
}

bool FUnrealProsperoEvent::Wait(uint32 milliseconds, bool ignoreThreadIdleStats)
{
    auto& value = getEvent();
    WaitForStats();
    UE::Stats::FThreadIdleStats::FScopeIdle scope(ignoreThreadIdleStats);
    return value.Wait(milliseconds);
}
