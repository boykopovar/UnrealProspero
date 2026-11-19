#include "UnrealProspero/RHI/Pipeline/ColorExportFormat.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    std::uint32_t ResolveColorExportFormat(std::uint32_t exports, std::span<const EPixelFormat> formats)
    {
        if (formats.size() > 8) throw std::invalid_argument("Pixel shader exceeds eight color targets");
        auto result = exports;
        for (std::uint32_t index = 0; index < formats.size(); ++index)
        {
            if (((exports >> (index * 4)) & 15u) == 0 || formats[index] == PF_Unknown) continue;
            std::uint32_t format;
            switch (formats[index])
            {
            case PF_G8:
            case PF_R8:
            case PF_R8G8:
            case PF_R8G8B8A8:
            case PF_B8G8R8A8:
            case PF_A2B10G10R10:
            case PF_R16F:
            case PF_R16F_FILTER:
            case PF_G16R16F:
            case PF_G16R16F_FILTER:
            case PF_FloatRGBA:
            case PF_FloatR11G11B10: format = 4; break;
            case PF_G16:
            case PF_G16R16:
            case PF_R16G16B16A16_UNORM:
            case PF_R32_FLOAT:
            case PF_A32B32G32R32F: format = 9; break;
            default: throw std::invalid_argument("Pixel shader color target export format is not implemented");
            }
            result = (result & ~(15u << (index * 4))) | (format << (index * 4));
        }
        return result;
    }
}
