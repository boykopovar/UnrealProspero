#ifndef UNREALPROSPERO_RHI_TEXTURE_FORMAT_H
#define UNREALPROSPERO_RHI_TEXTURE_FORMAT_H

#include <cstdint>

namespace UnrealProspero::RHI
{
    enum class TextureFormat
    {
        R8Unorm,
        Rg8Unorm,
        Rgba8Unorm,
        Rgba8Srgb,
        Bgra8Unorm,
        Bgra8Srgb,
        R32Float,
        R32Uint,
        R32Sint,
        Rgba32Float,
        Rgba16Float,
        R16Float,
        Rg16Float,
        Bc1Unorm,
        Bc1Srgb,
        Bc2Unorm,
        Bc2Srgb,
        Bc3Unorm,
        Bc3Srgb,
        Bc4Unorm,
        Bc4Snorm,
        Bc5Unorm,
        Bc5Snorm,
        Bc6Ufloat,
        Bc6Sfloat,
        Bc7Unorm,
        Bc7Srgb,
        Rgba8Uint,
        Rgba32Uint,
        Rgb10A2Unorm
    };

    struct TextureFormatInfo
    {
        std::uint32_t Code;
        std::uint32_t BlockBytes;
        std::uint32_t Swizzle;
        std::uint32_t BlockWidth = 1;
        std::uint32_t BlockHeight = 1;
        bool Integer = false;
    };

    UNREALPROSPERORHI_API TextureFormatInfo DescribeTextureFormat(TextureFormat format, bool disableSrgb = false);
}

#endif
