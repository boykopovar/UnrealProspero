#include "AnyUnreal/Process/Exit.h"
#include "AnyUnreal/AnyUnrealPlatformMisc.h"
#include <atomic>
#include <cstdlib>

namespace AnyUnreal::Process
{
    namespace
    {
        std::atomic<int> exitStatus{-1};
        static_assert(std::atomic<int>::is_always_lock_free);
    }

    void Exit::SetStatus(std::uint8_t status) noexcept
    {
        exitStatus.store(status, std::memory_order_release);
    }

    bool Exit::GetStatus(std::uint8_t* status) noexcept
    {
        const int value = exitStatus.load(std::memory_order_acquire);
        if (value < 0) return false;
        if (status != nullptr) *status = static_cast<std::uint8_t>(value);
        return true;
    }

    void Exit::Force() noexcept
    {
        std::uint8_t status = 1;
        GetStatus(&status);
        std::_Exit(status);
    }
}

bool FAnyUnrealPlatformMisc::HasOverriddenReturnCode(uint8* status)
{
    return AnyUnreal::Process::Exit::GetStatus(status);
}
