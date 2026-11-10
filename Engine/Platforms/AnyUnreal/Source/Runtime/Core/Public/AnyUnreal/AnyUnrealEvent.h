#ifndef ANYUNREAL_EVENT_H
#define ANYUNREAL_EVENT_H

#include "HAL/Event.h"
#include "AnyUnreal/Kernel/Event.h"
#include <optional>

class FAnyUnrealEvent final : public FEvent
{
public:
    FAnyUnrealEvent() = default;
    ~FAnyUnrealEvent() noexcept override = default;
    FAnyUnrealEvent(const FAnyUnrealEvent&) = delete;
    FAnyUnrealEvent& operator=(const FAnyUnrealEvent&) = delete;
    using FEvent::Wait;
    CORE_API bool Create(bool manualReset = false) override;
    CORE_API bool IsManualReset() override;
    CORE_API void Trigger() override;
    CORE_API void Reset() override;
    CORE_API bool Wait(uint32 milliseconds, bool ignoreThreadIdleStats = false) override;

private:
    std::optional<AnyUnreal::Kernel::Event> event;
    AnyUnreal::Kernel::Event& getEvent();
};

#endif
