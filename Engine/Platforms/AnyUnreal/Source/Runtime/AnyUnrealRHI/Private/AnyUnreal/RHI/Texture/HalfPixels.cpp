#include "AnyUnreal/RHI/Texture/HalfPixels.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace AnyUnreal::RHI
{
    static_assert(sizeof(std::array<std::uint16_t, 4>) == 8);
    bool IsHalfTextureFormat(TextureFormat format)
    {
        return format == TextureFormat::R16Float || format == TextureFormat::Rg16Float || format == TextureFormat::Rgba16Float;
    }

    std::vector<std::array<std::uint16_t, 4>> ExpandHalfPixels(std::span<const std::byte> source, std::size_t width, std::size_t height, std::size_t pitch, TextureFormat format)
    {
        std::size_t channels;
        switch (format)
        {
        case TextureFormat::R16Float: channels = 1; break;
        case TextureFormat::Rg16Float: channels = 2; break;
        case TextureFormat::Rgba16Float: channels = 4; break;
        default: throw std::invalid_argument("Pixel expansion requires a half-float texture");
        }
        const auto pixelBytes = channels * sizeof(std::uint16_t);
        if (width == 0 || height == 0 || width > std::numeric_limits<std::size_t>::max() / pixelBytes || height > std::numeric_limits<std::size_t>::max() / width / sizeof(std::array<std::uint16_t, 4>)) throw std::invalid_argument("Invalid half-float pixel extent");
        const auto rowBytes = width * pixelBytes;
        if (!source.data() || pitch < rowBytes || source.size() < rowBytes || height - 1 > (source.size() - rowBytes) / pitch) throw std::invalid_argument("Half-float source does not contain all rows");
        std::vector<std::array<std::uint16_t, 4>> result(width * height, {0, 0, 0, 0x3c00});
        for (std::size_t row = 0; row < height; ++row)
        for (std::size_t column = 0; column < width; ++column)
            std::memcpy(result[row * width + column].data(), source.data() + row * pitch + column * pixelBytes, pixelBytes);
        return result;
    }
}
