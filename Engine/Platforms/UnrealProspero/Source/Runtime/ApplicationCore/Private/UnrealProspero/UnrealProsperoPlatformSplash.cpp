#include "UnrealProspero/UnrealProsperoPlatformSplash.h"
#include "UnrealProspero/Kernel/Result.h"
#include "prx/libSceSystemService/include/SystemService.hpp"

void FUnrealProsperoPlatformSplash::Hide()
{
    UnrealProspero::Kernel::CheckResult(sceSystemServiceHideSplashScreen(), "sceSystemServiceHideSplashScreen");
}
