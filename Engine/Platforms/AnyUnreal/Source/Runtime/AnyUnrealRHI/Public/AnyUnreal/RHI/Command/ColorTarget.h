#ifndef ANYUNREAL_RHI_COLOR_TARGET_H
#define ANYUNREAL_RHI_COLOR_TARGET_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteColorTarget(CommandBuffer& commands, std::uint32_t index, const LinearTexture2D& texture, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
    ANYUNREALRHI_API void DisableColorTarget(CommandBuffer& commands, std::uint32_t index);
}

#endif
