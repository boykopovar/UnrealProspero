#ifndef ANYUNREAL_RHI_TEXTURE_HALF_PIXELS_H
#define ANYUNREAL_RHI_TEXTURE_HALF_PIXELS_H

#include "AnyUnreal/RHI/Texture/LinearTexture2D.h"
#include <vector>

namespace AnyUnreal::RHI
{
    ANYUNREALRHI_API bool IsHalfTextureFormat(TextureFormat format);
    ANYUNREALRHI_API std::vector<std::array<std::uint16_t, 4>> ExpandHalfPixels(std::span<const std::byte> source, std::size_t width, std::size_t height, std::size_t pitch, TextureFormat format);
}

#endif
