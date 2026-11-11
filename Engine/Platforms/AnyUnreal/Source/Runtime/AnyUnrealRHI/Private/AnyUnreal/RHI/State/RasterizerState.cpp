#include "AnyUnreal/RHI/State/RasterizerState.h"
#include <cmath>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    RasterizerState::RasterizerState(const FRasterizerStateInitializerRHI& initializer) : description(initializer)
    {
        switch (description.FillMode)
        {
        case FM_Point:
        case FM_Wireframe:
        case FM_Solid: break;
        default: throw std::invalid_argument("Unknown rasterizer fill mode");
        }
        switch (description.CullMode)
        {
        case CM_None:
        case CM_CW:
        case CM_CCW: break;
        default: throw std::invalid_argument("Unknown rasterizer cull mode");
        }
        switch (description.DepthClipMode)
        {
        case ERasterizerDepthClipMode::DepthClip:
        case ERasterizerDepthClipMode::DepthClamp: break;
        default: throw std::invalid_argument("Unknown rasterizer depth clipping mode");
        }
        if (!std::isfinite(description.DepthBias) || !std::isfinite(description.SlopeScaleDepthBias)) throw std::invalid_argument("Rasterizer depth bias must be finite");
    }

    bool RasterizerState::GetInitializer(FRasterizerStateInitializerRHI& result)
    {
        result = description;
        return true;
    }

    const FRasterizerStateInitializerRHI& RasterizerState::GetDescription() const { return description; }
}
