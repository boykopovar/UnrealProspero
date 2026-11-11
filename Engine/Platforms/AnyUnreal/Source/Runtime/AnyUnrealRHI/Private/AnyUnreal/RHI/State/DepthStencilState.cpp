#include "AnyUnreal/RHI/State/DepthStencilState.h"
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        uint32 comparison(ECompareFunction function)
        {
            switch (function)
            {
            case CF_Never: return 0;
            case CF_Less: return 1;
            case CF_Equal: return 2;
            case CF_LessEqual: return 3;
            case CF_Greater: return 4;
            case CF_NotEqual: return 5;
            case CF_GreaterEqual: return 6;
            case CF_Always: return 7;
            default: throw std::invalid_argument("Unknown depth or stencil comparison function");
            }
        }

        uint32 stencilOperation(EStencilOp operation)
        {
            switch (operation)
            {
            case SO_Keep: return 0;
            case SO_Zero: return 1;
            case SO_Replace: return 3;
            case SO_SaturatedIncrement: return 5;
            case SO_SaturatedDecrement: return 6;
            case SO_Invert: return 7;
            case SO_Increment: return 8;
            case SO_Decrement: return 9;
            default: throw std::invalid_argument("Unknown stencil operation");
            }
        }
    }

    DepthStencilState::DepthStencilState(const FDepthStencilStateInitializerRHI& initializer) : description(initializer)
    {
        comparison(description.DepthTest);
        comparison(description.FrontFaceStencilTest);
        comparison(description.BackFaceStencilTest);
        stencilOperation(description.FrontFaceStencilFailStencilOp);
        stencilOperation(description.FrontFaceDepthFailStencilOp);
        stencilOperation(description.FrontFacePassStencilOp);
        stencilOperation(description.BackFaceStencilFailStencilOp);
        stencilOperation(description.BackFaceDepthFailStencilOp);
        stencilOperation(description.BackFacePassStencilOp);
    }

    bool DepthStencilState::GetInitializer(FDepthStencilStateInitializerRHI& result)
    {
        result = description;
        return true;
    }

    const FDepthStencilStateInitializerRHI& DepthStencilState::GetDescription() const { return description; }
    uint32 DepthStencilState::GetDepthControl(bool hasDepth, bool hasStencil, bool depthBounds) const
    {
        if (depthBounds && !hasDepth) throw std::invalid_argument("Depth bounds require a depth attachment");
        const bool stencil = hasStencil && (description.bEnableFrontFaceStencil || description.bEnableBackFaceStencil);
        const bool depth = hasDepth && (description.bEnableDepthWrite || description.DepthTest != CF_Always);
        return static_cast<uint32>(stencil) | (static_cast<uint32>(depth) << 1) | (static_cast<uint32>(hasDepth && description.bEnableDepthWrite) << 2) | (static_cast<uint32>(depthBounds) << 3) | (comparison(description.DepthTest) << 4) | (static_cast<uint32>(stencil && description.bEnableBackFaceStencil) << 7) | (comparison(description.FrontFaceStencilTest) << 8) | (comparison(description.BackFaceStencilTest) << 20);
    }

    uint32 DepthStencilState::GetStencilControl() const
    {
        return stencilOperation(description.FrontFaceStencilFailStencilOp) | (stencilOperation(description.FrontFacePassStencilOp) << 4) | (stencilOperation(description.FrontFaceDepthFailStencilOp) << 8) | (stencilOperation(description.BackFaceStencilFailStencilOp) << 12) | (stencilOperation(description.BackFacePassStencilOp) << 16) | (stencilOperation(description.BackFaceDepthFailStencilOp) << 20);
    }

    uint32 DepthStencilState::GetStencilReferenceMask(uint32 reference) const
    {
        if (reference > 255) throw std::out_of_range("Stencil reference exceeds eight bits");
        return reference | (static_cast<uint32>(description.StencilReadMask) << 8) | (static_cast<uint32>(description.StencilWriteMask) << 16);
    }

}
