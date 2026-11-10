#include "AnyUnreal/AnyUnrealPlatformApplicationMisc.h"
#include "AnyUnreal/Kernel/Result.h"
#include "prx/libSceSystemService/include/SystemService.hpp"
#include "prx/libSceAgc/Misc/include/Suspend.hpp"

bool FAnyUnrealPlatformApplicationMisc::IsThisApplicationForeground()
{
    SystemServiceStatus status{};
    AnyUnreal::Kernel::CheckResult(sceSystemServiceGetStatus(&status), "sceSystemServiceGetStatus");
    return !status.is_in_background_execution && !status.is_system_ui_overlaid;
}

void FAnyUnrealPlatformApplicationMisc::PumpMessages(bool fromMainLoop)
{
    if (fromMainLoop) AnyUnreal::Kernel::CheckResult(sceAgcSuspendPoint(), "sceAgcSuspendPoint");
}
