#ifndef UNREALPROSPERO_RHI_SAMPLER_STATE_H
#define UNREALPROSPERO_RHI_SAMPLER_STATE_H

#include "RHIResources.h"
#include <array>
#include <cstdint>

namespace UnrealProspero::RHI
{
    class UNREALPROSPERORHI_API SamplerState final : public FRHISamplerState
    {
    public:
        explicit SamplerState(const FSamplerStateInitializerRHI& initializer);
        const std::array<std::uint32_t, 4>& GetDescriptor() const;

    private:
        std::array<std::uint32_t, 4> descriptor;
    };
}

#endif
