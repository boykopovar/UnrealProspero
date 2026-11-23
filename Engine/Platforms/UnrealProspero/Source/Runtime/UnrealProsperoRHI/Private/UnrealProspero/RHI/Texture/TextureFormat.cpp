#include "UnrealProspero/RHI/Texture/TextureFormat.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    TextureFormatInfo DescribeTextureFormat(TextureFormat format, bool disableSrgb)
    {
        if (disableSrgb && format == TextureFormat::R8Srgb) format = TextureFormat::R8Unorm;
        if (disableSrgb && format == TextureFormat::Rg8Srgb) format = TextureFormat::Rg8Unorm;
        if (disableSrgb && format == TextureFormat::Rgba8Srgb) format = TextureFormat::Rgba8Unorm;
        if (disableSrgb && format == TextureFormat::Bgra8Srgb) format = TextureFormat::Bgra8Unorm;
        if (disableSrgb && format == TextureFormat::Bc1Srgb) format = TextureFormat::Bc1Unorm;
        if (disableSrgb && format == TextureFormat::Bc2Srgb) format = TextureFormat::Bc2Unorm;
        if (disableSrgb && format == TextureFormat::Bc3Srgb) format = TextureFormat::Bc3Unorm;
        if (disableSrgb && format == TextureFormat::Bc7Srgb) format = TextureFormat::Bc7Unorm;
        switch (format)
        {
        case TextureFormat::R8Srgb: return {128, 1, 4 | (1 << 9)};
        case TextureFormat::Rg8Srgb: return {129, 2, 4 | (5 << 3) | (1 << 9)};
        case TextureFormat::R8Unorm: return {1, 1, 4 | (1 << 9)};
        case TextureFormat::A8Unorm: return {1, 1, 4 << 9};
        case TextureFormat::Rg8Unorm: return {14, 2, 4 | (5 << 3) | (1 << 9)};
        case TextureFormat::Rgba8Unorm: return {56, 4, 0xfac};
        case TextureFormat::Rgb10A2Unorm: return {50, 4, 0xfac};
        case TextureFormat::Rgba8Uint: return {60, 4, 0xfac, 1, 1, true};
        case TextureFormat::Rgba8Srgb: return {130, 4, 0xfac};
        case TextureFormat::Bgra8Unorm: return {56, 4, 6 | (5 << 3) | (4 << 6) | (7 << 9)};
        case TextureFormat::Bgra8Srgb: return {130, 4, 6 | (5 << 3) | (4 << 6) | (7 << 9)};
        case TextureFormat::R16Float: return {13, 2, 4 | (1 << 9)};
        case TextureFormat::Rg16Float: return {29, 4, 4 | (5 << 3) | (1 << 9)};
        case TextureFormat::R16Uint: return {11, 2, 4 | (1 << 9), 1, 1, true};
        case TextureFormat::R16Sint: return {12, 2, 4 | (1 << 9), 1, 1, true};
        case TextureFormat::Rg16Uint: return {27, 4, 4 | (5 << 3) | (1 << 9), 1, 1, true};
        case TextureFormat::R32Uint: return {20, 4, 4 | (1 << 9), 1, 1, true};
        case TextureFormat::R32Sint: return {21, 4, 4 | (1 << 9), 1, 1, true};
        case TextureFormat::R11G11B10Float: return {36, 4, 0x3ac};
        case TextureFormat::R32Float: return {22, 4, 4 | (1 << 9)};
        case TextureFormat::R16Unorm: return {7, 2, 4 | (1 << 9)};
        case TextureFormat::Rg16Unorm: return {23, 4, 4 | (5 << 3) | (1 << 9)};
        case TextureFormat::Rgba16Unorm: return {65, 8, 0xfac};
        case TextureFormat::Rgba16Float: return {71, 8, 0xfac};
        case TextureFormat::Rgba32Float: return {77, 16, 0xfac};
        case TextureFormat::Rgba32Uint: return {75, 16, 0xfac, 1, 1, true};
        case TextureFormat::Bc1Unorm: return {169, 8, 0xfac, 4, 4};
        case TextureFormat::Bc1Srgb: return {170, 8, 0xfac, 4, 4};
        case TextureFormat::Bc2Unorm: return {171, 16, 0xfac, 4, 4};
        case TextureFormat::Bc2Srgb: return {172, 16, 0xfac, 4, 4};
        case TextureFormat::Bc3Unorm: return {173, 16, 0xfac, 4, 4};
        case TextureFormat::Bc3Srgb: return {174, 16, 0xfac, 4, 4};
        case TextureFormat::Bc4Unorm: return {175, 8, 4 | (1 << 9), 4, 4};
        case TextureFormat::Bc4Snorm: return {176, 8, 4 | (1 << 9), 4, 4};
        case TextureFormat::Bc5Unorm: return {177, 16, 4 | (5 << 3) | (1 << 9), 4, 4};
        case TextureFormat::Bc5Snorm: return {178, 16, 4 | (5 << 3) | (1 << 9), 4, 4};
        case TextureFormat::Bc6Ufloat: return {179, 16, 4 | (5 << 3) | (6 << 6) | (1 << 9), 4, 4};
        case TextureFormat::Bc6Sfloat: return {180, 16, 4 | (5 << 3) | (6 << 6) | (1 << 9), 4, 4};
        case TextureFormat::Bc7Unorm: return {181, 16, 0xfac, 4, 4};
        case TextureFormat::Bc7Srgb: return {182, 16, 0xfac, 4, 4};
        default: throw std::invalid_argument("Unknown linear texture format");
        }
    }
}
