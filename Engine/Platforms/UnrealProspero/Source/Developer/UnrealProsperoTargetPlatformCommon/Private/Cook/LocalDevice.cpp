#include "UnrealProspero/Cook/LocalDevice.h"
#if PLATFORM_WINDOWS
#include "LocalPcTargetDevice.h"
#elif PLATFORM_LINUX
#include "LinuxTargetDevice.h"
#endif
#include <stdexcept>

namespace UnrealProspero::Cook
{
    ITargetDevicePtr LocalDevice::Create(const ITargetPlatformControls& controls, EUnrealProsperoTarget target)
    {
        if (target != EUnrealProsperoTarget::Windows && target != EUnrealProsperoTarget::Linux && target != EUnrealProsperoTarget::Ps5) throw std::invalid_argument("Unknown UnrealProspero device target");
#if PLATFORM_WINDOWS
        if (target == EUnrealProsperoTarget::Windows) return MakeShared<FLocalPcTargetDevice, ESPMode::ThreadSafe>(controls);
#elif PLATFORM_LINUX
        if (target == EUnrealProsperoTarget::Linux) return MakeShared<FLinuxTargetDevice, ESPMode::ThreadSafe>(controls, FPlatformProcess::ComputerName(), nullptr);
#else
        static_cast<void>(controls);
#endif
        return nullptr;
    }
}
