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
            case TextureFormat::R8Srgb: return (1 << 2) | (6 << 8) | (1 << 15) | (1 << 17);
            case TextureFormat::Rg8Srgb: return (3 << 2) | (6 << 8) | (1 << 15) | (1 << 17);
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

        struct TargetRegisters
        {
            std::uintptr_t Address;
            TextureFormat Format;
            std::uint32_t Height;
            std::uint32_t Pitch;
            std::uint32_t View;
            std::uint32_t Attributes;
            std::uint32_t MaxMip = 0;
        };

        void writeTarget(CommandBuffer& commands, std::uint32_t index, const TargetRegisters& target)
        {
            checkIndex(index);
            if (target.Pitch == 0 || target.Pitch > 16384 || target.Height == 0 || target.Height > 16384) throw std::out_of_range("Color target dimensions exceed the hardware limit");
            const auto format = target.Format;
            const bool forceAlpha = format == TextureFormat::R8Unorm || format == TextureFormat::R8Srgb || format == TextureFormat::Rg8Unorm || format == TextureFormat::Rg8Srgb || format == TextureFormat::R32Float || format == TextureFormat::R16Float || format == TextureFormat::Rg16Float;
            const auto base = index * 15;
            const std::array registers{
                ShaderRegister{0x318 + base, static_cast<std::uint32_t>(target.Address >> 8)},
                ShaderRegister{0x31b + base, target.View}, ShaderRegister{0x31c + base, colorInfo(target.Format)},
                ShaderRegister{0x31d + base, forceAlpha ? 1u << 17 : 0}, ShaderRegister{0x31e + base, 0},
                ShaderRegister{0x31f + base, 0}, ShaderRegister{0x321 + base, 0}, ShaderRegister{0x323 + base, 0},
                ShaderRegister{0x324 + base, 0}, ShaderRegister{0x325 + base, 0},
                ShaderRegister{0x390 + index, static_cast<std::uint32_t>(target.Address >> 40)},
                ShaderRegister{0x398 + index, 0}, ShaderRegister{0x3a0 + index, 0}, ShaderRegister{0x3a8 + index, 0},
                ShaderRegister{0x3b0 + index, (target.Height - 1) | (static_cast<std::uint32_t>(target.Pitch - 1) << 14) | (target.MaxMip << 28)},
                ShaderRegister{0x3b8 + index, target.Attributes}
            };
            for (const auto value : registers) sceAgcDcbSetCxRegisterDirect(&commands, value);
        }

    }

    void DisableColorTarget(CommandBuffer& commands, std::uint32_t index)
    {
        checkIndex(index);
        sceAgcDcbSetCxRegisterDirect(&commands, {0x31c + index * 15, 0});
    }

    void WriteColorTarget(CommandBuffer& commands, std::uint32_t index, const Texture2D& texture, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        const auto& mip = texture.GetMipLayout(mipLevel);
        const auto address = reinterpret_cast<std::uintptr_t>(texture.GetMipRange(mipLevel, arrayLayer).data());
        const auto pitch = static_cast<std::uint32_t>(mip.RowPitch / (mip.RowBytes / mip.Width));
        const auto attributes = 0x09000000u | ((texture.GetLayout() == Texture2DLayout::RenderTarget64K ? 27u : 0u) << 14);
        writeTarget(commands, index, {address, texture.GetFormat(), mip.Height, pitch, 0, attributes});
    }

    void WriteColorTarget(CommandBuffer& commands, std::uint32_t index, const LinearTexture3D& texture, std::uint32_t mipLevel, std::uint32_t firstSlice, std::uint32_t sliceCount)
    {
        const auto& mip = texture.GetMipLayout(mipLevel);
        if (firstSlice >= mip.Depth) throw std::out_of_range("Volume attachment starts past the mip depth");
        if (sliceCount == 0) sliceCount = mip.Depth - firstSlice;
        if (sliceCount > mip.Depth - firstSlice) throw std::out_of_range("Volume attachment exceeds the mip depth");
        const auto address = reinterpret_cast<std::uintptr_t>(texture.GetStorage().Data());
        const auto pitch = static_cast<std::uint32_t>(texture.GetRowPitch() / DescribeTextureFormat(texture.GetFormat()).BlockBytes);
        const auto view = firstSlice | ((firstSlice + sliceCount - 1) << 13) | (mipLevel << 26);
        const auto attributes = 0x0a000000u | (texture.GetDepth() - 1);
        writeTarget(commands, index, {address, texture.GetFormat(), texture.GetHeight(), pitch, view, attributes, texture.GetMipCount() - 1});
    }
}
