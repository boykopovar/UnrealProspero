#include "UnrealProspero/UnrealProsperoPlatformApplicationMisc.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Windowing/Application.h"
#include "Misc/CoreDelegates.h"
#include "prx/libSceSystemService/include/SystemService.hpp"
#include "prx/libSceAgc/Misc/include/Suspend.hpp"

namespace
{
    SystemServiceStatus ReadStatus()
    {
        SystemServiceStatus status{};
        UnrealProspero::Kernel::CheckResult(sceSystemServiceGetStatus(&status), "sceSystemServiceGetStatus");
        return status;
    }

    void DispatchLifecycle(const SystemServiceStatus& status)
    {
        static bool wasBackground = false;
        static bool wasInactive = false;
        const bool background = status.is_in_background_execution;
        const bool inactive = background || status.is_system_ui_overlaid;
        const bool deactivated = inactive && !wasInactive;
        const bool enteredBackground = background && !wasBackground;
        const bool enteredForeground = !background && wasBackground;
        const bool reactivated = !inactive && wasInactive;
        wasBackground = background;
        wasInactive = inactive;

        if (deactivated) FCoreDelegates::ApplicationWillDeactivateDelegate.Broadcast();
        if (enteredBackground) FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Broadcast();
        if (enteredForeground) FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Broadcast();
        if (reactivated) FCoreDelegates::ApplicationHasReactivatedDelegate.Broadcast();
    }
}

GenericApplication* FUnrealProsperoPlatformApplicationMisc::CreateApplication()
{
    return new UnrealProspero::Windowing::Application();
}

bool FUnrealProsperoPlatformApplicationMisc::IsThisApplicationForeground()
{
    const auto status = ReadStatus();
    return !status.is_in_background_execution && !status.is_system_ui_overlaid;
}

void FUnrealProsperoPlatformApplicationMisc::PumpMessages(bool fromMainLoop)
{
    if (!fromMainLoop) return;
    DispatchLifecycle(ReadStatus());
    UnrealProspero::Kernel::CheckResult(sceAgcSuspendPoint(), "sceAgcSuspendPoint");
}
