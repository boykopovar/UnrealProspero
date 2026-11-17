#include "UnrealProspero/RHI/Texture/PixelFormat.h"
#include "UnrealProspero/RHI/Buffer/TypedFormat.h"
#include <array>
#include <optional>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        struct FormatMapping
        {
            EPixelFormat Pixel;
            TextureFormat Linear;
            std::optional<TextureFormat> Srgb;
        };

        constexpr std::array pixelFormatMappings{
            FormatMapping{PF_R8G8B8A8, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb},
            FormatMapping{PF_B8G8R8A8, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb},
            FormatMapping{PF_DXT1, TextureFormat::Bc1Unorm, TextureFormat::Bc1Srgb},
            FormatMapping{PF_DXT3, TextureFormat::Bc2Unorm, TextureFormat::Bc2Srgb},
            FormatMapping{PF_DXT5, TextureFormat::Bc3Unorm, TextureFormat::Bc3Srgb},
            FormatMapping{PF_BC4, TextureFormat::Bc4Unorm, std::nullopt},
            FormatMapping{PF_BC5, TextureFormat::Bc5Unorm, std::nullopt},
            FormatMapping{PF_BC6H, TextureFormat::Bc6Ufloat, std::nullopt},
            FormatMapping{PF_BC7, TextureFormat::Bc7Unorm, TextureFormat::Bc7Srgb},
            FormatMapping{PF_G8, TextureFormat::R8Unorm, std::nullopt},
            FormatMapping{PF_R8, TextureFormat::R8Unorm, std::nullopt},
            FormatMapping{PF_R8G8, TextureFormat::Rg8Unorm, std::nullopt},
            FormatMapping{PF_R16F, TextureFormat::R16Float, std::nullopt},
            FormatMapping{PF_R16F_FILTER, TextureFormat::R16Float, std::nullopt},
            FormatMapping{PF_G16R16F, TextureFormat::Rg16Float, std::nullopt},
            FormatMapping{PF_G16R16F_FILTER, TextureFormat::Rg16Float, std::nullopt},
            FormatMapping{PF_R32_UINT, TextureFormat::R32Uint, std::nullopt},
            FormatMapping{PF_R32_SINT, TextureFormat::R32Sint, std::nullopt},
            FormatMapping{PF_R32_FLOAT, TextureFormat::R32Float, std::nullopt},
            FormatMapping{PF_FloatRGBA, TextureFormat::Rgba16Float, std::nullopt},
            FormatMapping{PF_A32B32G32R32F, TextureFormat::Rgba32Float, std::nullopt}
        };
    }

    TextureFormat ResolveTextureFormat(EPixelFormat format, bool srgb)
    {
        for (const auto& mapping : pixelFormatMappings)
        {
            if (mapping.Pixel != format) continue;
            if (!srgb) return mapping.Linear;
            if (!mapping.Srgb) throw std::runtime_error("sRGB is not implemented for this texture format");
            return *mapping.Srgb;
        }
        throw std::runtime_error("Texture pixel format is not implemented");
    }

    void ConfigurePixelFormats()
    {
        using Caps = EPixelFormatCapabilities;
        for (auto& format : GPixelFormats)
        {
            format.Supported = false;
            format.Capabilities = Caps::None;
        }
        constexpr auto colorCaps = Caps::Texture2D | Caps::Texture3D | Caps::TextureCube | Caps::TextureMipmaps | Caps::RenderTarget | Caps::TextureLoad | Caps::TextureSample | Caps::TextureFilterable | Caps::TextureBlendable | Caps::TextureStore | Caps::UAV | Caps::TypedUAVLoad | Caps::TypedUAVStore;
        for (const auto& mapping : pixelFormatMappings)
        {
            auto& format = GPixelFormats[mapping.Pixel];
            format.Supported = true;
            const auto info = DescribeTextureFormat(mapping.Linear);
            if (format.BlockSizeX != info.BlockWidth || format.BlockSizeY != info.BlockHeight || format.BlockBytes != info.BlockBytes) throw std::runtime_error("Texture format block layout does not match Unreal");
            if (info.BlockWidth > 1)
            {
                format.Capabilities = Caps::Texture2D | Caps::Texture3D | Caps::TextureCube | Caps::TextureMipmaps | Caps::TextureLoad | Caps::TextureSample | Caps::TextureFilterable;
                continue;
            }
            const bool integer = mapping.Linear == TextureFormat::R32Uint || mapping.Linear == TextureFormat::R32Sint;
            format.Capabilities = integer ? colorCaps & ~(Caps::TextureCube | Caps::RenderTarget | Caps::TextureSample | Caps::TextureFilterable | Caps::TextureBlendable) : colorCaps;
        }
        auto& depth = GPixelFormats[PF_ShadowDepth];
        depth.Supported = true;
        depth.BlockBytes = 4;
        depth.NumComponents = 1;
        depth.bIs24BitUnormDepthStencil = false;
        depth.Capabilities = Caps::Texture2D | Caps::DepthStencil | Caps::TextureLoad | Caps::TextureSample | Caps::TextureFilterable;
        for (auto pixel : {PF_R16_UINT, PF_R32_UINT})
        {
            auto& format = GPixelFormats[pixel];
            format.Capabilities |= Caps::Buffer | Caps::IndexBuffer;
        }
        ConfigureTypedBufferFormats();
    }
}
