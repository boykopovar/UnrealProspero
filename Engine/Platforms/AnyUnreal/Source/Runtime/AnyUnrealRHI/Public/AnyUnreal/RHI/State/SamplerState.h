#ifndef ANYUNREAL_RHI_SAMPLER_STATE_H
#define ANYUNREAL_RHI_SAMPLER_STATE_H

#include "RHIResources.h"
#include <array>
#include <cstdint>

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API SamplerState final : public FRHISamplerState
    {
    public:
        explicit SamplerState(const FSamplerStateInitializerRHI& initializer);
        const std::array<std::uint32_t, 4>& GetDescriptor() const;

    private:
        std::array<std::uint32_t, 4> descriptor;
    };
}

#endif
