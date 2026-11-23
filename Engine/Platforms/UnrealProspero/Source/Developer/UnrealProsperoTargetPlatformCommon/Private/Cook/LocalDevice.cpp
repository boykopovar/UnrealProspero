#include "UnrealProspero/Cook/LocalDevice.h"
#include <stdexcept>

namespace UnrealProspero::Cook
{
    ITargetDevicePtr LocalDevice::Create(const ITargetPlatformControls& controls, EUnrealProsperoTarget target)
    {
        if (target != EUnrealProsperoTarget::Ps5) throw std::invalid_argument("Unknown UnrealProspero device target");
        static_cast<void>(controls);
        return nullptr;
    }
}
