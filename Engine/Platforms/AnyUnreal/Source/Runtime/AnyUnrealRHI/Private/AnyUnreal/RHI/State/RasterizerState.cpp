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
    uint32 RasterizerState::UpdateModeControl(uint32 value, bool depthBias) const
    {
        const bool clockwiseFront = (value & 4u) != 0;
        uint32 cull = 0;
        switch (description.CullMode)
        {
        case CM_None: break;
        case CM_CW: cull = clockwiseFront ? 1u : 2u; break;
        case CM_CCW: cull = clockwiseFront ? 2u : 1u; break;
        default: throw std::invalid_argument("Unknown rasterizer cull mode");
        }
        uint32 polygon;
        switch (description.FillMode)
        {
        case FM_Point: polygon = 1u << 3; break;
        case FM_Wireframe: polygon = (1u << 3) | (1u << 5) | (1u << 8); break;
        case FM_Solid: polygon = (2u << 5) | (2u << 8); break;
        default: throw std::invalid_argument("Unknown rasterizer fill mode");
        }
        constexpr uint32 mask = 3u | (3u << 3) | (7u << 5) | (7u << 8) | (7u << 11);
        return (value & ~mask) | cull | polygon | (depthBias ? (7u << 11) : 0u);
    }

    uint32 RasterizerState::UpdateClipControl(uint32 value) const
    {
        constexpr uint32 mask = (1u << 26) | (1u << 27);
        return (value & ~mask) | (description.DepthClipMode == ERasterizerDepthClipMode::DepthClamp ? mask : 0u);
    }

}
