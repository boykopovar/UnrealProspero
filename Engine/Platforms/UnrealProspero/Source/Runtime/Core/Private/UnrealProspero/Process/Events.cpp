#include "UnrealProspero/UnrealProsperoPlatformProcess.h"
#include "UnrealProspero/UnrealProsperoEvent.h"
#include <memory>

FEvent* FUnrealProsperoPlatformProcess::CreateSynchEvent(bool manualReset)
{
    auto event = std::make_unique<FUnrealProsperoEvent>();
    event->Create(manualReset);
    return event.release();
}
