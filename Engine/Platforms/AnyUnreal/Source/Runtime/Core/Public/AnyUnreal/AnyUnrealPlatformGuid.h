#ifndef ANYUNREAL_PLATFORM_GUID_H
#define ANYUNREAL_PLATFORM_GUID_H

#include "Misc/Guid.h"

struct FAnyUnrealPlatformGuid
{
    static CORE_API FGuid CreateGuid();
    static CORE_API FGuid CreateGuidFromTimeAndRandom();
    static CORE_API FGuid CreateGuidFromRandomOnly();
};

#endif
