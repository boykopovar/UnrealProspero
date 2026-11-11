#include "AnyUnreal/RHI/State/BlendState.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        void checkOperation(EBlendOperation operation)
        {
            switch (operation)
            {
            case BO_Add:
            case BO_Subtract:
            case BO_Min:
            case BO_Max:
            case BO_ReverseSubtract: break;
            default: throw std::invalid_argument("Unknown blend operation");
            }
        }

        void checkFactor(EBlendFactor factor)
        {
            switch (factor)
            {
            case BF_Zero:
            case BF_One:
            case BF_SourceColor:
            case BF_InverseSourceColor:
            case BF_SourceAlpha:
            case BF_InverseSourceAlpha:
            case BF_DestAlpha:
            case BF_InverseDestAlpha:
            case BF_DestColor:
            case BF_InverseDestColor:
            case BF_ConstantBlendFactor:
            case BF_InverseConstantBlendFactor:
            case BF_Source1Color:
            case BF_InverseSource1Color:
            case BF_Source1Alpha:
            case BF_InverseSource1Alpha: break;
            default: throw std::invalid_argument("Unknown blend factor");
            }
        }
    }

    BlendState::BlendState(const FBlendStateInitializerRHI& initializer) : description(initializer)
    {
        for (const auto& target : description.RenderTargets)
        {
            checkOperation(target.ColorBlendOp);
            checkOperation(target.AlphaBlendOp);
            checkFactor(target.ColorSrcBlend);
            checkFactor(target.ColorDestBlend);
            checkFactor(target.AlphaSrcBlend);
            checkFactor(target.AlphaDestBlend);
            if ((static_cast<uint32>(target.ColorWriteMask.GetValue()) & ~static_cast<uint32>(CW_RGBA)) != 0) throw std::invalid_argument("Color write mask contains unknown channels");
        }
    }

    bool BlendState::GetInitializer(FBlendStateInitializerRHI& result)
    {
        result = description;
        return true;
    }

    const FBlendStateInitializerRHI& BlendState::GetDescription() const { return description; }

    const FBlendStateInitializerRHI::FRenderTarget& BlendState::GetRenderTarget(uint32 index) const
    {
        if (index >= MaxSimultaneousRenderTargets) throw std::out_of_range("Blend render target index exceeds the RHI limit");
        return description.RenderTargets[description.bUseIndependentRenderTargetBlendStates ? index : 0];
    }
}
