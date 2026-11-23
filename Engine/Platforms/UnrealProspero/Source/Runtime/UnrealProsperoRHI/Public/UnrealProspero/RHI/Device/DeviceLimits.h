#ifndef UNREALPROSPERO_RHI_DEVICE_LIMITS_H
#define UNREALPROSPERO_RHI_DEVICE_LIMITS_H

#include "CoreTypes.h"
#include <cstdint>

namespace UnrealProspero::RHI
{
    struct DeviceLimits final
    {
        static constexpr std::uint32_t Texture2D = 16384;
        static constexpr std::uint32_t Texture3D = 2048;
    };

    UNREALPROSPERORHI_API void ConfigureDeviceLimits();
}

#endif
