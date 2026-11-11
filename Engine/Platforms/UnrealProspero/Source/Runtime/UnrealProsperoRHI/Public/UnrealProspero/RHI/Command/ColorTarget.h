#ifndef UNREALPROSPERO_RHI_COLOR_TARGET_H
#define UNREALPROSPERO_RHI_COLOR_TARGET_H

#include "UnrealProspero/RHI/Texture/LinearTexture2D.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteColorTarget(CommandBuffer& commands, std::uint32_t index, const LinearTexture2D& texture, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
    UNREALPROSPERORHI_API void DisableColorTarget(CommandBuffer& commands, std::uint32_t index);
}

#endif
