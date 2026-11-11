#include "UnrealProspero/RHI/Descriptor/SamplerDescriptor.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::uint32_t addressMode(SamplerAddress mode)
        {
            switch (mode)
            {
            case SamplerAddress::Wrap: return 0;
            case SamplerAddress::Mirror: return 1;
            case SamplerAddress::Clamp: return 2;
            case SamplerAddress::Border: return 6;
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

    std::array<std::uint32_t, 4> CreateSamplerDescriptor(const SamplerDescription& initializer)
    {
        if (!std::isfinite(initializer.MipBias) || !std::isfinite(initializer.MinMipLevel) || !std::isfinite(initializer.MaxMipLevel)) throw std::invalid_argument("Sampler LOD values must be finite");
        if (initializer.MinMipLevel > initializer.MaxMipLevel) throw std::invalid_argument("Sampler minimum LOD exceeds its maximum");
        if (initializer.MipBias < -32.0f || initializer.MipBias > 8191.0f / 256.0f) throw std::out_of_range("Sampler LOD bias exceeds the hardware range");
        std::uint32_t filter;
        switch (initializer.Filter)
        {
        case SamplerFilter::Point: filter = 0; break;
        case SamplerFilter::Linear: filter = 1; break;
        case SamplerFilter::Anisotropic: filter = 3; break;
        default: throw std::invalid_argument("Unknown sampler filter");
        }
        if (initializer.MaxAnisotropy < 1 || initializer.MaxAnisotropy > 16) throw std::out_of_range("Sampler anisotropy exceeds the hardware range");
        const auto mipFilter = initializer.LinearMipFilter ? 2u : 1u;
        const auto compare = initializer.CompareLess ? 1u : 0u;
        const auto anisotropy = initializer.Filter == SamplerFilter::Anisotropic ? static_cast<std::uint32_t>(std::bit_width(initializer.MaxAnisotropy) - 1) : 0;
        const auto bias = static_cast<std::uint32_t>(static_cast<std::int32_t>(initializer.MipBias * 256.0f)) & 0x3fff;
        return {
            addressMode(initializer.AddressU) | (addressMode(initializer.AddressV) << 3) | (addressMode(initializer.AddressW) << 6) | (anisotropy << 9) | (compare << 12),
            lod(initializer.MinMipLevel) | (lod(initializer.MaxMipLevel) << 12),
            bias | (filter << 20) | (filter << 22) | (mipFilter << 26),
            borderColor(initializer.BorderColor) << 30
        };
    }
}
