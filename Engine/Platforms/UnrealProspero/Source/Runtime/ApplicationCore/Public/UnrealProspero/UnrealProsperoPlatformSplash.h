#ifndef UNREALPROSPERO_PLATFORM_SPLASH_H
#define UNREALPROSPERO_PLATFORM_SPLASH_H

#include "GenericPlatform/GenericPlatformSplash.h"

struct FUnrealProsperoPlatformSplash : FGenericPlatformSplash
{
    static APPLICATIONCORE_API void Hide();
};

using FPlatformSplash = FUnrealProsperoPlatformSplash;

#endif
