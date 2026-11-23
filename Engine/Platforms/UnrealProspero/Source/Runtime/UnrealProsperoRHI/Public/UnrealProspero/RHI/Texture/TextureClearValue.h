#ifndef UNREALPROSPERO_RHI_TEXTURE_CLEAR_VALUE_H
#define UNREALPROSPERO_RHI_TEXTURE_CLEAR_VALUE_H

#include "UnrealProspero/RHI/Texture/Texture2D.h"

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API std::array<std::uint32_t, 4> EncodeTextureClear(TextureFormat format, const std::array<float, 4>& values);
    UNREALPROSPERORHI_API std::array<std::uint32_t, 4> EncodeTextureClear(TextureFormat format, const std::array<std::uint32_t, 4>& values);
}

#endif
