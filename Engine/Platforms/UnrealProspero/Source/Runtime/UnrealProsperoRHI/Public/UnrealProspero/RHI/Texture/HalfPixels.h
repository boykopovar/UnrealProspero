#ifndef UNREALPROSPERO_RHI_TEXTURE_HALF_PIXELS_H
#define UNREALPROSPERO_RHI_TEXTURE_HALF_PIXELS_H

#include "UnrealProspero/RHI/Texture/Texture2D.h"
#include <vector>

namespace UnrealProspero::RHI
{
    UNREALPROSPERORHI_API bool IsHalfTextureFormat(TextureFormat format);
    UNREALPROSPERORHI_API std::vector<std::array<std::uint16_t, 4>> ExpandHalfPixels(std::span<const std::byte> source, std::size_t width, std::size_t height, std::size_t pitch, TextureFormat format);
}

#endif
