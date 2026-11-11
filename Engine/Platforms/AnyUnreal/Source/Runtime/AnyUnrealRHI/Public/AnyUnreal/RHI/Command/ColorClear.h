#ifndef ANYUNREAL_RHI_COMMAND_COLOR_CLEAR_H
#define ANYUNREAL_RHI_COMMAND_COLOR_CLEAR_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include "SceTypes.hpp"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API void WriteColorClear(CommandBuffer& commands, const LinearTexture2D& texture, const std::array<float, 4>& color, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
}

#endif
