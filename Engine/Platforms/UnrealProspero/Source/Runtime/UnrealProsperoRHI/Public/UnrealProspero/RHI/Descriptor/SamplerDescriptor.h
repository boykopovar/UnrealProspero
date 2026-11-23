#ifndef UNREALPROSPERO_RHI_SAMPLER_DESCRIPTOR_H
#define UNREALPROSPERO_RHI_SAMPLER_DESCRIPTOR_H

#include "CoreTypes.h"
#include <array>
#include <cstdint>

namespace UnrealProspero::RHI
{
    enum class SamplerAddress { Wrap, Mirror, Clamp, Border };
    enum class SamplerFilter { Point, Linear, Anisotropic };

    struct SamplerDescription
    {
        SamplerAddress AddressU = SamplerAddress::Wrap;
        SamplerAddress AddressV = SamplerAddress::Wrap;
        SamplerAddress AddressW = SamplerAddress::Wrap;
        SamplerFilter Filter = SamplerFilter::Point;
        bool LinearMipFilter = false;
        bool CompareLess = false;
        std::uint32_t MaxAnisotropy = 1;
        float MipBias = 0;
        float MinMipLevel = 0;
        float MaxMipLevel = 16;
        std::uint32_t BorderColor = 0;
    };

    UNREALPROSPERORHI_API std::array<std::uint32_t, 4> CreateSamplerDescriptor(const SamplerDescription& description);
}

#endif
