#ifndef ANYUNREAL_RHI_RASTERIZER_STATE_H
#define ANYUNREAL_RHI_RASTERIZER_STATE_H

#include "RHIResources.h"
#include "RHI.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API RasterizerState final : public FRHIRasterizerState
    {
    public:
        explicit RasterizerState(const FRasterizerStateInitializerRHI& initializer);
        bool GetInitializer(FRasterizerStateInitializerRHI& result) override;
        const FRasterizerStateInitializerRHI& GetDescription() const;
        uint32 UpdateModeControl(uint32 value, bool depthBias) const;
        uint32 UpdateClipControl(uint32 value) const;

    private:
        FRasterizerStateInitializerRHI description;
    };
}

#endif
