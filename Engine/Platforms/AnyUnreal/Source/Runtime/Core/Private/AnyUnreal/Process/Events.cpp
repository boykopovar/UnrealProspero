#include "AnyUnreal/AnyUnrealPlatformProcess.h"
#include "AnyUnreal/AnyUnrealEvent.h"
#include <memory>

FEvent* FAnyUnrealPlatformProcess::CreateSynchEvent(bool manualReset)
{
    auto event = std::make_unique<FAnyUnrealEvent>();
    event->Create(manualReset);
    return event.release();
}
