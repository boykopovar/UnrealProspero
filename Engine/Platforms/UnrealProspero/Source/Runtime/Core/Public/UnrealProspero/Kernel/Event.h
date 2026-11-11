#ifndef UNREALPROSPERO_KERNEL_EVENT_H
#define UNREALPROSPERO_KERNEL_EVENT_H

#include "SceTypes.hpp"
#include <cstdint>
#include <limits>

namespace UnrealProspero::Kernel
{
    class Event final
    {
    public:
        static constexpr std::uint32_t Infinite = std::numeric_limits<std::uint32_t>::max();
        explicit Event(bool manualReset);
        ~Event() noexcept(false);
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        bool IsManualReset() const;
        void Trigger();
        void Reset();
        bool Wait(std::uint32_t milliseconds = Infinite);

    private:
        KernelEventFlag handle = nullptr;
        bool manualReset;
        std::uint32_t waitMode;
    };
}

#endif
