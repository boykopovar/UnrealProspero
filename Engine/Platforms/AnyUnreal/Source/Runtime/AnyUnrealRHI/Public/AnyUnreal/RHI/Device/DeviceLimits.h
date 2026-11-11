#ifndef ANYUNREAL_RHI_DEVICE_LIMITS_H
#define ANYUNREAL_RHI_DEVICE_LIMITS_H

#include <cstdint>

namespace AnyUnreal::RHI
{
    struct DeviceLimits final
    {
        static constexpr std::uint32_t Texture2D = 16384;
        static constexpr std::uint32_t Texture3D = 2048;
    };

    ANYUNREALRHI_API void ConfigureDeviceLimits();
}

#endif
