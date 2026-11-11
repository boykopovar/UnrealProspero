#ifndef ANYUNREAL_PLATFORM_APPLICATION_MISC_H
#define ANYUNREAL_PLATFORM_APPLICATION_MISC_H

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct FAnyUnrealPlatformApplicationMisc : FGenericPlatformApplicationMisc
{
    static GenericApplication* CreateApplication();
    static bool IsThisApplicationForeground();
    static void PumpMessages(bool fromMainLoop);
};

using FPlatformApplicationMisc = FAnyUnrealPlatformApplicationMisc;

#endif
