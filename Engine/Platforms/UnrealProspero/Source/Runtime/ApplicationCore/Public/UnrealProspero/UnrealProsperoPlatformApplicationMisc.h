#ifndef UNREALPROSPERO_PLATFORM_APPLICATION_MISC_H
#define UNREALPROSPERO_PLATFORM_APPLICATION_MISC_H

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct FUnrealProsperoPlatformApplicationMisc : FGenericPlatformApplicationMisc
{
    static GenericApplication* CreateApplication();
    static bool IsThisApplicationForeground();
    static void PumpMessages(bool fromMainLoop);
};

using FPlatformApplicationMisc = FUnrealProsperoPlatformApplicationMisc;

#endif
