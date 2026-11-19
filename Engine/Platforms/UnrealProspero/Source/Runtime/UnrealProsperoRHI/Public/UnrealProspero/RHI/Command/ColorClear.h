#ifndef UNREALPROSPERO_RHI_COMMAND_COLOR_CLEAR_H
#define UNREALPROSPERO_RHI_COMMAND_COLOR_CLEAR_H

#include "UnrealProspero/RHI/Texture/Texture2D.h"
#include "UnrealProspero/RHI/Texture/LinearTexture3D.h"
#include "SceTypes.hpp"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API void WriteColorClear(CommandBuffer& commands, const Texture2D& texture, const std::array<float, 4>& color, std::uint32_t mipLevel = 0, std::uint32_t arrayLayer = 0);
    UNREALPROSPERORHI_API void WriteColorClear(CommandBuffer& commands, const LinearTexture3D& texture, const std::array<float, 4>& color, std::uint32_t mipLevel = 0, std::uint32_t firstSlice = 0, std::uint32_t sliceCount = 0);
}

#endif
