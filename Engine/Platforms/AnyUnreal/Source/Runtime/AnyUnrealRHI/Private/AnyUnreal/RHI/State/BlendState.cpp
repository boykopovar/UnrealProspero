#include "AnyUnreal/RHI/State/BlendState.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        uint32 operation(EBlendOperation value)
        {
            switch (value)
            {
            case BO_Add: return 0;
            case BO_Subtract: return 1;
            case BO_Min: return 2;
            case BO_Max: return 3;
            case BO_ReverseSubtract: return 4;
            default: throw std::invalid_argument("Unknown blend operation");
            }
        }

        uint32 factor(EBlendFactor value)
        {
            switch (value)
            {
            case BF_Zero: return 0;
            case BF_One: return 1;
            case BF_SourceColor: return 2;
            case BF_InverseSourceColor: return 3;
            case BF_SourceAlpha: return 4;
            case BF_InverseSourceAlpha: return 5;
            case BF_DestAlpha: return 6;
            case BF_InverseDestAlpha: return 7;
            case BF_DestColor: return 8;
            case BF_InverseDestColor: return 9;
            case BF_ConstantBlendFactor: return 13;
            case BF_InverseConstantBlendFactor: return 14;
            case BF_Source1Color: return 15;
            case BF_InverseSource1Color: return 16;
            case BF_Source1Alpha: return 17;
            case BF_InverseSource1Alpha: return 18;
            default: throw std::invalid_argument("Unknown blend factor");
            }
        }
    }

    BlendState::BlendState(const FBlendStateInitializerRHI& initializer) : description(initializer)
    {
        for (const auto& target : description.RenderTargets)
        {
            operation(target.ColorBlendOp);
            operation(target.AlphaBlendOp);
            factor(target.ColorSrcBlend);
            factor(target.ColorDestBlend);
            factor(target.AlphaSrcBlend);
            factor(target.AlphaDestBlend);
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
    uint32 BlendState::GetControl(uint32 index) const
    {
        const auto& target = GetRenderTarget(index);
        const bool enabled = target.ColorBlendOp != BO_Add || target.ColorSrcBlend != BF_One || target.ColorDestBlend != BF_Zero || target.AlphaBlendOp != BO_Add || target.AlphaSrcBlend != BF_One || target.AlphaDestBlend != BF_Zero;
        return factor(target.ColorSrcBlend) | (operation(target.ColorBlendOp) << 5) | (factor(target.ColorDestBlend) << 8) | (factor(target.AlphaSrcBlend) << 16) | (operation(target.AlphaBlendOp) << 21) | (factor(target.AlphaDestBlend) << 24) | (1u << 29) | (static_cast<uint32>(enabled) << 30);
    }

    uint32 BlendState::GetTargetMask(uint32 targetCount) const
    {
        static_assert(MaxSimultaneousRenderTargets == 8);
        if (targetCount > MaxSimultaneousRenderTargets) throw std::out_of_range("Blend render target count exceeds the hardware limit");
        uint32 mask = 0;
        for (uint32 index = 0; index < targetCount; ++index) mask |= static_cast<uint32>(GetRenderTarget(index).ColorWriteMask.GetValue()) << (index * 4);
        return mask;
    }

}
