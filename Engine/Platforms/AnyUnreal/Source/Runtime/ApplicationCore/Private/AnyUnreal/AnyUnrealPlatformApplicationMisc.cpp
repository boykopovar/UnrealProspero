#include "AnyUnreal/AnyUnrealPlatformApplicationMisc.h"
#include "AnyUnreal/Kernel/Result.h"
#include "AnyUnreal/Windowing/Application.h"
#include "prx/libSceSystemService/include/SystemService.hpp"
#include "prx/libSceAgc/Misc/include/Suspend.hpp"

GenericApplication* FAnyUnrealPlatformApplicationMisc::CreateApplication()
{
    return new AnyUnreal::Windowing::Application();
}

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
