#include "UnrealProspero/UnrealProsperoPlatformApplicationMisc.h"
#include "UnrealProspero/Kernel/Result.h"
#include "UnrealProspero/Windowing/Application.h"
#include "prx/libSceSystemService/include/SystemService.hpp"
#include "prx/libSceAgc/Misc/include/Suspend.hpp"

GenericApplication* FUnrealProsperoPlatformApplicationMisc::CreateApplication()
{
    return new UnrealProspero::Windowing::Application();
}

bool FUnrealProsperoPlatformApplicationMisc::IsThisApplicationForeground()
{
    SystemServiceStatus status{};
    UnrealProspero::Kernel::CheckResult(sceSystemServiceGetStatus(&status), "sceSystemServiceGetStatus");
    return !status.is_in_background_execution && !status.is_system_ui_overlaid;
}

void FUnrealProsperoPlatformApplicationMisc::PumpMessages(bool fromMainLoop)
{
    if (fromMainLoop) UnrealProspero::Kernel::CheckResult(sceAgcSuspendPoint(), "sceAgcSuspendPoint");
}
