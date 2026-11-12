#include "UnrealProspero/Process/Exit.h"
#include "UnrealProspero/UnrealProsperoPlatformMisc.h"
#include <atomic>
#include <cstdlib>
#if UNREALPROSPERO_TARGET_PS5
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceSystemService/libSceSystemService.hpp"
#include "prx/libkernel/include/Api.hpp"
#include <cstdio>
#endif

namespace UnrealProspero::Process
{
    namespace
    {
        std::atomic<int> exitStatus{-1};
        static_assert(std::atomic<int>::is_always_lock_free);
#if UNREALPROSPERO_TARGET_PS5
        [[noreturn]] void requestSystemExit(int status)
        {
            char message[80];
            const int length = std::snprintf(message, sizeof(message), "UnrealProspero exit status: %d\n", status);
            if (length < 0 || static_cast<std::size_t>(length) >= sizeof(message)) throw std::runtime_error("Cannot format application exit status");
            Kernel::CheckResult(sceKernelDebugOutText(0, message), "sceKernelDebugOutText");
            Kernel::CheckResult(sceSystemServiceLoadExec("exit", nullptr), "sceSystemServiceLoadExec");
            for (;;) sceKernelSleep(1);
        }
#endif
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

    int Exit::Complete(int status)
    {
#if UNREALPROSPERO_TARGET_PS5
        requestSystemExit(status);
#else
        return status;
#endif
    }

    void Exit::Force()
    {
        std::uint8_t status = 1;
        GetStatus(&status);
#if UNREALPROSPERO_TARGET_PS5
        requestSystemExit(status);
#else
        std::_Exit(status);
#endif
    }
}

bool FUnrealProsperoPlatformMisc::HasOverriddenReturnCode(uint8* status)
{
    return UnrealProspero::Process::Exit::GetStatus(status);
}
