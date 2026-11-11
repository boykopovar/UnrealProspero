#ifndef ANYUNREAL_RHI_DEPTH_TARGET_H
#define ANYUNREAL_RHI_DEPTH_TARGET_H

#include "AnyUnreal/RHI/Texture/DepthTexture2D.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteDepthTarget(CommandBuffer& commands, const DepthTexture2D& texture);
    ANYUNREALRHI_API void WriteDepthClear(CommandBuffer& commands, const DepthTexture2D& texture, float depth);
    ANYUNREALRHI_API void DisableDepthTarget(CommandBuffer& commands);
}

#endif
