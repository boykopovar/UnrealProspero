#include "AnyUnreal/RHI/State/SamplerState.h"
#include "RHIUtilities.h"
#include "RHI.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        std::uint32_t addressMode(ESamplerAddressMode mode)
        {
            switch (mode)
            {
            case AM_Wrap: return 0;
            case AM_Mirror: return 1;
            case AM_Clamp: return 2;
            case AM_Border: return 6;
            default: throw std::invalid_argument("Unknown sampler address mode");
            }
        }

        std::uint32_t borderColor(std::uint32_t color)
        {
            switch (color)
            {
            case 0: return 0;
            case 0xff000000: return 1;
            case 0xffffffff: return 2;
            default: throw std::runtime_error("Custom sampler border color tables are not implemented");
            }
        }

        std::uint32_t lod(float value)
        {
            return static_cast<std::uint32_t>(std::clamp(value, 0.0f, 4095.0f / 256.0f) * 256.0f);
        }
    }

    SamplerState::SamplerState(const FSamplerStateInitializerRHI& initializer)
    {
        if (!std::isfinite(initializer.MipBias) || !std::isfinite(initializer.MinMipLevel) || !std::isfinite(initializer.MaxMipLevel)) throw std::invalid_argument("Sampler LOD values must be finite");
        if (initializer.MinMipLevel > initializer.MaxMipLevel) throw std::invalid_argument("Sampler minimum LOD exceeds its maximum");
        if (initializer.MipBias < -32.0f || initializer.MipBias > 8191.0f / 256.0f) throw std::out_of_range("Sampler LOD bias exceeds the hardware range");
        std::uint32_t filter;
        std::uint32_t mipFilter;
        bool anisotropic = false;
        switch (initializer.Filter)
        {
        case SF_Point: filter = 0; mipFilter = 1; break;
        case SF_Bilinear: filter = 1; mipFilter = 1; break;
        case SF_Trilinear: filter = 1; mipFilter = 2; break;
        case SF_AnisotropicPoint: filter = 3; mipFilter = 1; anisotropic = true; break;
        case SF_AnisotropicLinear: filter = 3; mipFilter = 2; anisotropic = true; break;
        default: throw std::invalid_argument("Unknown sampler filter");
        }
        std::uint32_t compare;
        switch (initializer.SamplerComparisonFunction)
        {
        case SCF_Never: compare = 0; break;
        case SCF_Less: compare = 1; break;
        default: throw std::invalid_argument("Unknown sampler comparison function");
        }
        const auto anisotropy = anisotropic ? static_cast<std::uint32_t>(std::bit_width(ComputeAnisotropyRT(initializer.MaxAnisotropy)) - 1) : 0;
        const auto bias = static_cast<std::uint32_t>(static_cast<std::int32_t>(initializer.MipBias * 256.0f)) & 0x3fff;
        descriptor = {
            addressMode(initializer.AddressU) | (addressMode(initializer.AddressV) << 3) | (addressMode(initializer.AddressW) << 6) | (anisotropy << 9) | (compare << 12),
            lod(initializer.MinMipLevel) | (lod(initializer.MaxMipLevel) << 12),
            bias | (filter << 20) | (filter << 22) | (mipFilter << 26),
            borderColor(initializer.BorderColor) << 30
        };
    }

    const std::array<std::uint32_t, 4>& SamplerState::GetDescriptor() const { return descriptor; }
}
