#ifndef ANYUNREAL_RHI_TEXTURE_CLEAR_VALUE_H
#define ANYUNREAL_RHI_TEXTURE_CLEAR_VALUE_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API std::array<std::uint32_t, 4> EncodeTextureClear(TextureFormat format, const std::array<float, 4>& values);
    ANYUNREALRHI_API std::array<std::uint32_t, 4> EncodeTextureClear(TextureFormat format, const std::array<std::uint32_t, 4>& values);
}

#endif
