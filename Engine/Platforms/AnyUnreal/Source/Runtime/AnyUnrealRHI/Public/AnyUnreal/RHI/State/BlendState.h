#ifndef ANYUNREAL_RHI_BLEND_STATE_H
#define ANYUNREAL_RHI_BLEND_STATE_H

#include "RHIResources.h"
#include "RHI.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API BlendState final : public FRHIBlendState
    {
    public:
        explicit BlendState(const FBlendStateInitializerRHI& initializer);
        bool GetInitializer(FBlendStateInitializerRHI& result) override;
        const FBlendStateInitializerRHI& GetDescription() const;
        uint32 GetControl(uint32 index) const;
        uint32 GetTargetMask(uint32 targetCount) const;
        const FBlendStateInitializerRHI::FRenderTarget& GetRenderTarget(uint32 index) const;

    private:
        FBlendStateInitializerRHI description;
    };
}

#endif
