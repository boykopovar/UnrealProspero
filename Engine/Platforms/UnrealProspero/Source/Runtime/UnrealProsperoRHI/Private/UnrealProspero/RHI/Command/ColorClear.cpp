#include "UnrealProspero/RHI/Command/ColorClear.h"
#include "UnrealProspero/RHI/Command/PatternFill.h"
#include "UnrealProspero/RHI/Texture/TextureClearValue.h"
#include <stdexcept>

namespace UnrealProspero::RHI
{
    void WriteColorClear(CommandBuffer& commands, const Texture2D& texture, const std::array<float, 4>& color, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        const auto values = EncodeTextureClear(texture.GetFormat(), color);
        const auto storage = texture.GetMipRange(mipLevel, arrayLayer);
        WritePatternFill(commands, storage.data(), storage.size(), values);
    }

    void WriteColorClear(CommandBuffer& commands, const LinearTexture3D& texture, const std::array<float, 4>& color, std::uint32_t mipLevel, std::uint32_t firstSlice, std::uint32_t sliceCount)
    {
        const auto& mip = texture.GetMipLayout(mipLevel);
        if (firstSlice >= mip.Depth) throw std::out_of_range("Volume clear starts past the mip depth");
        if (sliceCount == 0) sliceCount = mip.Depth - firstSlice;
        if (sliceCount > mip.Depth - firstSlice) throw std::out_of_range("Volume clear exceeds the mip depth");
        const auto values = EncodeTextureClear(texture.GetFormat(), color);
        for (std::uint32_t slice = firstSlice; slice < firstSlice + sliceCount; ++slice)
        {
            const auto storage = texture.GetSliceRange(mipLevel, slice);
            WritePatternFill(commands, storage.data(), storage.size(), values);
        }
    }
}
