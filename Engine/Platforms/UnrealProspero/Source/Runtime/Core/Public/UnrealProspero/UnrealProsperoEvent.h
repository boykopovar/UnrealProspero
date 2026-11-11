#ifndef UNREALPROSPERO_EVENT_H
#define UNREALPROSPERO_EVENT_H

#include "HAL/Event.h"
#include "UnrealProspero/Kernel/Event.h"
#include <optional>

class FUnrealProsperoEvent final : public FEvent
{
public:
    FUnrealProsperoEvent() = default;
    ~FUnrealProsperoEvent() noexcept override = default;
    FUnrealProsperoEvent(const FUnrealProsperoEvent&) = delete;
    FUnrealProsperoEvent& operator=(const FUnrealProsperoEvent&) = delete;
    using FEvent::Wait;
    CORE_API bool Create(bool manualReset = false) override;
    CORE_API bool IsManualReset() override;
    CORE_API void Trigger() override;
    CORE_API void Reset() override;
    CORE_API bool Wait(uint32 milliseconds, bool ignoreThreadIdleStats = false) override;

private:
    std::optional<UnrealProspero::Kernel::Event> event;
    UnrealProspero::Kernel::Event& getEvent();
};

#endif
