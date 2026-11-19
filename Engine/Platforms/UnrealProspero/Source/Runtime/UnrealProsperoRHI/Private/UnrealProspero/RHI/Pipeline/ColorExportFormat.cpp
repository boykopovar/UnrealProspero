#include "UnrealProspero/RHI/Pipeline/ColorExportFormat.h"
#include "UnrealProspero/RHI/Texture/PixelFormat.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    std::uint32_t ResolveColorExportFormat(std::uint32_t exports, std::span<const EPixelFormat> formats)
    {
        if (formats.size() > 8) throw std::invalid_argument("Pixel shader exceeds eight color targets");
        for (std::uint32_t index = 0; index < 8; ++index) if (((exports >> (index * 4)) & 15u) > 9) throw std::invalid_argument("Invalid pixel shader color export format");
        auto result = exports;
        for (std::uint32_t index = 0; index < formats.size(); ++index)
        {
            if (formats[index] >= PF_MAX) throw std::invalid_argument("Invalid pixel shader color target format");
            if (((exports >> (index * 4)) & 15u) == 0 || formats[index] == PF_Unknown) continue;
            std::uint32_t format;
            switch (ResolveTextureFormat(formats[index], false))
            {
            case TextureFormat::R8Unorm:
            case TextureFormat::Rg8Unorm:
            case TextureFormat::Rgba8Unorm:
            case TextureFormat::Bgra8Unorm:
            case TextureFormat::Rgb10A2Unorm:
            case TextureFormat::R16Float:
            case TextureFormat::Rg16Float:
            case TextureFormat::Rgba16Float:
            case TextureFormat::R11G11B10Float: format = 4; break;
            case TextureFormat::R16Unorm:
            case TextureFormat::Rg16Unorm:
            case TextureFormat::Rgba16Unorm:
            case TextureFormat::R32Float:
            case TextureFormat::Rgba32Float: format = 9; break;
            default: throw std::invalid_argument("Pixel shader color target export format is not implemented");
            }
            result = (result & ~(15u << (index * 4))) | (format << (index * 4));
        }
        return result;
    }
}
