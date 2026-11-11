#ifndef UNREALPROSPERO_RHI_COMMAND_COLOR_CLEAR_H
#define UNREALPROSPERO_RHI_COMMAND_COLOR_CLEAR_H

#include "UnrealProspero/RHI/Texture/LinearTexture2D.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteColorClear(CommandBuffer& commands, const LinearTexture2D& texture, const std::array<float, 4>& color, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
}

#endif
