#include "UnrealProspero/RHI/State/SamplerState.h"
#include "UnrealProspero/RHI/Descriptor/SamplerDescriptor.h"
#include "RHIUtilities.h"
#include "RHI.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        SamplerAddress addressMode(ESamplerAddressMode mode)
        {
            switch (mode)
            {
            case AM_Wrap: return SamplerAddress::Wrap;
            case AM_Mirror: return SamplerAddress::Mirror;
            case AM_Clamp: return SamplerAddress::Clamp;
            case AM_Border: return SamplerAddress::Border;
            default: throw std::invalid_argument("Unknown sampler address mode");
            }
        }
    }

    SamplerState::SamplerState(const FSamplerStateInitializerRHI& initializer)
    {
        SamplerDescription description;
        description.AddressU = addressMode(initializer.AddressU);
        description.AddressV = addressMode(initializer.AddressV);
        description.AddressW = addressMode(initializer.AddressW);
        description.MipBias = initializer.MipBias;
        description.MinMipLevel = initializer.MinMipLevel;
        description.MaxMipLevel = initializer.MaxMipLevel;
        description.BorderColor = initializer.BorderColor;
        switch (initializer.Filter)
        {
        case SF_Point: description.Filter = SamplerFilter::Point; break;
        case SF_Bilinear: description.Filter = SamplerFilter::Linear; break;
        case SF_Trilinear: description.Filter = SamplerFilter::Linear; description.LinearMipFilter = true; break;
        case SF_AnisotropicPoint: description.Filter = SamplerFilter::Anisotropic; break;
        case SF_AnisotropicLinear: description.Filter = SamplerFilter::Anisotropic; description.LinearMipFilter = true; break;
        default: throw std::invalid_argument("Unknown sampler filter");
        }
        switch (initializer.SamplerComparisonFunction)
        {
        case SCF_Never: break;
        case SCF_Less: description.CompareLess = true; break;
        default: throw std::invalid_argument("Unknown sampler comparison function");
        }
        if (description.Filter == SamplerFilter::Anisotropic) description.MaxAnisotropy = ComputeAnisotropyRT(initializer.MaxAnisotropy);
        descriptor = CreateSamplerDescriptor(description);
    }

    const std::array<std::uint32_t, 4>& SamplerState::GetDescriptor() const { return descriptor; }
}
