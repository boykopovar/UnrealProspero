#ifndef UNREALPROSPERO_PLATFORM_GUID_H
#define UNREALPROSPERO_PLATFORM_GUID_H

#include "Misc/Guid.h"

struct FUnrealProsperoPlatformGuid
{
    static CORE_API FGuid CreateGuid();
    static CORE_API FGuid CreateGuidFromTimeAndRandom();
    static CORE_API FGuid CreateGuidFromRandomOnly();
};

#endif
