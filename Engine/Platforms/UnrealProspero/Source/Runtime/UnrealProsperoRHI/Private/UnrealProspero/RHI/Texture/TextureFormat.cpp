#include "UnrealProspero/RHI/Texture/TextureFormat.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    TextureFormatInfo DescribeTextureFormat(TextureFormat format, bool disableSrgb)
    {
        if (disableSrgb && format == TextureFormat::Rgba8Srgb) format = TextureFormat::Rgba8Unorm;
        if (disableSrgb && format == TextureFormat::Bgra8Srgb) format = TextureFormat::Bgra8Unorm;
        switch (format)
        {
        case TextureFormat::R8Unorm: return {1, 1, 4 | (1 << 9)};
        case TextureFormat::Rg8Unorm: return {14, 2, 4 | (5 << 3) | (1 << 9)};
        case TextureFormat::Rgba8Unorm: return {56, 4, 0xfac};
        case TextureFormat::Rgba8Srgb: return {130, 4, 0xfac};
        case TextureFormat::Bgra8Unorm: return {56, 4, 6 | (5 << 3) | (4 << 6) | (7 << 9)};
        case TextureFormat::Bgra8Srgb: return {130, 4, 6 | (5 << 3) | (4 << 6) | (7 << 9)};
        case TextureFormat::R16Float: return {13, 2, 4 | (1 << 9)};
        case TextureFormat::Rg16Float: return {29, 4, 4 | (5 << 3) | (1 << 9)};
        case TextureFormat::R32Uint: return {20, 4, 4 | (1 << 9)};
        case TextureFormat::R32Sint: return {21, 4, 4 | (1 << 9)};
        case TextureFormat::R32Float: return {22, 4, 4 | (1 << 9)};
        case TextureFormat::Rgba16Float: return {71, 8, 0xfac};
        case TextureFormat::Rgba32Float: return {77, 16, 0xfac};
        default: throw std::invalid_argument("Unknown linear texture format");
        }
    }
}
