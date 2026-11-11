#include "AnyUnreal/RHI/Texture/PixelFormat.h"
#include <array>
#include <optional>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    namespace
    {
        struct FormatMapping
        {
            EPixelFormat Pixel;
            TextureFormat Linear;
            std::optional<TextureFormat> Srgb;
        };

        constexpr std::array mappings{
            FormatMapping{PF_R8G8B8A8, TextureFormat::Rgba8Unorm, TextureFormat::Rgba8Srgb},
            FormatMapping{PF_B8G8R8A8, TextureFormat::Bgra8Unorm, TextureFormat::Bgra8Srgb},
            FormatMapping{PF_G8, TextureFormat::R8Unorm, std::nullopt},
            FormatMapping{PF_R8, TextureFormat::R8Unorm, std::nullopt},
            FormatMapping{PF_R8G8, TextureFormat::Rg8Unorm, std::nullopt},
            FormatMapping{PF_R32_FLOAT, TextureFormat::R32Float, std::nullopt},
            FormatMapping{PF_FloatRGBA, TextureFormat::Rgba16Float, std::nullopt},
            FormatMapping{PF_A32B32G32R32F, TextureFormat::Rgba32Float, std::nullopt}
        };
    }

    TextureFormat ResolveTextureFormat(EPixelFormat format, bool srgb)
    {
        for (const auto& mapping : mappings)
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
        constexpr auto colorCaps = Caps::Texture2D | Caps::Texture3D | Caps::RenderTarget | Caps::TextureLoad | Caps::TextureSample | Caps::TextureFilterable | Caps::TextureBlendable | Caps::TextureStore | Caps::UAV | Caps::TypedUAVLoad | Caps::TypedUAVStore;
        for (const auto& mapping : mappings)
        {
            auto& format = GPixelFormats[mapping.Pixel];
            format.Supported = true;
            format.Capabilities = colorCaps;
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
            format.Capabilities = Caps::Buffer | Caps::IndexBuffer;
        }
    }
}
