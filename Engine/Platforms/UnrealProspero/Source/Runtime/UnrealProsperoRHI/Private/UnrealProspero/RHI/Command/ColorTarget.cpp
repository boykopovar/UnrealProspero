#include "UnrealProspero/RHI/Command/ColorTarget.h"
#include "prx/libSceAgc/DcbState/include/Registers.hpp"
#include <array>
#include <stdexcept>

namespace UnrealProspero::RHI
{
    namespace
    {
        std::uint32_t colorInfo(TextureFormat format)
        {
            switch (format)
            {
            case TextureFormat::R8Unorm: return (1 << 2) | (1 << 15) | (1 << 17);
            case TextureFormat::Rg8Unorm: return (3 << 2) | (1 << 15) | (1 << 17);
            case TextureFormat::Rgba8Unorm: return (10 << 2) | (1 << 15) | (1 << 17);
            case TextureFormat::Rgb10A2Unorm: return (9 << 2) | (1 << 15) | (1 << 17);
            case TextureFormat::Rgba8Srgb: return (10 << 2) | (6 << 8) | (1 << 15) | (1 << 17);
            case TextureFormat::Bgra8Unorm: return (10 << 2) | (1 << 11) | (1 << 15) | (1 << 17);
            case TextureFormat::Bgra8Srgb: return (10 << 2) | (6 << 8) | (1 << 11) | (1 << 15) | (1 << 17);
            case TextureFormat::R16Float: return (2 << 2) | (7 << 8) | (1 << 17) | (1 << 18);
            case TextureFormat::Rg16Float: return (5 << 2) | (7 << 8) | (1 << 17) | (1 << 18);
            case TextureFormat::R32Float: return (4 << 2) | (7 << 8) | (1 << 17) | (1 << 18);
            case TextureFormat::Rgba16Float: return (12 << 2) | (7 << 8) | (1 << 17) | (1 << 18);
            case TextureFormat::Rgba32Float: return (14 << 2) | (7 << 8) | (1 << 17) | (1 << 18);
            default: throw std::invalid_argument("Unknown color target format");
            }
        }

        void checkIndex(std::uint32_t index)
        {
            if (index >= 8) throw std::out_of_range("Color target index exceeds the hardware limit");
        }
    }

    void DisableColorTarget(CommandBuffer& commands, std::uint32_t index)
    {
        checkIndex(index);
        sceAgcDcbSetCxRegisterDirect(&commands, {0x31c + index * 15, 0});
    }

    void WriteColorTarget(CommandBuffer& commands, std::uint32_t index, const Texture2D& texture, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        checkIndex(index);
        const auto& mip = texture.GetMipLayout(mipLevel);
        const auto info = colorInfo(texture.GetFormat());
        const auto address = reinterpret_cast<std::uintptr_t>(texture.GetMipRange(mipLevel, arrayLayer).data());
        const auto bytesPerPixel = mip.RowBytes / mip.Width;
        const auto pitch = mip.RowPitch / bytesPerPixel;
        if (pitch == 0 || pitch > 16384) throw std::out_of_range("Color target pitch exceeds the hardware limit");
        const auto format = texture.GetFormat();
        const bool forceAlpha = format == TextureFormat::R8Unorm || format == TextureFormat::Rg8Unorm || format == TextureFormat::R32Float || format == TextureFormat::R16Float || format == TextureFormat::Rg16Float;
        const auto base = index * 15;
        const std::array registers{
            ShaderRegister{0x318 + base, static_cast<std::uint32_t>(address >> 8)},
            ShaderRegister{0x31b + base, 0}, ShaderRegister{0x31c + base, info},
            ShaderRegister{0x31d + base, forceAlpha ? 1u << 17 : 0}, ShaderRegister{0x31e + base, 0},
            ShaderRegister{0x31f + base, 0}, ShaderRegister{0x321 + base, 0}, ShaderRegister{0x323 + base, 0},
            ShaderRegister{0x324 + base, 0}, ShaderRegister{0x325 + base, 0},
            ShaderRegister{0x390 + index, static_cast<std::uint32_t>(address >> 40)},
            ShaderRegister{0x398 + index, 0}, ShaderRegister{0x3a0 + index, 0}, ShaderRegister{0x3a8 + index, 0},
            ShaderRegister{0x3b0 + index, (mip.Height - 1) | (static_cast<std::uint32_t>(pitch - 1) << 14)},
            ShaderRegister{0x3b8 + index, 0x09000000}
        };
        for (const auto value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
    }
}
