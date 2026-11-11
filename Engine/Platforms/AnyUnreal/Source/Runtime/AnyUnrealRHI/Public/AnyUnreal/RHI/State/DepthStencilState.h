#ifndef ANYUNREAL_RHI_DEPTH_STENCIL_STATE_H
#define ANYUNREAL_RHI_DEPTH_STENCIL_STATE_H

#include "RHIResources.h"
#include "RHI.h"

namespace AnyUnreal::RHI
{
    class ANYUNREALRHI_API DepthStencilState final : public FRHIDepthStencilState
    {
    public:
        explicit DepthStencilState(const FDepthStencilStateInitializerRHI& initializer);
        bool GetInitializer(FDepthStencilStateInitializerRHI& result) override;
        const FDepthStencilStateInitializerRHI& GetDescription() const;

    private:
        FDepthStencilStateInitializerRHI description;
    };
}

#endif
