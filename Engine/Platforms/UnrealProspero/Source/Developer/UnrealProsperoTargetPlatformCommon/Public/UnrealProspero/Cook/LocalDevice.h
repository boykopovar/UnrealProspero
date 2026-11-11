#ifndef UNREALPROSPERO_COOK_LOCAL_DEVICE_H
#define UNREALPROSPERO_COOK_LOCAL_DEVICE_H

#include "Interfaces/ITargetDevice.h"
#include "UnrealProspero/UnrealProsperoPlatformProperties.h"

class ITargetPlatformControls;

namespace UnrealProspero::Cook
{
    class UNREALPROSPEROTARGETPLATFORMCOMMON_API LocalDevice final
    {
    public:
        static ITargetDevicePtr Create(const ITargetPlatformControls& controls, EUnrealProsperoTarget target);
    };
}

#endif
