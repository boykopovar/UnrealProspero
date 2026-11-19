#ifndef UNREALPROSPERO_RHI_DEPTH_TARGET_H
#define UNREALPROSPERO_RHI_DEPTH_TARGET_H

#include "UnrealProspero/RHI/Texture/DepthTexture2D.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteDepthTarget(CommandBuffer& commands, const DepthTexture2D& texture);
    UNREALPROSPERORHI_API void WriteDepthClear(CommandBuffer& commands, const DepthTexture2D& texture, float depth);
    UNREALPROSPERORHI_API void WriteStencilClear(CommandBuffer& commands, const DepthTexture2D& texture, std::uint8_t stencil);
    UNREALPROSPERORHI_API void DisableDepthTarget(CommandBuffer& commands);
}

#endif
